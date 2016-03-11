# @lint-avoid-python-3-compatibility-imports

from fboss.utils import COLOR_RED, COLOR_GREEN, COLOR_RESET

def convert_bps(bps):
    if bps < 10 ** 3:
        return (bps, 'bps')
    elif bps < 10 ** 6:
        return (bps / (10 ** 3), 'Kbps')
    elif bps < 10 ** 9:
        return (bps / (10 ** 6), 'Mbps')
    else:
        return (bps / (10 ** 9), 'Gbps')

def print_port_counters(client, port_id):
    '''Display port error counters all.'''

    desired_counters = [
        'in_bytes.sum',
        'in_discards.sum',
        'in_errors.sum',
        'out_bytes.sum',
        'out_discards.sum',
        'out_errors.sum',
    ]
    full_counter_list = []
    port_prefix = 'port%d.' % port_id
    for name in desired_counters:
        for suffix in ('.60', '.600', '.3600'):
            full_counter_list.append(port_prefix + name + suffix)

    port_counters = client.getSelectedCounters(full_counter_list)

    fmt = '{:<20} {}{:>12}{} {}{:>12}{} {}{:>12}{}'

    def bps(bytes_per_minute, interval):
        value, suffix = convert_bps((bytes_per_minute * 8) / interval)
        return (None, '{:.02f}{}'.format(value, suffix))

    def error_ctr(value, interval):
        color = COLOR_GREEN
        if value > 0:
            color = COLOR_RED
        return color, value

    def print_counter(header, name, transform):
        intervals = (60, 600, 3600)
        suffixes = ('.{}'.format(i) for i in intervals)
        values = [port_counters[port_prefix + name + s] for s in suffixes]

        fmt_args = [header]
        for v, i in zip(values, intervals):
            color, value_str = transform(v, i)
            if color is None:
                color_start = ''
                color_end = ''
            else:
                color_start = color
                color_end = COLOR_RESET
            fmt_args.extend([color_start, value_str, color_end])

        print(fmt.format(*fmt_args))

    print(fmt.format('    Time Interval:',
                     '', '1 minute', '',
                     '', '10 minutes', '',
                     '', '1 hour', ''))
    print_counter('Ingress', 'in_bytes.sum', bps)
    print_counter('Egress', 'out_bytes.sum', bps)
    print_counter('In Errors', 'in_errors.sum', error_ctr)
    print_counter('In Discards', 'in_discards.sum', error_ctr)
    print_counter('Out Errors', 'out_errors.sum', error_ctr)
    print_counter('Out Discards', 'out_discards.sum', error_ctr)

def print_port_details(client, port_id, port_info=None):
    if port_info is None:
        port_info = client.getPortInfo(port_id)

    admin_status = "DISABLED"
    if port_info.adminState:
        admin_status = "ENABLED"

    oper_status = "DOWN"
    if port_info.operState:
        oper_status = "UP"

    speed, suffix = convert_bps(port_info.speedMbps * (10 ** 6))
    vlans = ' '.join(str(vlan) for vlan in port_info.vlans)

    fmt = '{:.<50}{}'
    lines = [
        ('Interface', port_info.name.strip()),
        ('Port ID', str(port_info.portId)),
        ('Admin State', admin_status),
        ('Link State', oper_status),
        ('Speed', '{:g} {}'.format(speed, suffix)),
        ('VLANs', vlans)
    ]

    print()
    print('\n'.join(fmt.format(*l) for l in lines))
    print('Description'.ljust(20, '.') + port_info.description)

    print_port_counters(client, port_id)
