##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## network_handler
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
                self.client_socket.connect(self.server_address)
            except ConnectionRefusedError:
                return
            
            # Receive welcome message
            response = self.client_socket.recv(1024)
            if response != b"WELCOME\n":
                return
            
            # Send team name and get world info
            self.client_socket.send(f"{self.team_name}\n".encode())

            # Read response line by line to ensure complete message
            response_buffer = ""
            while "\n" not in response_buffer:
                data = self.client_socket.recv(1024)
                if not data:
                    return
                response_buffer += data.decode()

            response = response_buffer.split("\n")[0].strip()
            print(f"Server response after team name: '{response}'")

            try:
                values = response.split()
                if len(values) == 3:
                    self.available_slots, self.world_width, self.world_height = [int(x) for x in values]
                    print(f"Parsed: slots={self.available_slots}, width={self.world_width}, height={self.world_height}")
                else:
                    print(f"Unexpected response format: {response} (got {len(values)} values)")
                    return
            except ValueError as e:
                print(f"Error parsing server response: {e}")
                return

            # Initialize game logic
            self.game_logic = GameLogic(
                self.team_name, self.server_address[0], self.server_address[1]
            )
            if self.available_slots >= 0:
                self.is_connected = True
        
        # Set socket timeout for non-blocking operation
        self.client_socket.settimeout(0.5)
        
        # Main network loop
        while self.is_running:
            try:
                data = self.client_socket.recv(1024)
                if not data:  # Connection closed
                    break
                
                # Add received data to buffer
                with self.data_lock:
                    self.message_buffer += data.decode()
                    
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
            return message
    
    def send_command(self, command):
        """Send a command to the server."""
        self.client_socket.send(f"{command}\n".encode())
    
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
                return
        
        received_responses = []
        broadcast_messages = []
        pending_commands = []
        
        while self.is_running:
            # Process responses for sent commands
            commands_processed = 0
            while commands_processed < min(len(pending_commands), 10):
                # Get message from server
                if self.message_ready.is_set():
                    server_message = self.wait_for_message()
                else:
                    temp_message = self.get_next_message()
                    if temp_message is None:
                        server_message = self.wait_for_message()
                    else:
                        server_message = temp_message
                
                # Handle broadcast messages
                if server_message.split(" ")[0] == "message":
                    broadcast_messages.append(server_message.split("message ")[1])
                elif "Elevation underway" in server_message:
                    received_responses.append("Incantation|" + server_message)
                elif "Current level" in server_message:
                    received_responses.append("Incantation|" + server_message)
                else:
                    # Regular command response
                    received_responses.append(pending_commands[commands_processed].split("\n", 1)[0] + "|" + server_message)
                    commands_processed += 1
            
            # Remove processed commands
            if len(pending_commands) > 10:
                pending_commands = pending_commands[10:]
            else:
                pending_commands = []
            
            # Get new commands from game logic
            if not pending_commands:
                pending_commands = self.game_logic.process_turn(received_responses, broadcast_messages)
                received_responses = []
                broadcast_messages = []
            
            # Send up to 10 commands
            commands_to_send = min(len(pending_commands), 10)
            for i in range(commands_to_send):
                self.send_command(pending_commands[i])
                
            # Remove incantation commands immediately after sending
            for i in range(commands_to_send):
                if "Incantation" in pending_commands[i]:
                    pending_commands.pop(i)
                    break
