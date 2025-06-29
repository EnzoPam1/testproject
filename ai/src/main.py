#!/usr/bin/env python3

import sys
import argparse
import signal
import time
from network_handler import NetworkHandler

def signal_handler(signum, frame):
    """Handle interrupt signals gracefully."""
    print(f"\nReceived signal {signum}, shutting down gracefully...")
    sys.exit(0)

def parse_arguments():
    """Parse command line arguments with improved validation."""
    parser = argparse.ArgumentParser(
        prog='./zappy_ai',
        description='Zappy AI Client - Enhanced Version', 
        add_help=False
    )
    parser.add_argument('-p', '--port', type=int, required=True, 
                       help='Port number (1-65535)')
    parser.add_argument('-n', '--name', type=str, required=True, 
                       help='Team name')
    parser.add_argument('-h', '--host', type=str, default='localhost', 
                       help='Server hostname (default: localhost)')
    parser.add_argument('--help', action='help', 
                       help='Show this help message and exit')
    parser.add_argument('--debug', action='store_true', 
                       help='Enable debug output')
    parser.add_argument('--timeout', type=int, default=300, 
                       help='Connection timeout in seconds (default: 300)')
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.port < 1 or args.port > 65535:
        parser.error("Port must be between 1 and 65535")
    
    if not args.name or len(args.name.strip()) == 0:
        parser.error("Team name cannot be empty")
    
    if len(args.name) > 50:
        parser.error("Team name is too long (max 50 characters)")
    
    # Clean team name
    args.name = args.name.strip()
    
    return args

def validate_environment():
    """Validate the runtime environment."""
    # Check Python version
    if sys.version_info < (3, 6):
        print("Error: Python 3.6 or higher is required")
        return False
    
    # Check required modules
    try:
        import socket
        import threading
        import select
        import time
    except ImportError as e:
        print(f"Error: Missing required module: {e}")
        return False
    
    return True

def main():
    """Enhanced main function with better error handling."""
    # Set up signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        # Validate environment
        if not validate_environment():
            sys.exit(1)
        
        # Parse arguments
        args = parse_arguments()
        
        if args.debug:
            print(f"Debug mode enabled")
            print(f"Connecting to {args.host}:{args.port} as team '{args.name}'")
        
        # Create and start the AI
        print(f"Zappy AI Client starting...")
        print(f"Team: {args.name}")
        print(f"Server: {args.host}:{args.port}")
        print(f"Timeout: {args.timeout}s")
        
        start_time = time.time()
        
        # Initialize network handler
        ai_client = NetworkHandler(args.name, args.port, args.host)
        
        # Start the main loop
        ai_client.start()
        
        # Calculate runtime
        runtime = time.time() - start_time
        print(f"AI client ran for {runtime:.2f} seconds")
        
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        sys.exit(0)
    except ConnectionRefusedError:
        print(f"Error: Could not connect to server at {args.host}:{args.port}")
        print("Make sure the server is running and accessible")
        sys.exit(1)
    except ConnectionError as e:
        print(f"Connection error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        if args.debug:
            import traceback
            traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()