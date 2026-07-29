# bash completion for the fboss2 CLI (OSS).
#
# fboss2 exposes its compiled-in command tree via the hidden `__completion`
# subcommand: given the sub-commands typed so far, it prints the valid next
# sub-command names (one per line). Because the answer comes from the binary
# itself, completion never goes stale as commands are added.
#
# Install: copy to /etc/bash_completion.d/ or `source` it from your shell rc.
_fboss2_complete() {
  local cur completions
  cur="${COMP_WORDS[COMP_CWORD]}"
  completions="$(fboss2 __completion "${COMP_WORDS[@]:1:COMP_CWORD-1}" 2>/dev/null)"
  # shellcheck disable=SC2207
  COMPREPLY=($(compgen -W "${completions}" -- "${cur}"))
}
complete -F _fboss2_complete fboss2
