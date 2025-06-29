# MODIFICATION network_handler.py - Version avec logs debug complets

##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## network_handler avec debug complet
##

import socket
import threading
import sys
import os
from time import sleep

sys.path.insert(0, os.path.dirname(__file__))

from game_logic import GameLogic

class NetworkHandler:
    def __init__(self, team_name, port, host="localhost"):
        """Initialize the network handler."""
        self.game_logic = None
        
        # Team data
        self.team_name = team_name
        self.world_width = 0
        self.world_height = 0
        
        # Network configuration
        self.server_address = (host, port)
        self.network_thread = threading.Thread(target=self._network_task)
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.available_slots = 0
        
        # Thread synchronization
        self.data_lock = threading.Lock()
        self.is_connected = False
        self.is_running = True
        self.message_buffer = ""
        self.message_ready = threading.Event()
        
        # Start network thread
        self.network_thread.start()
    
    def _network_task(self):
        """Network thread main task."""
        # Connect to server
        with self.data_lock:
            try:
                print(f"[DEBUG] Connecting to {self.server_address}")
                self.client_socket.connect(self.server_address)
                print(f"[DEBUG] Connected successfully")
            except ConnectionRefusedError:
                print(f"[ERROR] Connection refused to {self.server_address}")
                return
            
            # Receive welcome message
            response = self.client_socket.recv(1024)
            print(f"[DEBUG] Received welcome: {response}")
            if response != b"WELCOME\n":
                print(f"[ERROR] Expected WELCOME, got: {response}")
                return
            
            # Send team name and get world info
            team_msg = f"{self.team_name}\n"
            print(f"[DEBUG] Sending team name: {team_msg.strip()}")
            self.client_socket.send(team_msg.encode())
            
            response = self.client_socket.recv(1024)
            print(f"[DEBUG] Received world info: {response}")
            try:
                response_str = response.decode().strip()
                parts = response_str.split()
                print(f"[DEBUG] Parsing world info parts: {parts}")
                self.available_slots = int(parts[0])
                self.world_width = int(parts[1]) 
                self.world_height = int(parts[2])
                print(f"[DEBUG] Parsed: slots={self.available_slots}, width={self.world_width}, height={self.world_height}")
            except (ValueError, IndexError) as e:
                print(f"[ERROR] Failed to parse world info '{response_str}': {e}")
                return
            
            # Initialize game logic
            self.game_logic = GameLogic(
                self.team_name, self.server_address[0], self.server_address[1]
            )
            if self.available_slots >= 0:
                self.is_connected = True
                print(f"[DEBUG] Game logic initialized, ready to start")
        
        # Set socket timeout for non-blocking operation
        self.client_socket.settimeout(0.5)
        
        # Main network loop
        while self.is_running:
            try:
                data = self.client_socket.recv(1024)
                if not data:  # Connection closed
                    print(f"[DEBUG] Connection closed by server")
                    break
                
                # Add received data to buffer
                with self.data_lock:
                    received = data.decode()
                    print(f"[DEBUG] Raw received: {repr(received)}")
                    self.message_buffer += received
                    
            except socket.timeout:
                if not self.is_running:
                    return
                    
            with self.data_lock:
                if "\n" in self.message_buffer:
                    self.message_ready.set()
    
    def get_next_message(self):
        """Get the next complete message from buffer."""
        with self.data_lock:
            if "\n" not in self.message_buffer:
                return None
            message = self.message_buffer.split("\n", 1)[0]
            self.message_buffer = self.message_buffer[len(message) + 1:]
            print(f"[DEBUG IA] Extracted message: '{message}'")
            return message
    
    def send_command(self, command):
        """Send a command to the server."""
        cmd_msg = f"{command}\n"
        print(f"[DEBUG IA] Sending command: '{command}'")
        self.client_socket.send(cmd_msg.encode())
    
    def get_message_count(self):
        """Get number of complete messages in buffer."""
        with self.data_lock:
            count = len(self.message_buffer.split("\n")) - 1
        return count
    
    def wait_for_message(self):
        """Wait for a message to be available."""
        self.message_ready.wait()
        self.message_ready.clear()
        return self.get_next_message()
    
    def __del__(self):
        """Cleanup network handler."""
        self.is_running = False
        if threading.current_thread() != self.network_thread:
            self.network_thread.join()
        self.client_socket.close()
    
    def start(self):
        """Start the main AI loop."""
        # Check connection
        with self.data_lock:
            if not self.is_connected:
                print(f"[ERROR] Not connected, cannot start AI loop")
                return
        
        print(f"[DEBUG] Starting AI main loop")
        received_responses = []
        broadcast_messages = []
        pending_commands = []
        turn_count = 0
        
        while self.is_running:
            turn_count += 1
            print(f"\n[DEBUG] ===== TURN {turn_count} =====")
            print(f"[DEBUG] Pending commands: {len(pending_commands)}")
            print(f"[DEBUG] Received responses: {len(received_responses)}")
            print(f"[DEBUG] Broadcast messages: {len(broadcast_messages)}")
            
            # Process responses for sent commands
            commands_processed = 0
            while commands_processed < min(len(pending_commands), 10):
                print(f"[DEBUG] Waiting for response {commands_processed + 1}")
                
                # Get message from server
                if self.message_ready.is_set():
                    server_message = self.wait_for_message()
                else:
                    temp_message = self.get_next_message()
                    if temp_message is None:
                        print(f"[DEBUG] No message ready, waiting...")
                        server_message = self.wait_for_message()
                    else:
                        server_message = temp_message
                
                print(f"[DEBUG] Processing server message: '{server_message}'")
                
                # Handle broadcast messages
                if server_message and server_message.split(" ")[0] == "message":
                    print(f"[DEBUG] Broadcast message detected")
                    broadcast_messages.append(server_message.split("message ")[1])
                elif server_message and "Elevation underway" in server_message:
                    print(f"[DEBUG] Elevation underway detected")
                    received_responses.append("Incantation|" + server_message)
                elif server_message and "Current level" in server_message:
                    print(f"[DEBUG] Current level detected")
                    received_responses.append("Incantation|" + server_message)
                else:
                    # Regular command response
                    if commands_processed < len(pending_commands):
                        cmd_name = pending_commands[commands_processed].split()[0]
                        response_pair = f"{cmd_name}|{server_message}"
                        print(f"[DEBUG] Command response: {response_pair}")
                        received_responses.append(response_pair)
                        commands_processed += 1
                    else:
                        print(f"[WARNING] Unexpected message: {server_message}")
            
            # Remove processed commands
            if len(pending_commands) > 10:
                pending_commands = pending_commands[10:]
                print(f"[DEBUG] Removed 10 commands, {len(pending_commands)} remaining")
            else:
                pending_commands = []
                print(f"[DEBUG] All commands processed")
            
            # Get new commands from game logic
            if not pending_commands:
                print(f"[DEBUG] Getting new commands from game logic")
                try:
                    new_commands = self.game_logic.process_turn(received_responses, broadcast_messages)
                    pending_commands = new_commands if new_commands else []
                    print(f"[DEBUG] Game logic returned {len(pending_commands)} commands: {pending_commands}")
                except Exception as e:
                    print(f"[ERROR] Game logic failed: {e}")
                    import traceback
                    traceback.print_exc()
                    break
                    
                received_responses = []
                broadcast_messages = []
            
            # Send up to 10 commands
            commands_to_send = min(len(pending_commands), 10)
            print(f"[DEBUG] Sending {commands_to_send} commands")
            for i in range(commands_to_send):
                print(f"[DEBUG] Sending command {i+1}: {pending_commands[i]}")
                self.send_command(pending_commands[i])
                
            # Remove incantation commands immediately after sending
            for i in range(commands_to_send):
                if "Incantation" in pending_commands[i]:
                    print(f"[DEBUG] Removing incantation command from queue")
                    pending_commands.pop(i)
                    break
            
            print(f"[DEBUG] Turn {turn_count} completed")
            sleep(0.1)  # Small delay to avoid overwhelming logs