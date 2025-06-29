##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## Improved game_logic with better communication and strategy
##

from enum import Enum
from typing import Tuple, List, Dict
import subprocess
from itertools import cycle
from random import randint
import time
from game_utils import GameUtils

class Orientation(Enum):
    NORTH = 1
    EAST = 2
    SOUTH = 3
    WEST = 4

class PlayerState(Enum):
    EXPLORING = 1
    COLLECTING = 2
    ELEVATING = 3
    COORDINATING = 4

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

    def move_north(self):
        self.y = (self.y - 1 + self.map_height) % self.map_height

    def move_south(self):
        self.y = (self.y + 1) % self.map_height

    def move_west(self):
        self.x = (self.x - 1 + self.map_width) % self.map_width

    def move_east(self):
        self.x = (self.x + 1) % self.map_width

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
RESOURCE_PRIORITIES = {
    "food": 10,
    "linemate": 8,
    "deraumere": 6,
    "sibur": 5,
    "mendiane": 4,
    "phiras": 3,
    "thystame": 2
}

class Inventory:
    def __init__(self):
        self.resources = {
            "food": 0,
            "linemate": 0,
            "deraumere": 0,
            "sibur": 0,
            "mendiane": 0,
            "phiras": 0,
            "thystame": 0
        }

    def add_resource(self, resource: str, amount: int = 1):
        """Add a resource to inventory."""
        if resource in self.resources:
            self.resources[resource] += amount

    def remove_resource(self, resource: str, amount: int = 1):
        """Remove a resource from inventory."""
        if resource in self.resources and self.resources[resource] >= amount:
            self.resources[resource] -= amount
            return True
        return False

    def get_resource(self, resource: str) -> int:
        """Get amount of a specific resource."""
        return self.resources.get(resource, 0)

    def set_resource(self, resource: str, amount: int):
        """Set a specific resource amount."""
        if resource in self.resources:
            self.resources[resource] = amount

    def to_string(self):
        """Convert inventory to string representation."""
        return ", ".join([f"{res} {amt}" for res, amt in self.resources.items()])

    def set_from_list(self, inventory_list):
        """Set inventory from parsed list."""
        for i, resource in enumerate(RESOURCE_TYPES):
            if i < len(inventory_list):
                self.set_resource(resource, int(inventory_list[i][1]))

class GameLogic:
    def __init__(self, team_name: str, server_host, server_port):
        self.player_id = 0
        self.level = 1
        self.state = PlayerState.EXPLORING
        self.inventory = Inventory()
        self.team_inventories = [Inventory() for _ in range(7)]  # 6 players + total
        self.orientation = Orientation.NORTH
        self.available_slots = 0
        
        # Command management
        self.command_queue = []
        self.pending_commands = []
        self.response_queue = []
        
        # Game state
        self.vision_data = []
        self.inventory_data = []
        self.team_name = team_name
        self.server_host = server_host
        self.server_port = server_port
        
        # AI state
        self.target_resource = ""
        self.enough_players = False
        self.first_turn = True
        self.inventory_updated = False
        self.is_elevating = False
        self.movement_done = False
        
        # Performance tracking
        self.last_action_time = time.time()
        self.action_timeout = 5.0  # seconds
        self.exploration_pattern = 0
        
        # Team coordination
        self.team_leader = False
        self.coordination_messages = []
        self.shared_resources = {}

    def encrypt_message(self, message: str):
        """Encrypt message using team name as key."""
        return GameUtils.encrypt_and_compress(str(self.player_id) + "|" + message, self.team_name)

    def decrypt_message(self, message: str):
        """Decrypt message using team name as key."""
        return GameUtils.decrypt_and_decompress(message, self.team_name)

    def parse_vision_data(self, content: str):
        """Parse vision data from look command."""
        content = content.replace('[', '').replace(']', '')
        tiles = content.split(',')
        self.vision_data = []
        
        for tile in tiles:
            tile = tile.strip()
            if tile:
                items = tile.split()
                self.vision_data.append(items)
            else:
                self.vision_data.append([])

    def broadcast_inventory(self):
        """Broadcast current inventory to team."""
        encrypted_msg = self.encrypt_message("inventory|" + self.inventory.to_string())
        self.command_queue.append(f"Broadcast {encrypted_msg}")

    def broadcast_resource_found(self, resource: str, direction: int):
        """Broadcast resource location to team."""
        encrypted_msg = self.encrypt_message(f"resource|{resource}|{direction}")
        self.command_queue.append(f"Broadcast {encrypted_msg}")

    def calculate_priority_score(self, resource: str) -> int:
        """Calculate priority score for a resource."""
        base_priority = RESOURCE_PRIORITIES.get(resource, 0)
        
        # Increase priority if we need this resource for elevation
        if self.level <= 7:
            requirements = ELEVATION_REQUIREMENTS[self.level - 1]
            resource_index = RESOURCE_TYPES.index(resource) if resource in RESOURCE_TYPES else -1
            if resource_index > 0 and resource_index <= len(requirements) - 1:
                needed = requirements[resource_index]
                current = self.inventory.get_resource(resource)
                if current < needed:
                    base_priority += (needed - current) * 3
        
        return base_priority

    def find_needed_resource(self) -> str:
        """Find the most needed resource for elevation."""
        if self.level > 7:
            return None
            
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        most_needed = None
        highest_priority = 0
        
        for i, resource in enumerate(RESOURCE_TYPES[1:], 1):  # Skip food
            needed = requirements[i] if i < len(requirements) else 0
            current = self.team_inventories[6].get_resource(resource)
            
            if current < needed:
                priority = self.calculate_priority_score(resource)
                if priority > highest_priority:
                    highest_priority = priority
                    most_needed = resource
        
        return most_needed

    def navigate_to_tile(self, tile_position: int, target_item: str):
        """Navigate to a specific tile and collect item."""
        if tile_position == 0:
            # Item is on current tile
            self.command_queue.append(f"Take {target_item}")
            self.inventory_updated = True
            return

        # Calculate movement to target tile
        if self.level >= len(VISION_RANGES):
            return
            
        vision_range = VISION_RANGES[self.level - 1]
        if tile_position < vision_range[0] or tile_position > vision_range[1]:
            return
            
        # Simple navigation - move forward towards target
        steps_needed = min(tile_position, 3)  # Limit steps per turn
        for _ in range(steps_needed):
            self.command_queue.append("Forward")
        
        self.command_queue.append(f"Take {target_item}")
        self.inventory_updated = True

    def find_item_in_vision(self, item: str) -> int:
        """Find item in vision and return its position."""
        for i, tile_data in enumerate(self.vision_data):
            if item in tile_data:
                return i
        return -1

    def smart_exploration(self):
        """Implement smarter exploration pattern."""
        current_time = time.time()
        
        # If stuck in same place, change pattern
        if current_time - self.last_action_time > self.action_timeout:
            self.exploration_pattern = (self.exploration_pattern + 1) % 4
            self.last_action_time = current_time
        
        # Different exploration patterns
        if self.exploration_pattern == 0:
            # Spiral pattern
            if randint(0, 3) == 0:
                self.command_queue.append("Right")
            self.command_queue.append("Forward")
        elif self.exploration_pattern == 1:
            # Random walk with bias
            if randint(0, 5) == 0:
                self.command_queue.append("Left")
            elif randint(0, 5) == 0:
                self.command_queue.append("Right")
            self.command_queue.append("Forward")
        elif self.exploration_pattern == 2:
            # Linear exploration
            for _ in range(3):
                self.command_queue.append("Forward")
            self.command_queue.append("Right")
        else:
            # Return to center and restart
            self.command_queue.extend(["Left", "Left", "Forward", "Forward"])

    def collect_resources(self):
        """Enhanced resource collection with priorities."""
        food_threshold = 15 if self.player_id == 0 else 25
        
        # Priority 1: Critical food shortage
        if self.inventory.get_resource("food") < 5:
            food_tile = self.find_item_in_vision("food")
            if food_tile >= 0:
                self.navigate_to_tile(food_tile, "food")
                self.state = PlayerState.COLLECTING
                return
        
        # Priority 2: Needed resources for elevation
        needed_resource = self.find_needed_resource()
        if needed_resource:
            resource_tile = self.find_item_in_vision(needed_resource)
            if resource_tile >= 0:
                self.navigate_to_tile(resource_tile, needed_resource)
                self.broadcast_resource_found(needed_resource, resource_tile)
                self.state = PlayerState.COLLECTING
                return
        
        # Priority 3: Food maintenance
        if self.inventory.get_resource("food") < food_threshold:
            food_tile = self.find_item_in_vision("food")
            if food_tile >= 0:
                self.navigate_to_tile(food_tile, "food")
                self.state = PlayerState.COLLECTING
                return
        
        # Priority 4: Opportunistic collection
        for resource in RESOURCE_TYPES[1:]:  # Skip food
            if self.inventory.get_resource(resource) < 5:  # Keep some buffer
                resource_tile = self.find_item_in_vision(resource)
                if resource_tile >= 0:
                    self.navigate_to_tile(resource_tile, resource)
                    self.state = PlayerState.COLLECTING
                    return
        
        # No specific targets, explore intelligently
        self.state = PlayerState.EXPLORING
        self.smart_exploration()

    def update_inventory(self, inventory_list):
        """Update player inventory and return if changed."""
        changed = False
        for item in inventory_list:
            if len(item) >= 2:
                resource = item[0]
                amount = int(item[1])
                if self.inventory.get_resource(resource) != amount:
                    changed = True
                self.inventory.set_resource(resource, amount)
        
        # Update team inventory
        if self.player_id > 0:
            self.team_inventories[self.player_id - 1] = self.inventory
        self.calculate_total_inventory()
        
        return changed

    def calculate_total_inventory(self):
        """Calculate total team inventory."""
        for resource in RESOURCE_TYPES:
            total = sum(inv.get_resource(resource) for inv in self.team_inventories[:6])
            self.team_inventories[6].set_resource(resource, total)

    def can_perform_elevation(self) -> bool:
        """Check if elevation requirements are met."""
        if not self.vision_data or self.level > 7:
            return False
        
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        
        # Count players and resources on current tile
        player_count = sum(1 for item in self.vision_data[0] if "player" in item)
        
        if player_count < requirements[0]:
            return False
        
        # Check resource requirements
        for i, resource in enumerate(RESOURCE_TYPES[1:], 1):
            if i < len(requirements):
                needed = requirements[i]
                available = sum(1 for item in self.vision_data[0] if resource in item)
                if available < needed:
                    return False
        
        return True

    def handle_elevation(self):
        """Enhanced elevation handling."""
        if self.level > 7:
            return
        
        # Only coordinate if we have enough resources
        needed_resource = self.find_needed_resource()
        if needed_resource is None and self.inventory.get_resource("food") > 20:
            self.state = PlayerState.ELEVATING
            
            # Check if we can elevate on current tile
            if self.can_perform_elevation():
                self.drop_elevation_resources()
                self.command_queue.append("Incantation")
                self.is_elevating = True
            else:
                # Broadcast need for elevation
                elevation_msg = self.encrypt_message("elevation_needed")
                self.command_queue.append(f"Broadcast {elevation_msg}")

    def drop_elevation_resources(self):
        """Drop resources needed for elevation."""
        if self.level > 7:
            return
            
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        
        for i, resource in enumerate(RESOURCE_TYPES[1:], 1):
            if i < len(requirements):
                needed = requirements[i]
                current = self.inventory.get_resource(resource)
                amount_to_drop = min(current, needed)
                
                for _ in range(amount_to_drop):
                    self.command_queue.append(f"Set {resource}")

    def spawn_new_player(self):
        """Handle player forking logic."""
        if (self.inventory.get_resource("food") < 30 or 
            self.enough_players or self.player_id != 0):
            return
        
        if self.available_slots > 0:
            subprocess.Popen([
                "./zappy_ai", "-p", str(self.server_port),
                "-n", self.team_name, "-h", str(self.server_host)
            ])
            self.player_id = 1

    def process_broadcast_messages(self, messages: List[str]):
        """Process received broadcast messages."""
        for message in messages:
            parts = message.split(", ", 1)
            if len(parts) != 2:
                continue
                
            direction = parts[0]
            content = parts[1].rstrip('\n')
            decrypted = self.decrypt_message(content)
            
            if decrypted == "error":
                continue
            
            try:
                sender_id, message_content = decrypted.split("|", 1)
                sender_id = int(sender_id)
                
                if "ready" in message_content:
                    self.enough_players = True
                    if self.player_id == 0:
                        self.player_id = 1
                
                elif "here" in message_content:
                    self.player_id += 1
                    if self.player_id >= 6:
                        ready_msg = self.encrypt_message("ready")
                        self.command_queue.append(f"Broadcast {ready_msg}")
                
                elif "inventory" in message_content:
                    if sender_id <= 6:
                        inv_data = message_content.split("|")[1]
                        # Parse and update team inventory
                        # Implementation depends on inventory format
                
                elif "resource" in message_content:
                    # Handle resource location sharing
                    parts = message_content.split("|")
                    if len(parts) >= 3:
                        resource_type = parts[1]
                        resource_dir = int(parts[2])
                        self.shared_resources[resource_type] = resource_dir
                
                elif "elevation_needed" in message_content:
                    if not self.is_elevating:
                        self.state = PlayerState.COORDINATING
                        
            except (ValueError, IndexError):
                continue

    def process_command_responses(self, responses: List[str]):
        """Process responses from sent commands."""
        for response in responses:
            if "|" not in response:
                continue
                
            command_parts = response.split("|", 1)
            command = command_parts[0]
            result = command_parts[1] if len(command_parts) > 1 else ""
            
            if command == "Connect_nbr":
                self.available_slots = int(result) if result.isdigit() else 0
            elif command == "Look":
                self.parse_vision_data(result)
            elif command == "Inventory":
                self.inventory_data = GameUtils.parse_string_to_list(result)
            elif command == "Incantation":
                self.is_elevating = False
                if "Current level:" in result:
                    try:
                        self.level = int(result.split(":")[1].strip())
                    except (ValueError, IndexError):
                        pass
                finished_msg = self.encrypt_message("finished")
                self.command_queue.append(f"Broadcast {finished_msg}")
            elif command in ["Forward", "Right", "Left"]:
                self.movement_done = True
                self.last_action_time = time.time()

    def initialize_player(self):
        """Initialize player on first turn."""
        here_msg = self.encrypt_message("here")
        self.command_queue.extend([
            f"Broadcast {here_msg}",
            "Inventory",
            "Look",
            "Connect_nbr"
        ])

    def finalize_turn(self):
        """Finalize turn with inventory check and look."""
        if self.inventory_updated:
            self.command_queue.append("Inventory")
            self.inventory_updated = False
        
        if self.inventory_data:
            if self.update_inventory(self.inventory_data):
                self.broadcast_inventory()
        
        # Always end with look to update vision
        self.command_queue.append("Look")

    def process_turn(self, responses: List[str], messages: List[str]) -> List[str]:
        """Main game logic processing with improved strategy."""
        self.command_queue = []
        
        # First turn initialization
        if self.first_turn:
            self.initialize_player()
            self.first_turn = False
            return self.command_queue
        
        # Process responses and messages
        self.process_command_responses(responses)
        self.process_broadcast_messages(messages)
        
        # State machine for different behaviors
        if self.state == PlayerState.ELEVATING or self.is_elevating:
            self.handle_elevation()
        elif self.state == PlayerState.COORDINATING:
            # Move towards team coordination point
            self.command_queue.extend(["Forward", "Look"])
            self.state = PlayerState.EXPLORING
        else:
            # Normal gameplay
            if not self.is_elevating:
                self.spawn_new_player()
                self.collect_resources()
        
        # Finalize turn
        self.finalize_turn()
        
        # Reset turn state
        self.inventory_data = []
        self.movement_done = False
        self.command_queue.append("Connect_nbr")
        
        return self.command_queue