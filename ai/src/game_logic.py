##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## game_logic
##

from enum import Enum
from typing import Tuple, List
import subprocess
from itertools import cycle
from random import randint
from game_utils import GameUtils

class Orientation(Enum):
    NORTH = 1
    EAST = 2
    SOUTH = 3
    WEST = 4

class Position:
    def __init__(self, x: int, y: int, map_width: int, map_height: int):
        self.x = x
        self.y = y
        self.map_width = map_width
        self.map_height = map_height

    def __str__(self):
        return f"x: {self.x} y: {self.y}"

    def __eq__(self, other):
        if isinstance(other, Position):
            return self.x == other.x and self.y == other.y
        return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def get_coordinates(self):
        return (self.x, self.y)

    def set_coordinates(self, x: int, y: int):
        self.x = x
        self.y = y

    def move_north(self):
        if self.y == 0:
            self.y = self.map_height - 1
        else:
            self.y -= 1

    def move_south(self):
        if self.y == self.map_height - 1:
            self.y = 0
        else:
            self.y += 1

    def move_west(self):
        if self.x == 0:
            self.x = self.map_width - 1
        else:
            self.x -= 1

    def move_east(self):
        if self.x == self.map_width - 1:
            self.x = 0
        else:
            self.x += 1

# Requirements for each elevation level
ELEVATION_REQUIREMENTS = [
    # [nb_players, linemate, deraumere, sibur, mendiane, phiras, thystame]
    [1, 1, 0, 0, 0, 0, 0],  # Level 1->2
    [2, 1, 1, 1, 0, 0, 0],  # Level 2->3
    [2, 2, 0, 1, 0, 2, 0],  # Level 3->4
    [4, 1, 1, 2, 0, 1, 0],  # Level 4->5
    [4, 1, 2, 1, 3, 0, 0],  # Level 5->6
    [6, 1, 2, 3, 0, 1, 0],  # Level 6->7
    [6, 2, 2, 2, 2, 2, 1],  # Level 7->8
]

VISION_RANGES = [
    [1, 3],    # Level 1
    [4, 8],    # Level 2
    [9, 15],   # Level 3
    [16, 24],  # Level 4
    [25, 35],  # Level 5
    [36, 48],  # Level 6
    [49, 63],  # Level 7
    [64, 80],  # Level 8
]

RESOURCE_TYPES = ["food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]

class Inventory:
    def __init__(self):
        self.food = 0
        self.linemate = 0
        self.deraumere = 0
        self.sibur = 0
        self.mendiane = 0
        self.phiras = 0
        self.thystame = 0

    def add_resource(self, resource: str, amount: int):
        """Add a resource to inventory."""
        if resource == "food":
            self.food += amount
        elif resource == "linemate":
            self.linemate += amount
        elif resource == "deraumere":
            self.deraumere += amount
        elif resource == "sibur":
            self.sibur += amount
        elif resource == "mendiane":
            self.mendiane += amount
        elif resource == "phiras":
            self.phiras += amount
        elif resource == "thystame":
            self.thystame += amount
        else:
            raise ValueError("Invalid resource type")

    def remove_resource(self, resource: str, amount: int):
        """Remove a resource from inventory."""
        current_amount = self.get_resource(resource)
        if current_amount < amount:
            raise ValueError(f"Not enough {resource}")
        
        if resource == "food":
            self.food -= amount
        elif resource == "linemate":
            self.linemate -= amount
        elif resource == "deraumere":
            self.deraumere -= amount
        elif resource == "sibur":
            self.sibur -= amount
        elif resource == "mendiane":
            self.mendiane -= amount
        elif resource == "phiras":
            self.phiras -= amount
        elif resource == "thystame":
            self.thystame -= amount

    def get_resource(self, resource: str):
        """Get amount of a specific resource."""
        if resource == "food":
            return self.food
        elif resource == "linemate":
            return self.linemate
        elif resource == "deraumere":
            return self.deraumere
        elif resource == "sibur":
            return self.sibur
        elif resource == "mendiane":
            return self.mendiane
        elif resource == "phiras":
            return self.phiras
        elif resource == "thystame":
            return self.thystame
        else:
            raise ValueError("Invalid resource type")

    def to_string(self):
        """Convert inventory to string representation."""
        return (f"food {self.food}, linemate {self.linemate}, "
                f"deraumere {self.deraumere}, sibur {self.sibur}, "
                f"mendiane {self.mendiane}, phiras {self.phiras}, "
                f"thystame {self.thystame}")

    def set_resource(self, resource: str, amount: int):
        """Set a specific resource amount."""
        if "food" in resource:
            self.food = int(amount)
        elif "linemate" in resource:
            self.linemate = int(amount)
        elif "deraumere" in resource:
            self.deraumere = int(amount)
        elif "sibur" in resource:
            self.sibur = int(amount)
        elif "mendiane" in resource:
            self.mendiane = int(amount)
        elif "phiras" in resource:
            self.phiras = int(amount)
        elif "thystame" in resource:
            self.thystame = int(amount)
        else:
            raise ValueError("Invalid resource type")

    def set_from_list(self, inventory_list):
        """Set inventory from parsed list."""
        for i in range(7):
            self.set_resource(RESOURCE_TYPES[i], inventory_list[i][1])

class GameLogic:
    def __init__(self, team_name: str, server_host, server_port):
        self.player_id = 0
        self.level = 1
        self.inventory = Inventory()
        self.team_inventories = [Inventory() for _ in range(7)]  # 6 players + total
        self.orientation = Orientation.NORTH
        self.available_slots = 0
        
        self.command_queue = []
        self.vision_data = []
        self.inventory_data = []
        self.team_name = team_name
        self.server_host = server_host
        self.server_port = server_port
        self.new_item_found = False
        self.target_resource = ""
        
        self.enough_players = False
        self.first_turn = True
        self.inventory_updated = False
        self.is_elevating = False
        self.movement_done = False

    def encrypt_message(self, message: str):
        """Encrypt message using team name as key."""
        return GameUtils.encrypt_and_compress(str(self.player_id) + "|" + message, self.team_name)

    def decrypt_message(self, message: str):
        """Decrypt message using team name as key."""
        return GameUtils.decrypt_and_decompress(message, self.team_name)

    def parse_vision_data(self, content: str):
        """Parse vision data from look command."""
        content = content.split(",")
        self.vision_data = GameUtils.parse_string_to_list(content)

    def broadcast_inventory(self):
        """Broadcast current inventory to team."""
        encrypted_msg = self.encrypt_message("inventory|" + self.inventory.to_string())
        self.command_queue.append(f"Broadcast {encrypted_msg}")

    def turn_left(self):
        self.orientation = Orientation((self.orientation.value - 2) % 4 + 1)

    def turn_right(self):
        self.orientation = Orientation(self.orientation.value % 4 + 1)

    def move_forward(self):
        pass

    def find_needed_resource(self):
        resource_index = 1
        for resource in RESOURCE_TYPES:
            if resource == "food":
                continue
            if self.team_inventories[6].get_resource(resource) < ELEVATION_REQUIREMENTS[self.level - 1][resource_index]:
                return resource
            resource_index += 1
        return None

    def navigate_to_tile(self, tile_position: int, target_item: str):
        """Navigate to a specific tile and collect item."""
        if tile_position == 0:
            # Item is on current tile
            for item in self.vision_data[0]:
                if target_item in item:
                    self.command_queue.append(f"Take {target_item}")
            self.inventory_updated = True
            return
        self.command_queue.append("Forward")
        # Calculate which level the tile is on
        level = 0
        while not VISION_RANGES[level][0] <= tile_position <= VISION_RANGES[level][1]:
            self.command_queue.append("Forward")
            level += 1
        # Calculate direction adjustment
        level_center = (VISION_RANGES[level][0] + VISION_RANGES[level][1]) // 2
        direction_offset = tile_position - level_center

        if direction_offset < 0:
            self.command_queue.append("Left")
        elif direction_offset > 0:
            self.command_queue.append("Right")
        
        # Move to target tile
        for _ in range(abs(direction_offset)):
            self.command_queue.append("Forward")
        
        # Collect item
        for item in self.vision_data[level]:
            if target_item in item:
                self.command_queue.append(f"Take {target_item}")
        
        self.inventory_updated = True

    def find_item_in_vision(self, item: str):
        for i, tile_data in enumerate(self.vision_data):
            if item in str(tile_data):
                return i
        return None

    def collect_resources(self):
        food_threshold = 20 if self.player_id == 0 else 40
        
        # Priority 1: Collect food if needed
        if self.inventory.get_resource("food") < food_threshold:
            food_tile = self.find_item_in_vision("food")
            if food_tile is not None:
                self.navigate_to_tile(food_tile, "food")
                return
            else:
                # Wander to find food
                if randint(0, 5) == 0:
                    self.command_queue.append("Left")
                self.command_queue.append("Forward")
                return
        
        # Priority 2: Collect stones needed for elevation
        needed_resource = self.find_needed_resource()
        if needed_resource is not None:
            resource_tile = self.find_item_in_vision(needed_resource)
            if resource_tile is not None:
                self.navigate_to_tile(resource_tile, needed_resource)
                return
            else:
                # Wander to find needed resource
                if randint(0, 5) == 0:
                    self.command_queue.append("Left")
                self.command_queue.append("Forward")
                return
        else:
            # Check inventory if no resources needed
            self.command_queue.append("Inventory")

    def update_inventory(self, inventory_list):
        """Update player inventory and return if changed."""
        print(f"[DEBUG LOGIC] Updating inventory with: {inventory_list}")
        try:
            changed = False
            for item in inventory_list:
                if len(item) >= 2:
                    resource_name = item[0]
                    try:
                        amount = int(item[1])
                        old_amount = self.inventory.get_resource(resource_name)
                        if old_amount != amount:
                            changed = True
                            print(f"[DEBUG LOGIC] {resource_name}: {old_amount} -> {amount}")
                        self.inventory.set_resource(resource_name, amount)
                    except (ValueError, IndexError) as e:
                        print(f"[ERROR] Failed to parse inventory item {item}: {e}")
                else:
                    print(f"[WARNING] Malformed inventory item: {item}")
            
            if changed:
                print(f"[DEBUG LOGIC] Inventory updated, broadcasting to team")
                self.team_inventories[self.player_id - 1].set_from_list(inventory_list)
            return changed
        except Exception as e:
            print(f"[ERROR] Exception updating inventory: {e}")
            return False

    def update_team_inventory(self, inventory_list):
        """Update team member inventory."""
        for item in inventory_list:
            self.team_inventories[self.player_id - 1].set_resource(item[0], item[1])
        self.calculate_total_inventory()

    def calculate_total_inventory(self):
        """Calculate total team inventory."""
        for resource in RESOURCE_TYPES:
            self.team_inventories[6].set_resource(resource, 0)
        
        for i in range(6):
            for resource in RESOURCE_TYPES:
                current_amount = self.team_inventories[6].get_resource(resource)
                member_amount = self.team_inventories[i].get_resource(resource)
                self.team_inventories[6].set_resource(resource, current_amount + member_amount)

    def finalize_turn(self):
        """Finalize turn with inventory check and look."""
        if self.inventory_updated:
            self.command_queue.append("Inventory")
            self.inventory_updated = False
        
        if self.inventory_data:
            if self.update_inventory(self.inventory_data):
                self.broadcast_inventory()
        
        self.command_queue.append("Look")

    def spawn_new_player(self):
        """Handle player forking logic."""
        if (self.inventory.get_resource("food") < 20 or 
            self.enough_players or self.player_id != 0):
            return
        
        if self.available_slots != 0:
            subprocess.Popen([
                "./zappy_ai", "-p", str(self.server_port),
                "-n", self.team_name, "-h", str(self.server_host)
            ])
            self.player_id = 1
        else:
            self.command_queue.append("Fork")

    def process_broadcast_messages(self, messages: List[str]):
        """Process received broadcast messages."""
        print(f"[DEBUG LOGIC] Processing {len(messages)} broadcast messages")
        
        for message in messages:
            try:
                print(f"[DEBUG LOGIC] Processing broadcast: '{message}'")
                
                if ", " not in message:
                    print(f"[WARNING] Malformed broadcast message: '{message}'")
                    continue
                    
                direction = message.split(", ")[0]
                content = message.split(", ")[1].split("\n")[0]
                
                print(f"[DEBUG LOGIC] Direction: {direction}, Content: '{content}'")
                
                try:
                    decrypted = self.decrypt_message(content)
                    print(f"[DEBUG LOGIC] Decrypted: '{decrypted}'")
                except:
                    print(f"[DEBUG LOGIC] Failed to decrypt or not for us: '{content}'")
                    continue
                
                if decrypted == "error":
                    print(f"[DEBUG LOGIC] Decryption failed")
                    continue
                
                if "|" not in decrypted:
                    print(f"[WARNING] Malformed decrypted message: '{decrypted}'")
                    continue
                    
                sender_id_str = decrypted.split("|", 1)[0]
                message_content = decrypted.split("|", 1)[1]
                
                try:
                    sender_id = int(sender_id_str)
                    print(f"[DEBUG LOGIC] Sender ID: {sender_id}, Message: '{message_content}'")
                except ValueError:
                    print(f"[ERROR] Invalid sender ID: '{sender_id_str}'")
                    continue
                
                if "ready" in message_content:
                    print(f"[DEBUG LOGIC] Team ready signal received")
                    self.enough_players = True
                    if self.player_id == 0:
                        self.player_id = 1
                
                if "here" in message_content:
                    print(f"[DEBUG LOGIC] Player here signal received")
                    self.player_id += 1
                    if self.player_id == 6:
                        ready_msg = self.encrypt_message("ready")
                        self.command_queue.append(f"Broadcast {ready_msg}")
                        print(f"[DEBUG LOGIC] Broadcasting ready signal")
                
                if "inventory" in message_content:
                    print(f"[DEBUG LOGIC] Team inventory update received")
                    try:
                        inventory_str = message_content.split("|")[1]
                        parsed_inventory = GameUtils.parse_string_to_list(inventory_str)
                        self.team_inventories[sender_id - 1].set_from_list(parsed_inventory)
                        self.calculate_total_inventory()
                    except Exception as e:
                        print(f"[ERROR] Failed to process team inventory: {e}")
                
                if "finished" in message_content:
                    print(f"[DEBUG LOGIC] Elevation finished signal received")
                    self.is_elevating = False
                
                if "elevation" in message_content and not self.movement_done:
                    print(f"[DEBUG LOGIC] Elevation signal received, moving to position")
                    self.command_queue = []
                    self.is_elevating = True
                    
                    try:
                        direction_num = int(direction)
                        print(f"[DEBUG LOGIC] Moving in direction: {direction_num}")
                        
                        # Move based on broadcast direction
                        if direction_num == 1:
                            self.command_queue.append("Forward")
                        elif direction_num == 5:
                            self.command_queue.extend(["Left", "Left", "Forward"])
                        elif direction_num in [2, 3, 4]:
                            self.command_queue.extend(["Left", "Forward"])
                        elif direction_num in [6, 7, 8]:
                            self.command_queue.extend(["Right", "Forward"])
                        elif direction_num == 0:
                            self.drop_elevation_resources()
                    except ValueError:
                        print(f"[ERROR] Invalid direction number: '{direction}'")
                    
            except Exception as e:
                print(f"[ERROR] Exception processing broadcast '{message}': {e}")
                import traceback
                traceback.print_exc()

    def drop_elevation_resources(self):
        """Drop resources needed for elevation."""
        resource_index = 1
        for resource in RESOURCE_TYPES:
            if resource == "food":
                continue
            required_amount = ELEVATION_REQUIREMENTS[self.level - 1][resource_index]
            if required_amount > 0:
                current_amount = self.inventory.get_resource(resource)
                for _ in range(current_amount):
                    self.command_queue.append(f"Set {resource}")
            resource_index += 1

    def process_command_responses(self, responses: List[str]):
        """Process responses from sent commands with error protection."""
        print(f"[DEBUG LOGIC] Processing {len(responses)} responses: {responses}")
        
        for response in responses:
            try:
                print(f"[DEBUG LOGIC] Processing response: '{response}'")
                command_parts = response.split("|")
                if len(command_parts) < 2:
                    print(f"[WARNING] Malformed response: '{response}'")
                    continue
                    
                command = command_parts[0]
                result = command_parts[1] if len(command_parts) > 1 else ""
                print(f"[DEBUG LOGIC] Command: '{command}', Result: '{result}'")
                
                if command == "Connect_nbr":
                    try:
                        self.available_slots = int(result)
                        print(f"[DEBUG LOGIC] Available slots: {self.available_slots}")
                    except ValueError as e:
                        print(f"[ERROR] Failed to parse Connect_nbr result '{result}': {e}")
                        
                elif command == "Look":
                    try:
                        print(f"[DEBUG LOGIC] Parsing vision data: '{result}'")
                        self.vision_data = GameUtils.parse_string_to_list(result)
                        print(f"[DEBUG LOGIC] Vision data parsed: {self.vision_data}")
                    except Exception as e:
                        print(f"[ERROR] Failed to parse Look result '{result}': {e}")
                        
                elif command == "Inventory":
                    try:
                        print(f"[DEBUG LOGIC] Parsing inventory data: '{result}'")
                        self.inventory_data = GameUtils.parse_string_to_list(result)
                        print(f"[DEBUG LOGIC] Inventory data parsed: {self.inventory_data}")
                    except Exception as e:
                        print(f"[ERROR] Failed to parse Inventory result '{result}': {e}")
                        
                elif command == "Incantation":
                    print(f"[DEBUG LOGIC] Incantation response: '{result}'")
                    self.is_elevating = False
                    if "Current level:" in result:
                        try:
                            level_part = result.split(":")[1].strip()
                            self.level = int(level_part)
                            print(f"[DEBUG LOGIC] New level: {self.level}")
                        except (IndexError, ValueError) as e:
                            print(f"[ERROR] Failed to parse level from '{result}': {e}")
                            
                    finished_msg = self.encrypt_message("finished")
                    self.command_queue.append(f"Broadcast {finished_msg}")
                    
                elif command in ["Forward", "Right", "Left"]:
                    print(f"[DEBUG LOGIC] Movement command completed: {command}")
                    if result == "ok":
                        self.movement_done = True
                    else:
                        print(f"[WARNING] Movement failed: {command} -> {result}")
                        
                elif command in ["Take", "Set"]:
                    print(f"[DEBUG LOGIC] Resource command completed: {command}")
                    if result == "ok":
                        print(f"[DEBUG LOGIC] {command} successful")
                    else:
                        print(f"[DEBUG LOGIC] {command} failed")
                        
                elif command == "Broadcast":
                    print(f"[DEBUG LOGIC] Broadcast completed: {result}")
                    
                elif command == "Fork":
                    print(f"[DEBUG LOGIC] Fork completed: {result}")
                    
                elif command == "Eject":
                    print(f"[DEBUG LOGIC] Eject completed: {result}")
                    
                else:
                    print(f"[WARNING] Unknown command response: '{command}' -> '{result}'")
                    
            except Exception as e:
                print(f"[ERROR] Exception processing response '{response}': {e}")
                import traceback
                traceback.print_exc()

    def initialize_player(self):
        """Initialize player on first turn."""
        here_msg = self.encrypt_message("here")
        self.command_queue.extend([
            f"Broadcast {here_msg}",
            "Inventory",
            "Look",
            "Connect_nbr"
        ])

    def can_perform_elevation(self):
        """Check if elevation requirements are met."""
        if not self.vision_data:
            return False
        
        # Count players and resources on current tile
        player_count = 0
        resources = {"linemate": 0, "deraumere": 0, "sibur": 0, 
                    "mendiane": 0, "phiras": 0, "thystame": 0}
        
        for item in self.vision_data[0]:
            if "player" in item:
                player_count += 1
            else:
                for resource in resources:
                    if resource in item:
                        resources[resource] += 1
        
        # Check requirements
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        return (player_count >= 6 and
                resources["linemate"] >= requirements[1] and
                resources["deraumere"] >= requirements[2] and
                resources["sibur"] >= requirements[3] and
                resources["mendiane"] >= requirements[4] and
                resources["phiras"] >= requirements[5] and
                resources["thystame"] >= requirements[6])

    def handle_elevation(self):
        """Handle elevation logic."""
        if self.player_id != 6:
            return
        
        # Check if all team members have enough food
        enough_food = True
        if not self.is_elevating:
            for i in range(6):
                if self.team_inventories[i].get_resource("food") < 40:
                    enough_food = False
                    break
        
        # Start elevation if conditions are met
        if self.find_needed_resource() is None and enough_food:
            self.is_elevating = True
        
        if self.is_elevating:
            elevation_msg = self.encrypt_message("elevation")
            self.command_queue.extend([
                f"Broadcast {elevation_msg}",
                "Look"
            ])
            self.drop_elevation_resources()
            
            if self.can_perform_elevation():
                self.command_queue.append("Incantation")

    def process_turn(self, responses: List[str], messages: List[str]) -> List[str]:
        """Main game logic processing."""
        self.command_queue = []
        
        # First turn initialization
        if self.first_turn:
            self.initialize_player()
            self.first_turn = False
            return self.command_queue
        
        # Process command responses and messages
        self.process_command_responses(responses)
        self.process_broadcast_messages(messages)
        
        # Handle elevation if applicable
        self.handle_elevation()
        
        # Normal gameplay when not elevating
        if not self.is_elevating:
            self.spawn_new_player()
            self.collect_resources()
            self.finalize_turn()
        
        # Reset turn state
        self.inventory_data = []
        self.movement_done = False
        self.command_queue.append("Connect_nbr")
        
        return self.command_queue
