##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## network_handler - CORRECTED VERSION
##

import socket
import threading
import sys
import os
from time import sleep
import logging

# Setup logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout)
    ]
)

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
        self.receive_buffer = b''
        self.message_ready = threading.Event()
        
        # Message queue for complete messages
        self.message_queue = []
        
        # Start network thread
        self.network_thread.start()
    
    def _network_task(self):
        """Network thread main task."""
        try:
            # Connect to server
            with self.data_lock:
                try:
                    logging.info(f"Connecting to {self.server_address[0]}:{self.server_address[1]}")
                    self.client_socket.connect(self.server_address)
                    logging.info("Connected to server")
                except ConnectionRefusedError:
                    logging.error("Connection refused by server")
                    return
                except Exception as e:
                    logging.error(f"Connection error: {e}")
                    return
                
                # Receive welcome message
                welcome_msg = self._receive_complete_message()
                if not welcome_msg or welcome_msg.strip() != "WELCOME":
                    logging.error(f"Invalid welcome message: '{welcome_msg}'")
                    return
                
                logging.info(f"Received welcome: '{welcome_msg}'")
                
                # Send team name
                self._send_message(self.team_name)
                logging.info(f"Sent team name: '{self.team_name}'")

                # Read client number response
                client_response = self._receive_complete_message()
                if not client_response:
                    logging.error("Failed to receive client number")
                    return
                
                logging.info(f"Received client response: '{client_response}'")
                
                try:
                    self.available_slots = int(client_response.strip())
                    if self.available_slots <= 0:
                        logging.error(f"No slots available: {self.available_slots}")
                        return
                except ValueError as e:
                    logging.error(f"Error parsing client number '{client_response}': {e}")
                    return

                # Read world dimensions
                world_response = self._receive_complete_message()
                if not world_response:
                    logging.error("Failed to receive world dimensions")
                    return
                
                logging.info(f"Received world response: '{world_response}'")
                
                try:
                    dimensions = world_response.strip().split()
                    if len(dimensions) == 2:
                        self.world_width, self.world_height = [int(x) for x in dimensions]
                        logging.info(f"World dimensions: {self.world_width}x{self.world_height}")
                    else:
                        logging.error(f"Invalid world dimensions format: {world_response}")
                        return
                except ValueError as e:
                    logging.error(f"Error parsing world dimensions '{world_response}': {e}")
                    return

                # Initialize game logic
                self.game_logic = GameLogic(
                    self.team_name, self.server_address[0], self.server_address[1]
                )
                
                self.is_connected = True
                logging.info("Initialization complete, starting main loop")
            
            # Set socket timeout for non-blocking operation
            self.client_socket.settimeout(0.5)
            
            # Main network loop
            while self.is_running:
                try:
                    data = self.client_socket.recv(1024)
                    if not data:  # Connection closed
                        logging.info("Server closed connection")
                        break
                    
                    # Add received data to buffer and process messages
                    with self.data_lock:
                        self.receive_buffer += data
                        self._process_buffer()
                        
                except socket.timeout:
                    # This is normal, just continue
                    continue
                except Exception as e:
                    logging.error(f"Network error: {e}")
                    break
                    
        except Exception as e:
            logging.error(f"Network task error: {e}")
        finally:
            self.is_running = False
            logging.info("Network task ended")

    def _receive_complete_message(self):
        """Receive one complete message (blocking)."""
        temp_buffer = b''
        
        while True:
            try:
                data = self.client_socket.recv(1024)
                if not data:
                    return None
                
                temp_buffer += data
                
                if b'\n' in temp_buffer:
                    line, remaining = temp_buffer.split(b'\n', 1)
                    # Put remaining back in main buffer
                    self.receive_buffer = remaining + self.receive_buffer
                    return line.decode('utf-8').strip()
                    
            except Exception as e:
                logging.error(f"Error receiving message: {e}")
                return None

    def _process_buffer(self):
        """Process the receive buffer and extract complete messages."""
        while b'\n' in self.receive_buffer:
            line, self.receive_buffer = self.receive_buffer.split(b'\n', 1)
            message = line.decode('utf-8').strip()
            
            if message:  # Only add non-empty messages
                self.message_queue.append(message)
                logging.debug(f"Queued message: '{message}'")
                self.message_ready.set()

    def _send_message(self, message):
        """Send a message with proper formatting."""
        try:
            # Ensure message doesn't already have newline
            message = message.strip()
            # Add newline and encode
            formatted_message = f"{message}\n".encode('utf-8')
            
            # Use sendall to ensure complete transmission
            self.client_socket.sendall(formatted_message)
            logging.debug(f"SENT: '{message}'")
            return True
        except Exception as e:
            logging.error(f"Error sending message '{message}': {e}")
            return False

    def get_next_message(self):
        """Get the next complete message from queue."""
        with self.data_lock:
            if self.message_queue:
                message = self.message_queue.pop(0)
                if not self.message_queue:
                    self.message_ready.clear()
                return message
            return None

    def send_command(self, command):
        """Send a command to the server."""
        return self._send_message(command)

    def get_message_count(self):
        """Get number of complete messages in queue."""
        with self.data_lock:
            return len(self.message_queue)

    def wait_for_message(self):
        """Wait for a message to be available."""
        self.message_ready.wait()
        return self.get_next_message()

    def __del__(self):
        """Cleanup network handler."""
        self.is_running = False
        if threading.current_thread() != self.network_thread:
            self.network_thread.join()
        try:
            self.client_socket.close()
        except:
            pass

    def start(self):
        """Start the main AI loop."""
        # Check connection
        with self.data_lock:
            if not self.is_connected:
                logging.error("Not connected to server")
                return
        
        logging.info("Starting AI main loop")
        
        received_responses = []
        broadcast_messages = []
        pending_commands = []
        
        while self.is_running:
            try:
                # Process responses for sent commands
                commands_processed = 0
                max_commands = min(len(pending_commands), 10)
                
                while commands_processed < max_commands:
                    # Get message from server
                    if self.message_ready.is_set():
                        server_message = self.wait_for_message()
                    else:
                        temp_message = self.get_next_message()
                        if temp_message is None:
                            # Wait for a message with timeout
                            self.message_ready.wait(timeout=1.0)
                            if not self.message_ready.is_set():
                                logging.debug("Timeout waiting for server response")
                                break
                            server_message = self.wait_for_message()
                        else:
                            server_message = temp_message
                    
                    if server_message is None:
                        logging.debug("No message received")
                        break
                    
                    logging.debug(f"Processing server message: '{server_message}'")
                    
                    # Handle broadcast messages
                    if server_message.startswith("message "):
                        broadcast_part = server_message[8:]  # Remove "message "
                        broadcast_messages.append(broadcast_part)
                        logging.debug(f"Added broadcast: '{broadcast_part}'")
                    elif "Elevation underway" in server_message:
                        received_responses.append("Incantation|" + server_message)
                        commands_processed += 1
                    elif "Current level" in server_message:
                        received_responses.append("Incantation|" + server_message)
                        commands_processed += 1
                    elif server_message.startswith("eject:"):
                        # Handle ejection message
                        logging.info(f"Ejected: {server_message}")
                        # This is not a command response, don't count it
                    else:
                        # Regular command response
                        if commands_processed < len(pending_commands):
                            command_sent = pending_commands[commands_processed].strip()
                            response_pair = f"{command_sent}|{server_message}"
                            received_responses.append(response_pair)
                            logging.debug(f"Command response: '{response_pair}'")
                            commands_processed += 1
                        else:
                            # Unexpected message
                            logging.warning(f"Unexpected server message: '{server_message}'")
                
                # Remove processed commands
                if len(pending_commands) > 10:
                    pending_commands = pending_commands[10:]
                else:
                    pending_commands = []
                
                # Get new commands from game logic
                if not pending_commands:
                    try:
                        new_commands = self.game_logic.process_turn(received_responses, broadcast_messages)
                        if new_commands:
                            pending_commands = new_commands
                            logging.debug(f"Got {len(pending_commands)} new commands")
                        received_responses = []
                        broadcast_messages = []
                    except Exception as e:
                        logging.error(f"Error in game logic: {e}")
                        sleep(1)
                        continue
                
                # Send up to 10 commands
                commands_to_send = min(len(pending_commands), 10)
                for i in range(commands_to_send):
                    if not self._send_message(pending_commands[i]):
                        logging.error(f"Failed to send command: {pending_commands[i]}")
                        break
                        
                # Remove incantation commands immediately after sending
                for i in range(commands_to_send):
                    if i < len(pending_commands) and "Incantation" in pending_commands[i]:
                        pending_commands.pop(i)
                        break
                
                # Small delay to prevent busy waiting
                sleep(0.1)
                
            except Exception as e:
                logging.error(f"Error in main loop: {e}")
                sleep(1)
        
        logging.info("AI main loop ended")