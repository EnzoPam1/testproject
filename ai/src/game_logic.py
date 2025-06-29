##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## game_logic avec logging complet
##

import logging
from enum import Enum
from typing import Tuple, List
import subprocess
from itertools import cycle
from random import randint
from game_utils import GameUtils

# Configuration du logging détaillé
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - AI_LOGIC - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('ai_behavior.log')
    ]
)
logger = logging.getLogger(__name__)

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
        old_amount = getattr(self, resource)
        setattr(self, resource, old_amount + amount)
        logger.info(f"INVENTORY: Added {amount} {resource}, total: {getattr(self, resource)}")

    def remove_resource(self, resource: str, amount: int):
        """Remove a resource from inventory."""
        current_amount = self.get_resource(resource)
        if current_amount < amount:
            logger.warning(f"INVENTORY: Not enough {resource} (have {current_amount}, need {amount})")
            raise ValueError(f"Not enough {resource}")
        
        setattr(self, resource, current_amount - amount)
        logger.info(f"INVENTORY: Removed {amount} {resource}, remaining: {getattr(self, resource)}")

    def get_resource(self, resource: str):
        """Get amount of a specific resource."""
        return getattr(self, resource, 0)

    def to_string(self):
        """Convert inventory to string representation."""
        return (f"food {self.food}, linemate {self.linemate}, "
                f"deraumere {self.deraumere}, sibur {self.sibur}, "
                f"mendiane {self.mendiane}, phiras {self.phiras}, "
                f"thystame {self.thystame}")

    def set_resource(self, resource: str, amount: int):
        """Set a specific resource amount."""
        old_amount = getattr(self, resource, 0)
        setattr(self, resource, int(amount))
        if old_amount != int(amount):
            logger.info(f"INVENTORY: {resource} changed from {old_amount} to {amount}")

    def set_from_list(self, inventory_list):
        """Set inventory from parsed list."""
        logger.debug(f"INVENTORY: Setting from list: {inventory_list}")
        for i in range(min(7, len(inventory_list))):
            if len(inventory_list[i]) >= 2:
                self.set_resource(RESOURCE_TYPES[i], inventory_list[i][1])

class GameLogic:
    def __init__(self, team_name: str, server_host, server_port):
        logger.info(f"INIT: Creating AI for team '{team_name}' on {server_host}:{server_port}")
        
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
        
        logger.info(f"INIT: AI initialized with default values")

    def encrypt_message(self, message: str):
        """Encrypt message using team name as key."""
        encrypted = GameUtils.encrypt_and_compress(str(self.player_id) + "|" + message, self.team_name)
        logger.debug(f"BROADCAST: Encrypted '{message}' -> '{encrypted[:20]}...'")
        return encrypted

    def decrypt_message(self, message: str):
        """Decrypt message using team name as key."""
        decrypted = GameUtils.decrypt_and_decompress(message, self.team_name)
        if decrypted != "error":
            logger.debug(f"BROADCAST: Decrypted message: '{decrypted}'")
        return decrypted

    def parse_vision_data(self, content: str):
        """Parse vision data from look command."""
        content = content.split(",")
        self.vision_data = GameUtils.parse_string_to_list(content)
        logger.info(f"VISION: Parsed {len(self.vision_data)} tiles")
        for i, tile in enumerate(self.vision_data):
            logger.debug(f"VISION: Tile {i}: {tile}")

    def broadcast_inventory(self):
        """Broadcast current inventory to team."""
        encrypted_msg = self.encrypt_message("inventory|" + self.inventory.to_string())
        self.command_queue.append(f"Broadcast {encrypted_msg}")
        logger.info(f"BROADCAST: Sending inventory to team")

    def find_needed_resource(self):
        """Find what resource is needed for next elevation."""
        if self.level > len(ELEVATION_REQUIREMENTS):
            logger.info("ELEVATION: Max level reached")
            return None
            
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        resource_index = 1
        
        for resource in RESOURCE_TYPES[1:]:  # Skip food
            needed = requirements[resource_index]
            have = self.team_inventories[6].get_resource(resource)
            if have < needed:
                logger.info(f"ELEVATION: Need {needed-have} more {resource} (have {have}, need {needed})")
                return resource
            resource_index += 1
        
        logger.info("ELEVATION: All resources collected for current level")
        return None

    def navigate_to_tile(self, tile_position: int, target_item: str):
        """Navigate to a specific tile and collect item."""
        logger.info(f"NAVIGATION: Going to tile {tile_position} for {target_item}")
        
        if tile_position == 0:
            # Item is on current tile
            logger.info(f"COLLECTION: Taking {target_item} from current tile")
            self.command_queue.append(f"Take {target_item}")
            self.inventory_updated = True
            return
        
        # Simple navigation - just move forward for now
        logger.info(f"MOVEMENT: Moving toward {target_item} at tile {tile_position}")
        self.command_queue.append("Forward")
        self.inventory_updated = True

    def find_item_in_vision(self, item: str):
        """Find item in vision data."""
        logger.debug(f"SEARCH: Looking for {item} in {len(self.vision_data)} tiles")
        
        for i, tile_data in enumerate(self.vision_data):
            tile_str = str(tile_data)
            if item in tile_str:
                logger.info(f"SEARCH: Found {item} at tile {i}: {tile_data}")
                return i
        
        logger.debug(f"SEARCH: {item} not found in vision")
        return None

    def collect_resources(self):
        """Main resource collection logic."""
        logger.info("COLLECTION: Starting resource collection phase")
        
        food_threshold = 15  # Lowered for more active behavior
        current_food = self.inventory.get_resource("food")
        
        logger.info(f"COLLECTION: Current food: {current_food}, threshold: {food_threshold}")
        
        # Priority 1: Collect food if needed
        if current_food < food_threshold:
            logger.info("COLLECTION: Need food, searching...")
            food_tile = self.find_item_in_vision("food")
            if food_tile is not None:
                logger.info(f"COLLECTION: Found food, collecting from tile {food_tile}")
                self.navigate_to_tile(food_tile, "food")
                return
            else:
                logger.info("COLLECTION: No food visible, wandering...")
                if randint(0, 5) == 0:
                    self.command_queue.append("Left")
                self.command_queue.append("Forward")
                return
        
        # Priority 2: Collect stones needed for elevation
        needed_resource = self.find_needed_resource()
        if needed_resource is not None:
            logger.info(f"COLLECTION: Looking for needed resource: {needed_resource}")
            resource_tile = self.find_item_in_vision(needed_resource)
            if resource_tile is not None:
                logger.info(f"COLLECTION: Found {needed_resource}, collecting from tile {resource_tile}")
                self.navigate_to_tile(resource_tile, needed_resource)
                return
            else:
                logger.info(f"COLLECTION: {needed_resource} not visible, exploring...")
                if randint(0, 5) == 0:
                    self.command_queue.append("Left")
                self.command_queue.append("Forward")
                return
        
        # Priority 3: Collect any stone for general stockpile
        logger.info("COLLECTION: Looking for any collectible stones")
        for stone in ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]:
            stone_tile = self.find_item_in_vision(stone)
            if stone_tile is not None:
                logger.info(f"COLLECTION: Found {stone} for stockpile, collecting from tile {stone_tile}")
                self.navigate_to_tile(stone_tile, stone)
                return
        
        logger.info("COLLECTION: No resources visible, continuing exploration")
        # Wander to find resources
        if randint(0, 3) == 0:
            self.command_queue.append("Left")
        self.command_queue.append("Forward")

    def update_inventory(self, inventory_list):
        """Update player inventory and return if changed."""
        logger.info(f"INVENTORY: Updating from server response: {inventory_list}")
        changed = False
        
        for item in inventory_list:
            if len(item) >= 2:
                resource_name = item[0]
                new_amount = int(item[1])
                old_amount = self.inventory.get_resource(resource_name)
                
                if old_amount != new_amount:
                    changed = True
                    logger.info(f"INVENTORY: {resource_name} changed: {old_amount} -> {new_amount}")
                
                self.inventory.set_resource(resource_name, new_amount)
        
        if changed:
            logger.info(f"INVENTORY: Updated - {self.inventory.to_string()}")
        
        self.team_inventories[max(0, self.player_id - 1)].set_from_list(inventory_list)
        return changed

    def finalize_turn(self):
        """Finalize turn with inventory check and look."""
        logger.debug("TURN: Finalizing turn")
        
        if self.inventory_updated:
            logger.debug("TURN: Requesting inventory update")
            self.command_queue.append("Inventory")
            self.inventory_updated = False
        
        if self.inventory_data:
            if self.update_inventory(self.inventory_data):
                logger.info("TURN: Inventory changed, broadcasting to team")
                self.broadcast_inventory()
        
        logger.debug("TURN: Adding Look command")
        self.command_queue.append("Look")

    def spawn_new_player(self):
        """Handle player forking logic."""
        logger.debug("SPAWN: Checking if should spawn new player")
        
        if (self.inventory.get_resource("food") < 20 or 
            self.enough_players or self.player_id != 0):
            logger.debug(f"SPAWN: Not spawning (food={self.inventory.get_resource('food')}, enough_players={self.enough_players}, player_id={self.player_id})")
            return
        
        if self.available_slots != 0:
            logger.info("SPAWN: Spawning new AI process")
            subprocess.Popen([
                "./zappy_ai", "-p", str(self.server_port),
                "-n", self.team_name, "-h", str(self.server_host)
            ])
            self.player_id = 1
        else:
            logger.info("SPAWN: Forking on server")
            self.command_queue.append("Fork")

    def process_broadcast_messages(self, messages: List[str]):
        """Process received broadcast messages."""
        logger.debug(f"BROADCAST: Processing {len(messages)} messages")
        
        for message in messages:
            try:
                parts = message.split(", ", 1)
                if len(parts) != 2:
                    continue
                    
                direction = parts[0]
                content = parts[1].split("\n")[0]
                decrypted = self.decrypt_message(content)
                
                if decrypted == "error":
                    logger.debug("BROADCAST: Message from other team, ignoring")
                    continue
                
                sender_id = int(decrypted.split("|", 1)[0])
                message_content = decrypted.split("|", 1)[1]
                
                logger.info(f"BROADCAST: Received from player {sender_id}: {message_content}")
                
                if "ready" in message_content:
                    logger.info("BROADCAST: Team is ready!")
                    self.enough_players = True
                    if self.player_id == 0:
                        self.player_id = 1
                
                if "here" in message_content:
                    logger.info("BROADCAST: Another player announced presence")
                    self.player_id += 1
                    if self.player_id == 6:
                        ready_msg = self.encrypt_message("ready")
                        self.command_queue.append(f"Broadcast {ready_msg}")
                        logger.info("BROADCAST: Team full, sending ready signal")
                
                # Handle other message types...
                
            except Exception as e:
                logger.error(f"BROADCAST: Error processing message: {e}")

    def process_command_responses(self, responses: List[str]):
        """Process responses from sent commands."""
        logger.debug(f"RESPONSE: Processing {len(responses)} command responses")
        
        for response in responses:
            try:
                command_parts = response.split("|", 1)
                if len(command_parts) != 2:
                    continue
                    
                command = command_parts[0]
                result = command_parts[1]
                
                logger.debug(f"RESPONSE: {command} -> {result}")
                
                if command == "Connect_nbr":
                    self.available_slots = int(result)
                    logger.info(f"RESPONSE: Available slots: {self.available_slots}")
                elif command == "Look":
                    self.vision_data = GameUtils.parse_string_to_list(result)
                    logger.info(f"RESPONSE: Updated vision with {len(self.vision_data)} tiles")
                elif command == "Inventory":
                    self.inventory_data = GameUtils.parse_string_to_list(result)
                    logger.info(f"RESPONSE: Received inventory data")
                elif command in ["Forward", "Right", "Left"]:
                    self.movement_done = True
                    logger.info(f"RESPONSE: Movement command {command} completed")
                elif command == "Take":
                    if result == "ok":
                        logger.info(f"RESPONSE: Successfully took item")
                    else:
                        logger.warning(f"RESPONSE: Failed to take item: {result}")
                        
            except Exception as e:
                logger.error(f"RESPONSE: Error processing response: {e}")

    def initialize_player(self):
        """Initialize player on first turn."""
        logger.info("INIT: Initializing player for first turn")
        
        # Ensure we have a player ID
        if self.player_id == 0:
            self.player_id = 1
            logger.info("INIT: Set default player_id to 1")
        
        here_msg = self.encrypt_message("here")
        self.command_queue.extend([
            f"Broadcast {here_msg}",
            "Inventory",
            "Look",
            "Connect_nbr"
        ])
        
        logger.info("INIT: First turn commands queued")

    def process_turn(self, responses: List[str], messages: List[str]) -> List[str]:
        """Main game logic processing."""
        logger.info(f"TURN: Starting new turn (level {self.level}, player_id {self.player_id})")
        self.command_queue = []
        
        # First turn initialization
        if self.first_turn:
            logger.info("TURN: First turn detected")
            self.initialize_player()
            self.first_turn = False
            logger.info(f"TURN: Generated {len(self.command_queue)} first-turn commands")
            return self.command_queue
        
        # Process command responses and messages
        logger.debug("TURN: Processing responses and messages")
        self.process_command_responses(responses)
        self.process_broadcast_messages(messages)
        
        # Handle elevation if applicable
        # self.handle_elevation()  # Commented out for now
        
        # Normal gameplay when not elevating
        if not self.is_elevating:
            logger.info("TURN: Normal gameplay mode")
            self.spawn_new_player()
            self.collect_resources()
            self.finalize_turn()
        else:
            logger.info("TURN: Elevation mode - skipping normal actions")
        
        # Reset turn state
        self.inventory_data = []
        self.movement_done = False
        self.command_queue.append("Connect_nbr")
        
        logger.info(f"TURN: Generated {len(self.command_queue)} commands: {self.command_queue}")
        return self.command_queue