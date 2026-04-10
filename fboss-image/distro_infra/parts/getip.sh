#!/bin/bash

# getip.sh - MAC Address to IP Resolution Utility (JSON Output)
#
# Description:
#   Resolves IP addresses (IPv4/IPv6) from MAC addresses using the kernel's
#   neighbor table (ARP/NDP cache). Supports optional network interface filtering.
#   Returns results in JSON format.
#
# Usage:
#   getip.sh <MAC_ADDRESS> [INTERFACE]
#
# Algorithm:
#   1. Check neighbor table for existing MAC-to-IP mappings
#   2. If found: Ping specific IPs to verify and refresh the mapping
#   3. If not found: Ping broadcast (IPv4) and multicast (IPv6) to discover devices
#   4. Wait for neighbor table to update (1 second)
#   5. Query neighbor table again and return the IP addresses
#
# Output Format (JSON):
#   Success:
#     {
#       "mac": "aa:bb:cc:dd:ee:ff",
#       "interface": "eth0",           # Optional, if interface specified
#       "ipv4": "192.168.1.100",       # Optional, if IPv4 found
#       "ipv6": "fe80::1"              # Optional, if IPv6 found
#     }
#
#   Error (MAC not found):
#     {
#       "mac": "aa:bb:cc:dd:ee:ff",
#       "error_code": "MAC_NOT_FOUND",
#       "error": "MAC address not found in ip neighbor table."
#     }
#
#   Error (Invalid arguments):
#     {
#       "error_code": "INVALID_ARGUMENTS",
#       "error": "MAC address argument required. Use -h for help."
#     }
#
#   Error (Command failed):
#     {
#       "error_code": "COMMAND_FAILED",
#       "error": "Command 'ip -4 neighbor show dev eth99' failed: Device \"eth99\" does not exist."
#     }
#
# Error Codes:
#   MAC_NOT_FOUND       - The specified MAC address was not found in the neighbor table
#   INVALID_ARGUMENTS   - Missing or invalid command-line arguments
#   COMMAND_FAILED      - A system command (ip) failed to execute (command included in error message)
#
# Exit Codes:
#   0 - Success: IP address found and returned
#   1 - Error: MAC address not found in neighbor table or command failed
#   2 - Error: Invalid arguments or missing MAC address
#
# Dependencies:
#   - iproute (ip command)
#   - iputils (ping, ping6 commands)
#   - jq (JSON processor)

print_usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS] <MAC_ADDRESS> [INTERFACE]

Get the IP address associated with a MAC address and an optional interface
from the ip neighbor table. Returns results in JSON format.

Arguments:
  MAC_ADDRESS    The MAC address to look up (e.g., aa:bb:cc:dd:ee:ff)
  INTERFACE      (Optional) The network interface to filter the search (e.g., eth0)

Options:
  -h             Show this help message and exit

Output:
  JSON object containing mac, ipv4, ipv6, and optional interface fields.
  On error, returns JSON with error_code and error fields. Possible error codes:
    - INVALID_ARGUMENTS
    - MAC_NOT_FOUND
    - COMMAND_FAILED

Examples:
  $(basename "$0") aa:bb:cc:dd:ee:ff eth0
  $(basename "$0") aa:bb:cc:dd:ee:ff
  $(basename "$0") -h

EOF
}

# Helper function to build JSON error output
# Args: $1=error_code, $2=error_message, $3=MAC address (optional)
build_error_json() {
  local error_code="$1"
  local error_msg="$2"
  local mac="$3"

  # Build JSON using jq
  jq -n \
    --arg mac "$mac" \
    --arg error_code "$error_code" \
    --arg error "$error_msg" \
    '(if $mac != "" then {mac: $mac} else {} end) +
     {error_code: $error_code, error: $error}'
}

# Helper function to build JSON output for successful MAC-to-IP resolution
# Args: $1=MAC address, $2=interface (optional), $3=IPv4 address (optional), $4=IPv6 address (optional)
build_success_json() {
  local mac="$1"
  local interface="$2"
  local ipv4="$3"
  local ipv6="$4"

  # Build JSON using jq
  jq -n \
    --arg mac "$mac" \
    --arg interface "$interface" \
    --arg ipv4 "$ipv4" \
    --arg ipv6 "$ipv6" \
    '{mac: $mac} +
     (if $interface != "" then {interface: $interface} else {} end) +
     (if $ipv4 != "" then {ipv4: $ipv4} else {} end) +
     (if $ipv6 != "" then {ipv6: $ipv6} else {} end)'
}

# Get IPv4 broadcast address from local interface configs or device (if specified)
get_ipv4_broadcast() {
  local target_intf="$1"
  local broadcast_ip=""
  local dev_option=""

  if [ -n "$target_intf" ]; then
    dev_option="dev $target_intf"
  fi

  # Capture both stdout and stderr
  local tmp_output="/tmp/getip_broadcast_$$"
  local tmp_error="/tmp/getip_broadcast_err_$$"
  local cmd="ip -4 addr show ${dev_option}"

  ip -4 addr show "${dev_option}" >"$tmp_output" 2>"$tmp_error"
  local exit_code=$?

  if [ $exit_code -ne 0 ]; then
    # Command failed, return error in JSON format
    local error_msg=""
    error_msg=$(cat "$tmp_error" 2>/dev/null || echo "Failed to get IPv4 broadcast address")
    rm -f "$tmp_output" "$tmp_error"
    build_error_json "COMMAND_FAILED" "Command '$cmd' failed: $error_msg"
    return 1
  fi

  broadcast_ip=$(grep -oP 'brd \K[\d.]+' "$tmp_output" | head -n 1)
  rm -f "$tmp_output" "$tmp_error"

  echo "$broadcast_ip"
}

# Get link-local multicast address for IPv6
get_ipv6_multicast() {
  # Use all-nodes multicast address
  echo "ff02::1"
}

# Helper function to get IP from neighbor table for a given MAC address
# Args: $1=IP version (4 or 6), $2=MAC address, $3=dev_option (optional)
get_ip_from_neighbor() {
  local ip_version="$1"
  local target_mac="$2"
  local dev_option="$3"

  # Capture both stdout and stderr
  local tmp_output="/tmp/getip_neighbor_${ip_version}_$$"
  local tmp_error="/tmp/getip_neighbor_err_${ip_version}_$$"
  local cmd="ip -${ip_version} neighbor show ${dev_option}"

  ip -"${ip_version}" neighbor show "${dev_option}" >"$tmp_output" 2>"$tmp_error"
  local exit_code=$?

  if [ $exit_code -ne 0 ]; then
    # Command failed, return error in JSON format
    local error_msg=""
    error_msg=$(cat "$tmp_error" 2>/dev/null || echo "Failed to query IP neighbor table")
    rm -f "$tmp_output" "$tmp_error"
    build_error_json "COMMAND_FAILED" "Command '$cmd' failed: $error_msg"
    return 1
  fi

  local result=""
  result=$(grep -i "lladdr $target_mac" "$tmp_output" | awk '{print $1}' | head -n 1)
  rm -f "$tmp_output" "$tmp_error"

  echo "$result"
}

# Helper function to ping an IP address with optional interface
# Args: $1=IP address, $2=interface (optional), $3=additional options (optional)
ping_ip() {
  local ip_addr="$1"
  local target_intf="$2"
  local extra_options="$3"
  local ping_cmd="ping"
  local ping_options="-c 1 -w 1 -q"

  # Determine if IPv6 based on presence of colon in IP
  if [[ $ip_addr =~ : ]]; then
    ping_cmd="ping6"
  fi

  # Add extra options if provided (e.g., -b for broadcast)
  if [ -n "$extra_options" ]; then
    ping_options="$extra_options $ping_options"
  fi

  # Add interface option if provided
  if [ -n "$target_intf" ]; then
    ping_options="$ping_options -I $target_intf"
  fi

  # Ping the IP address. Suppress output and errors.
  # shellcheck disable=SC2086  # $ping_options intentionally unquoted for word splitting
  "$ping_cmd" $ping_options "$ip_addr" >/dev/null 2>&1
}

# Check if an IP is IPv6
# shellcheck disable=SC2317  # Used in subsequent diffs
is_ipv6() {
  local ip="$1"
  [[ $ip =~ : ]]
}

get_ip_from_mac() {
  local target_mac="$1"
  local target_intf="$2" # Optional interface argument

  # Build device option for ip commands
  local dev_option=""
  if [ -n "$target_intf" ]; then
    dev_option="dev $target_intf"
  fi

  # Step 1: Check the neighbor table for existing entries (both IPv4 and IPv6)
  # Check for IPv4 entry
  local existing_ipv4=""
  existing_ipv4=$(get_ip_from_neighbor 4 "$target_mac" "$dev_option")

  # Check if IPv4 query returned an error
  if echo "$existing_ipv4" | grep -q '"error_code"'; then
    echo "$existing_ipv4"
    return 1
  fi

  # Check for IPv6 entry
  local existing_ipv6=""
  existing_ipv6=$(get_ip_from_neighbor 6 "$target_mac" "$dev_option")

  # Check if IPv6 query returned an error
  if echo "$existing_ipv6" | grep -q '"error_code"'; then
    echo "$existing_ipv6"
    return 1
  fi

  if [ -n "$existing_ipv4" ] || [ -n "$existing_ipv6" ]; then
    # Entry exists, ping the specific IP(s) to verify the MAC-IP mapping
    [ -n "$existing_ipv4" ] && ping_ip "$existing_ipv4" "$target_intf"
    [ -n "$existing_ipv6" ] && ping_ip "$existing_ipv6" "$target_intf"
  else
    # Entry doesn't exist, ping the broadcast/multicast addresses

    # Ping IPv4 broadcast if we have one
    local broadcast_ipv4=""
    broadcast_ipv4=$(get_ipv4_broadcast "$target_intf")

    # Check if broadcast query returned an error
    if echo "$broadcast_ipv4" | grep -q '"error_code"'; then
      echo "$broadcast_ipv4"
      return 1
    fi

    [ -n "$broadcast_ipv4" ] && ping_ip "$broadcast_ipv4" "$target_intf" "-b"

    # Ping IPv6 multicast address
    local multicast_ipv6=""
    multicast_ipv6=$(get_ipv6_multicast)
    ping_ip "$multicast_ipv6" "$target_intf"
  fi

  # Wait a moment for the neighbor table to update
  sleep 1

  # Step 2: Check the neighbor table again and return all IPs which match the MAC
  # Get IPv4 address
  local ipv4_addr=""
  ipv4_addr=$(get_ip_from_neighbor 4 "$target_mac" "$dev_option")

  # Check if IPv4 query returned an error
  if echo "$ipv4_addr" | grep -q '"error_code"'; then
    echo "$ipv4_addr"
    return 1
  fi

  # Get IPv6 address
  local ipv6_addr=""
  ipv6_addr=$(get_ip_from_neighbor 6 "$target_mac" "$dev_option")

  # Check if IPv6 query returned an error
  if echo "$ipv6_addr" | grep -q '"error_code"'; then
    echo "$ipv6_addr"
    return 1
  fi

  # Build and return JSON output
  build_success_json "$target_mac" "$target_intf" "$ipv4_addr" "$ipv6_addr"
}

# Parse arguments
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
  print_usage
  exit 0
fi

if [ -z "$1" ]; then
  build_error_json "INVALID_ARGUMENTS" "MAC address argument required. Use -h for help."
  exit 2
fi

# Get the IP address for the provided MAC address (and optional interface)
result_json=$(get_ip_from_mac "$@")

# Parse the JSON to check if an IP was found or if there's an error
if echo "$result_json" | grep -q '"error_code"'; then
  # Already has an error (e.g., COMMAND_FAILED), pass it through
  echo "$result_json"
  exit 1
elif echo "$result_json" | grep -qE '"ipv4"|"ipv6"'; then
  # Success case - found at least one IP address
  echo "$result_json"
  exit 0
else
  # No IP found and no error - this is MAC_NOT_FOUND
  if [ -n "$2" ]; then
    error_msg="MAC address $1 not found in ip neighbor table on interface $2."
  else
    error_msg="MAC address $1 not found in ip neighbor table."
  fi

  build_error_json "MAC_NOT_FOUND" "$error_msg" "$1"
  exit 1
fi
