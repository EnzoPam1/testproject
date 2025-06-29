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
                print(f"Connected to server at {self.server_address}")
            except ConnectionRefusedError:
                print(f"Connection refused to {self.server_address}")
                return
            
            # Receive welcome message
            welcome_msg = self._recv_line()
            if not welcome_msg.startswith("WELCOME"):
                print(f"Expected WELCOME, got: {welcome_msg}")
                return
            
            print(f"Received: {welcome_msg.strip()}")
            
            # Send team name
            self.client_socket.send(f"{self.team_name}\n".encode())
            print(f"Sent team name: {self.team_name}")

            # Read client number (available slots)
            client_num_line = self._recv_line()
            try:
                self.available_slots = int(client_num_line.strip())
                print(f"Available slots: {self.available_slots}")
            except ValueError:
                print(f"Error parsing available slots: '{client_num_line}'")
                return

            # Read world dimensions
            dimensions_line = self._recv_line()
            try:
                dimensions = dimensions_line.strip().split()
                if len(dimensions) == 2:
                    self.world_width, self.world_height = [int(x) for x in dimensions]
                    print(f"World dimensions: {self.world_width}x{self.world_height}")
                else:
                    print(f"Unexpected dimensions format: {dimensions_line}")
                    return
            except ValueError as e:
                print(f"Error parsing world dimensions: {e}")
                return

            # Initialize game logic only if connection is successful
            if self.available_slots > 0:
                self.game_logic = GameLogic(
                    self.team_name, self.server_address[0], self.server_address[1]
                )
                self.is_connected = True
                print("AI initialized successfully")
            else:
                print("No available slots for this team")
                return
        
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
                continue
            except Exception as e:
                print(f"Network error: {e}")
                break
                    
            with self.data_lock:
                if "\n" in self.message_buffer:
                    self.message_ready.set()
    
    def _recv_line(self):
        """Receive a complete line from socket."""
        line = ""
        while not line.endswith('\n'):
            try:
                data = self.client_socket.recv(1)
                if not data:
                    break
                line += data.decode()
            except socket.timeout:
                continue
        return line
    
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
        try:
            self.client_socket.send(f"{command}\n".encode())
            print(f"Sent: {command}")
        except Exception as e:
            print(f"Error sending command '{command}': {e}")
    
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
                print("Not connected to server")
                return
        
        print("Starting AI main loop...")
        
        received_responses = []
        broadcast_messages = []
        pending_commands = []
        commands_sent = 0
        max_commands_in_flight = 10
        
        while self.is_running:
            try:
                # Process responses for sent commands
                while commands_sent > 0:
                    # Get message from server
                    message = None
                    if self.message_ready.is_set():
                        message = self.wait_for_message()
                    else:
                        temp_message = self.get_next_message()
                        if temp_message is None:
                            # Wait a bit for a message
                            self.client_socket.settimeout(0.1)
                            try:
                                data = self.client_socket.recv(1024)
                                if data:
                                    with self.data_lock:
                                        self.message_buffer += data.decode()
                                    if "\n" in self.message_buffer:
                                        message = self.get_next_message()
                            except socket.timeout:
                                break
                            except Exception as e:
                                print(f"Error receiving data: {e}")
                                break
                        else:
                            message = temp_message
                    
                    if message is None:
                        break
                        
                    print(f"Received: {message}")
                    
                    # Handle broadcast messages
                    if message.startswith("message "):
                        broadcast_messages.append(message[8:])  # Remove "message " prefix
                    elif "Elevation underway" in message:
                        received_responses.append("Incantation|" + message)
                        commands_sent -= 1
                    elif "Current level" in message:
                        received_responses.append("Incantation|" + message)
                        commands_sent -= 1
                    elif message == "dead":
                        print("Player died!")
                        self.is_running = False
                        return
                    else:
                        # Regular command response
                        if pending_commands:
                            command_sent = pending_commands.pop(0)
                            received_responses.append(command_sent + "|" + message)
                            commands_sent -= 1
                
                # Get new commands from game logic if we have processed all responses
                if commands_sent == 0 and self.game_logic:
                    new_commands = self.game_logic.process_turn(received_responses, broadcast_messages)
                    pending_commands.extend(new_commands)
                    received_responses = []
                    broadcast_messages = []
                
                # Send up to max_commands_in_flight commands
                commands_to_send = min(len(pending_commands), max_commands_in_flight - commands_sent)
                for i in range(commands_to_send):
                    command = pending_commands[i]
                    self.send_command(command)
                    commands_sent += 1
                    
                    # For incantation, we expect an immediate response
                    if command == "Incantation":
                        break
                
                # Remove sent commands from pending list
                pending_commands = pending_commands[commands_to_send:]
                
                # Small delay to prevent busy waiting
                sleep(0.01)
                
            except KeyboardInterrupt:
                print("Interrupted by user")
                break
            except Exception as e:
                print(f"Error in main loop: {e}")
                break
        
        print("AI stopped")
        self.is_running = False