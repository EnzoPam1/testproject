##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## Improved network_handler with better buffering and error handling
##

import socket
import threading
import sys
import os
import select
import time
from collections import deque

sys.path.insert(0, os.path.dirname(__file__))

from game_logic import GameLogic

class CircularBuffer:
    """Circular buffer for efficient message handling."""
    
    def __init__(self, max_size=4096):
        self.buffer = bytearray(max_size)
        self.max_size = max_size
        self.start = 0
        self.end = 0
        self.size = 0
    
    def write(self, data: bytes) -> int:
        """Write data to buffer, returns bytes written."""
        if isinstance(data, str):
            data = data.encode()
        
        written = 0
        for byte in data:
            if self.size < self.max_size:
                self.buffer[self.end] = byte
                self.end = (self.end + 1) % self.max_size
                self.size += 1
                written += 1
            else:
                break
        return written
    
    def read_line(self) -> str:
        """Read a complete line from buffer."""
        if self.size == 0:
            return None
        
        # Look for newline
        current = self.start
        length = 0
        found_newline = False
        
        for _ in range(self.size):
            if self.buffer[current] == ord('\n'):
                found_newline = True
                length += 1
                break
            current = (current + 1) % self.max_size
            length += 1
        
        if not found_newline:
            return None
        
        # Extract the line
        line_bytes = bytearray()
        for _ in range(length):
            line_bytes.append(self.buffer[self.start])
            self.start = (self.start + 1) % self.max_size
            self.size -= 1
        
        return line_bytes.decode().rstrip('\r\n')
    
    def available_space(self) -> int:
        """Get available space in buffer."""
        return self.max_size - self.size

class NetworkHandler:
    def __init__(self, team_name, port, host="localhost"):
        """Initialize the network handler with improved architecture."""
        self.game_logic = None
        
        # Team data
        self.team_name = team_name
        self.world_width = 0
        self.world_height = 0
        
        # Network configuration
        self.server_address = (host, port)
        self.client_socket = None
        self.available_slots = 0
        
        # Improved buffering
        self.receive_buffer = CircularBuffer(8192)
        self.send_queue = deque(maxlen=100)
        self.response_queue = deque(maxlen=50)
        self.message_queue = deque(maxlen=50)
        
        # Thread synchronization
        self.data_lock = threading.Lock()
        self.is_connected = False
        self.is_running = True
        self.network_thread = None
        
        # Performance tracking
        self.last_activity = time.time()
        self.connection_attempts = 0
        self.max_connection_attempts = 5
        
        # Command tracking
        self.pending_commands = deque(maxlen=10)
        self.command_responses = {}
        self.command_timeout = 30.0  # seconds
        
        # Start network operations
        self._start_network()
    
    def _start_network(self):
        """Start network thread and establish connection."""
        self.network_thread = threading.Thread(target=self._network_task, daemon=True)
        self.network_thread.start()
    
    def _connect_to_server(self) -> bool:
        """Establish connection to server with retries."""
        for attempt in range(self.max_connection_attempts):
            try:
                self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.client_socket.settimeout(10.0)
                self.client_socket.connect(self.server_address)
                
                # Receive welcome message
                welcome = self.client_socket.recv(1024).decode().strip()
                if welcome != "WELCOME":
                    print(f"Unexpected welcome message: {welcome}")
                    self.client_socket.close()
                    continue
                
                # Send team name
                self.client_socket.send(f"{self.team_name}\n".encode())
                
                # Receive game info
                response_buffer = ""
                while "\n" not in response_buffer:
                    data = self.client_socket.recv(1024)
                    if not data:
                        raise ConnectionError("Server closed connection")
                    response_buffer += data.decode()
                
                response = response_buffer.split("\n")[0].strip()
                values = response.split()
                
                if len(values) == 3:
                    self.available_slots = int(values[0])
                    self.world_width = int(values[1])
                    self.world_height = int(values[2])
                    
                    if self.available_slots <= 0:
                        print("No available slots for team")
                        self.client_socket.close()
                        return False
                    
                    # Set socket to non-blocking mode
                    self.client_socket.setblocking(False)
                    
                    # Initialize game logic
                    self.game_logic = GameLogic(
                        self.team_name, self.server_address[0], self.server_address[1]
                    )
                    
                    print(f"Connected successfully: slots={self.available_slots}, "
                          f"world={self.world_width}x{self.world_height}")
                    return True
                else:
                    print(f"Invalid server response: {response}")
                    self.client_socket.close()
                    continue
                    
            except (socket.error, ConnectionError, ValueError) as e:
                print(f"Connection attempt {attempt + 1} failed: {e}")
                if self.client_socket:
                    self.client_socket.close()
                time.sleep(1)
        
        return False
    
    def _network_task(self):
        """Main network thread task with improved error handling."""
        # Establish connection
        if not self._connect_to_server():
            print("Failed to connect to server after all attempts")
            self.is_running = False
            return
        
        with self.data_lock:
            self.is_connected = True
        
        # Main network loop
        while self.is_running:
            try:
                # Use select for efficient I/O multiplexing
                ready_to_read, ready_to_write, in_error = select.select(
                    [self.client_socket], 
                    [self.client_socket] if self.send_queue else [], 
                    [self.client_socket], 
                    0.1  # 100ms timeout
                )
                
                # Handle incoming data
                if ready_to_read:
                    self._handle_incoming_data()
                
                # Handle outgoing data
                if ready_to_write and self.send_queue:
                    self._handle_outgoing_data()
                
                # Handle errors
                if in_error:
                    print("Socket error detected")
                    break
                
                # Process received messages
                self._process_received_messages()
                
                # Update activity timestamp
                self.last_activity = time.time()
                
            except socket.error as e:
                print(f"Network error: {e}")
                break
            except Exception as e:
                print(f"Unexpected error in network thread: {e}")
                break
        
        # Cleanup
        self._cleanup_connection()
    
    def _handle_incoming_data(self):
        """Handle incoming data from server."""
        try:
            data = self.client_socket.recv(4096)
            if not data:
                print("Server closed connection")
                self.is_running = False
                return
            
            # Write to circular buffer
            with self.data_lock:
                bytes_written = self.receive_buffer.write(data)
                if bytes_written < len(data):
                    print(f"Warning: Buffer overflow, lost {len(data) - bytes_written} bytes")
        
        except socket.error as e:
            if e.errno not in (socket.EAGAIN, socket.EWOULDBLOCK):
                print(f"Error receiving data: {e}")
                self.is_running = False
    
    def _handle_outgoing_data(self):
        """Handle outgoing data to server."""
        try:
            with self.data_lock:
                if not self.send_queue:
                    return
                
                command = self.send_queue.popleft()
            
            # Send command
            if not command.endswith('\n'):
                command += '\n'
            
            sent = self.client_socket.send(command.encode())
            if sent == 0:
                print("Could not send data to server")
                self.is_running = False
            else:
                # Track command for response matching
                command_name = command.strip().split()[0]
                timestamp = time.time()
                self.pending_commands.append((command_name, timestamp))
        
        except socket.error as e:
            if e.errno not in (socket.EAGAIN, socket.EWOULDBLOCK):
                print(f"Error sending data: {e}")
                self.is_running = False
    
    def _process_received_messages(self):
        """Process complete messages from receive buffer."""
        with self.data_lock:
            while True:
                line = self.receive_buffer.read_line()
                if line is None:
                    break
                
                # Classify message type
                if line.startswith("message "):
                    # Broadcast message
                    self.message_queue.append(line[8:])  # Remove "message " prefix
                elif line in ["ok", "ko", "dead"] or line.startswith("[") or "Current level:" in line:
                    # Command response
                    self.response_queue.append(line)
                else:
                    # Other server messages
                    self.response_queue.append(line)
    
    def send_command(self, command: str):
        """Send a command to the server."""
        with self.data_lock:
            if len(self.send_queue) < self.send_queue.maxlen:
                self.send_queue.append(command)
            else:
                print(f"Send queue full, dropping command: {command}")
    
    def get_next_response(self) -> str:
        """Get the next response from the server."""
        with self.data_lock:
            if self.response_queue:
                return self.response_queue.popleft()
        return None
    
    def get_next_message(self) -> str:
        """Get the next broadcast message."""
        with self.data_lock:
            if self.message_queue:
                return self.message_queue.popleft()
        return None
    
    def get_pending_command_count(self) -> int:
        """Get number of pending commands."""
        with self.data_lock:
            return len(self.send_queue)
    
    def is_connection_healthy(self) -> bool:
        """Check if connection is healthy."""
        current_time = time.time()
        return (self.is_connected and 
                self.is_running and 
                current_time - self.last_activity < 30.0)
    
    def _cleanup_connection(self):
        """Clean up network resources."""
        with self.data_lock:
            self.is_connected = False
            self.is_running = False
        
        if self.client_socket:
            try:
                self.client_socket.close()
            except:
                pass
    
    def start(self):
        """Start the main AI loop with improved error handling."""
        # Wait for connection
        max_wait = 30  # seconds
        wait_time = 0
        while not self.is_connected and wait_time < max_wait:
            time.sleep(0.1)
            wait_time += 0.1
        
        if not self.is_connected:
            print("Failed to establish connection within timeout")
            return
        
        print("Starting AI main loop...")
        
        received_responses = []
        broadcast_messages = []
        pending_commands = []
        last_command_time = time.time()
        
        while self.is_running and self.is_connection_healthy():
            try:
                current_time = time.time()
                
                # Collect responses for pending commands
                commands_processed = 0
                max_commands_to_process = min(len(pending_commands), 10)
                
                while commands_processed < max_commands_to_process:
                    response = self.get_next_response()
                    if response is None:
                        break
                    
                    # Match response to command
                    if commands_processed < len(pending_commands):
                        command_name = pending_commands[commands_processed].split()[0]
                        received_responses.append(f"{command_name}|{response}")
                        commands_processed += 1
                    else:
                        # Unmatched response
                        received_responses.append(f"Unknown|{response}")
                
                # Collect broadcast messages
                while True:
                    message = self.get_next_message()
                    if message is None:
                        break
                    broadcast_messages.append(message)
                
                # Remove processed commands
                pending_commands = pending_commands[commands_processed:]
                
                # Get new commands from game logic
                if not pending_commands or current_time - last_command_time > 1.0:
                    try:
                        new_commands = self.game_logic.process_turn(received_responses, broadcast_messages)
                        pending_commands.extend(new_commands)
                        received_responses = []
                        broadcast_messages = []
                        last_command_time = current_time
                    except Exception as e:
                        print(f"Error in game logic: {e}")
                        continue
                
                # Send up to 10 commands
                commands_to_send = min(len(pending_commands), 10)
                for i in range(commands_to_send):
                    self.send_command(pending_commands[i])
                
                # Handle special commands that don't need responses
                for i in range(commands_to_send - 1, -1, -1):
                    if "Incantation" in pending_commands[i]:
                        pending_commands.pop(i)
                        break
                
                # Maintain reasonable loop frequency
                time.sleep(0.05)  # 50ms delay
                
            except KeyboardInterrupt:
                print("Received interrupt signal, shutting down...")
                break
            except Exception as e:
                print(f"Error in main loop: {e}")
                continue
        
        print("AI main loop terminated")
        self._cleanup_connection()
    
    def __del__(self):
        """Cleanup network handler."""
        self._cleanup_connection()
        if self.network_thread and self.network_thread.is_alive():
            self.network_thread.join(timeout=1.0)