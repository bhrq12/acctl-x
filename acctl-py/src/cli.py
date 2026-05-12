#!/usr/bin/python3
"""
AC Controller Command Line Interface
"""

import argparse
import sys
import os

# Add src directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from acctl.ap_manager import ap_manager
from acctl.config import config_manager
from acctl.db import db_manager


def cmd_ap_list(args):
    """List all APs."""
    aps = ap_manager.get_all_aps()
    if not aps:
        print("No APs registered")
        return
    
    print(f"{'MAC Address':<18} {'Name':<20} {'Status':<10} {'Clients':<8}")
    print("-" * 60)
    for ap in aps:
        print(f"{ap.mac:<18} {ap.name:<20} {ap.status:<10} {ap.clients:<8}")


def cmd_ap_show(args):
    """Show AP details."""
    ap = ap_manager.get_ap_status(args.mac)
    if not ap:
        print(f"AP {args.mac} not found")
        return
    
    print(f"MAC Address:     {ap.mac}")
    print(f"Name:            {ap.name}")
    print(f"Model:           {ap.model}")
    print(f"Firmware:        {ap.firmware_version}")
    print(f"IP Address:      {ap.ip_address}")
    print(f"Status:          {ap.status}")
    print(f"Uptime:          {ap.uptime} seconds")
    print(f"Clients:         {ap.clients}")
    print(f"Registered At:   {ap.registered_at}")
    print(f"Last Seen:       {ap.last_seen}")


def cmd_ap_config(args):
    """Configure AP."""
    ap = ap_manager.get_ap_status(args.mac)
    if not ap:
        print(f"AP {args.mac} not found")
        return
    
    update_data = {}
    if args.name:
        update_data['name'] = args.name
    
    if update_data:
        ap_manager.register_ap(args.mac, update_data)
        print(f"AP {args.mac} configured successfully")
    else:
        print("No configuration changes specified")


def cmd_ap_restart(args):
    """Restart AP."""
    ap = ap_manager.get_ap_status(args.mac)
    if not ap:
        print(f"AP {args.mac} not found")
        return
    
    print(f"Restart command sent to {args.mac}")
    # TODO: Implement actual restart logic


def cmd_ap_upgrade(args):
    """Upgrade AP firmware."""
    ap = ap_manager.get_ap_status(args.mac)
    if not ap:
        print(f"AP {args.mac} not found")
        return
    
    if not args.firmware:
        print("Firmware path required")
        return
    
    print(f"Upgrading {args.mac} with firmware {args.firmware}")
    # TODO: Implement actual firmware upgrade logic


def cmd_status(args):
    """Show AC status."""
    aps = ap_manager.get_all_aps()
    online_count = sum(1 for ap in aps if ap.status == 'online')
    
    print("AC Controller Status")
    print("-" * 30)
    print(f"Total APs:       {len(aps)}")
    print(f"Online APs:      {online_count}")
    print(f"Offline APs:     {len(aps) - online_count}")
    
    config = config_manager.get_config()
    print(f"\nAC UUID:         {config.get('global', {}).get('uuid', 'N/A')}")
    print(f"AC Name:         {config.get('global', {}).get('name', 'N/A')}")


def cmd_reload(args):
    """Reload configuration."""
    config_manager.load_config()
    print("Configuration reloaded")


def cmd_logs(args):
    """Show recent logs."""
    logs = db_manager.get_logs(args.limit)
    for log in logs:
        print(f"[{log['timestamp']}] [{log['level']}] {log['message']}")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="AC Controller CLI")
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # AP commands
    ap_parser = subparsers.add_parser('ap', help='AP management commands')
    ap_subparsers = ap_parser.add_subparsers(dest='ap_command')
    
    ap_subparsers.add_parser('list', help='List all APs').set_defaults(func=cmd_ap_list)
    
    show_parser = ap_subparsers.add_parser('show', help='Show AP details')
    show_parser.add_argument('mac', help='AP MAC address')
    show_parser.set_defaults(func=cmd_ap_show)
    
    config_parser = ap_subparsers.add_parser('config', help='Configure AP')
    config_parser.add_argument('mac', help='AP MAC address')
    config_parser.add_argument('--name', help='AP name')
    config_parser.set_defaults(func=cmd_ap_config)
    
    restart_parser = ap_subparsers.add_parser('restart', help='Restart AP')
    restart_parser.add_argument('mac', help='AP MAC address')
    restart_parser.set_defaults(func=cmd_ap_restart)
    
    upgrade_parser = ap_subparsers.add_parser('upgrade', help='Upgrade AP firmware')
    upgrade_parser.add_argument('mac', help='AP MAC address')
    upgrade_parser.add_argument('--firmware', help='Firmware file path')
    upgrade_parser.set_defaults(func=cmd_ap_upgrade)
    
    # Other commands
    subparsers.add_parser('status', help='Show AC status').set_defaults(func=cmd_status)
    subparsers.add_parser('reload', help='Reload configuration').set_defaults(func=cmd_reload)
    
    logs_parser = subparsers.add_parser('logs', help='Show recent logs')
    logs_parser.add_argument('--limit', type=int, default=50, help='Number of logs to show')
    logs_parser.set_defaults(func=cmd_logs)
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    if hasattr(args, 'func'):
        args.func(args)
    else:
        if args.command == 'ap':
            ap_parser.print_help()
        else:
            parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
