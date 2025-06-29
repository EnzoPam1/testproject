##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## main
##

#!/usr/bin/env python3

import sys
import argparse
from network_handler import NetworkHandler

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        prog='./zappy_ai',
        description='Zappy AI Client', 
        add_help=False
    )
    parser.add_argument('-p', '--port', type=int, required=True, help='Port number')
    parser.add_argument('-n', '--name', type=str, required=True, help='Team name')
    parser.add_argument('-h', '--host', type=str, default='localhost', help='Server hostname (default: localhost)')
    parser.add_argument('--help', action='help', help='Show this help message and exit')
    
    return parser.parse_args()

def main():
    """Main function."""
    try:
        args = parse_arguments()
        
        # Create and start the AI
        ai_client = NetworkHandler(args.name, args.port, args.host)
        ai_client.start()
        
    except KeyboardInterrupt:
        print("\nShutting down AI client...")
        sys.exit(0)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
