# ipcalc.py
import sys
import json
import argparse
import re
from typing import Tuple, List, Optional

def ip_to_int(ip: str) -> int:
    parts = ip.split('.')
    return (int(parts[0]) << 24) | (int(parts[1]) << 16) | (int(parts[2]) << 8) | int(parts[3])

def int_to_ip(num: int) -> str:
    return f"{(num >> 24) & 0xff}.{(num >> 16) & 0xff}.{(num >> 8) & 0xff}.{num & 0xff}"

def prefix_to_mask(prefix: int) -> int:
    return (~0 << (32 - prefix)) & 0xffffffff

def mask_to_prefix(mask: int) -> int:
    return bin(mask).count('1')

def ip_binary(ip: int) -> str:
    return '.'.join(f"{((ip >> (24 - i*8)) & 0xff):08b}" for i in range(4))

class IPCalc:
    def __init__(self, ip_str: str, mask_str: Optional[str] = None):
        if '/' in ip_str:
            ip_part, prefix_str = ip_str.split('/')
            self.ip_int = ip_to_int(ip_part)
            self.prefix = int(prefix_str)
            self.mask_int = prefix_to_mask(self.prefix)
        elif mask_str:
            self.ip_int = ip_to_int(ip_str)
            if '.' in mask_str:
                self.mask_int = ip_to_int(mask_str)
                self.prefix = mask_to_prefix(self.mask_int)
            else:
                self.prefix = int(mask_str)
                self.mask_int = prefix_to_mask(self.prefix)
        else:
            raise ValueError("Invalid input")
        self.ip_str = int_to_ip(self.ip_int)
        self.mask_str = int_to_ip(self.mask_int)
        self.network_int = self.ip_int & self.mask_int
        self.broadcast_int = self.network_int | (~self.mask_int & 0xffffffff)
        self.wildcard_int = ~self.mask_int & 0xffffffff
        self.first_usable = self.network_int + 1 if self.network_int != self.broadcast_int else None
        self.last_usable = self.broadcast_int - 1 if self.network_int != self.broadcast_int else None
        self.total_hosts = 1 << (32 - self.prefix)
        self.usable_hosts = max(0, self.total_hosts - 2)

    def to_dict(self) -> dict:
        return {
            "ip": self.ip_str,
            "mask": self.mask_str,
            "prefix": self.prefix,
            "network": int_to_ip(self.network_int),
            "broadcast": int_to_ip(self.broadcast_int),
            "wildcard": int_to_ip(self.wildcard_int),
            "first_usable": int_to_ip(self.first_usable) if self.first_usable else None,
            "last_usable": int_to_ip(self.last_usable) if self.last_usable else None,
            "total_hosts": self.total_hosts,
            "usable_hosts": self.usable_hosts,
            "binary_ip": ip_binary(self.ip_int),
            "binary_mask": ip_binary(self.mask_int)
        }

    def subnet(self, new_prefix: int) -> List[dict]:
        if new_prefix <= self.prefix:
            raise ValueError("New prefix must be larger than original")
        num_subnets = 1 << (new_prefix - self.prefix)
        step = 1 << (32 - new_prefix)
        subnets = []
        for i in range(num_subnets):
            net = self.network_int + i * step
            sub = IPCalc(int_to_ip(net), str(new_prefix))
            subnets.append({
                "network": sub.ip_str,
                "mask": sub.mask_str,
                "prefix": sub.prefix,
                "broadcast": sub.broadcast_str,
                "first_usable": sub.first_usable_str,
                "last_usable": sub.last_usable_str
            })
        return subnets

    @property
    def broadcast_str(self): return int_to_ip(self.broadcast_int)
    @property
    def first_usable_str(self): return int_to_ip(self.first_usable) if self.first_usable else None
    @property
    def last_usable_str(self): return int_to_ip(self.last_usable) if self.last_usable else None

def main():
    parser = argparse.ArgumentParser(description="IP Calculator")
    parser.add_argument('input', help="IP address (e.g., 192.168.1.0/24 or 192.168.1.10 255.255.255.0)")
    parser.add_argument('mask', nargs='?', help="Subnet mask if not in CIDR")
    parser.add_argument('--subnet', type=int, help="Create subnets of this prefix length")
    parser.add_argument('--bin', action='store_true', help="Show binary representation")
    parser.add_argument('--json', action='store_true', help="Output as JSON")
    args = parser.parse_args()

    try:
        calc = IPCalc(args.input, args.mask)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    result = calc.to_dict()
    if args.bin:
        result['binary_network'] = ip_binary(calc.network_int)
        result['binary_broadcast'] = ip_binary(calc.broadcast_int)

    subnets = []
    if args.subnet:
        try:
            subnets = calc.subnet(args.subnet)
        except ValueError as e:
            print(f"Subnet error: {e}", file=sys.stderr)
            sys.exit(1)

    if args.json:
        output = result.copy()
        if subnets:
            output['subnets'] = subnets
        print(json.dumps(output, indent=2))
    else:
        print("\nIP Calculator")
        print(f"IP Address:     {result['ip']}")
        print(f"Subnet Mask:    {result['mask']} (/{result['prefix']})")
        print(f"Network:        {result['network']}")
        print(f"Broadcast:      {result['broadcast']}")
        print(f"Wildcard Mask:  {result['wildcard']}")
        if result['first_usable']:
            print(f"First Usable:   {result['first_usable']}")
            print(f"Last Usable:    {result['last_usable']}")
        else:
            print("First Usable:   (none)")
            print("Last Usable:    (none)")
        print(f"Total Hosts:    {result['total_hosts']}")
        print(f"Usable Hosts:   {result['usable_hosts']}")

        if args.bin:
            print("\nBinary:")
            print(f"IP:      {result['binary_ip']}")
            print(f"Mask:    {result['binary_mask']}")
            print(f"Network: {ip_binary(calc.network_int)}")

        if subnets:
            print(f"\nSubnets (/{args.subnet}):")
            for sub in subnets:
                first = sub['first_usable'] or "none"
                last = sub['last_usable'] or "none"
                print(f"  {sub['network']}/{sub['prefix']}   ({first} - {last})")

if __name__ == "__main__":
    main()
