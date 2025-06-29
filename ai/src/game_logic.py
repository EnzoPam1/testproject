##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## game_logic - VERSION CORRIGÉE AVEC MACHINE À ÉTATS
##

from enum import Enum
from typing import Tuple, List, Dict
import subprocess
import time
import random
import logging
from game_utils import GameUtils

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class AIState(Enum):
    INITIALISATION = "initialisation"
    EVALUATION = "evaluation"
    SURVIE = "survie"
    COLLECTE = "collecte"
    COORDINATION = "coordination"
    ELEVATION = "elevation"
    EXPLORATION = "exploration"

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

    def move_north(self):
        self.y = (self.y - 1) % self.map_height

    def move_south(self):
        self.y = (self.y + 1) % self.map_height

    def move_west(self):
        self.x = (self.x - 1) % self.map_width

    def move_east(self):
        self.x = (self.x + 1) % self.map_width

# Exigences pour chaque niveau d'élévation
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
    [1, 3], [4, 8], [9, 15], [16, 24], [25, 35], [36, 48], [49, 63], [64, 80]
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
        setattr(self, resource, getattr(self, resource, 0) + amount)

    def remove_resource(self, resource: str, amount: int):
        current = getattr(self, resource, 0)
        if current < amount:
            raise ValueError(f"Pas assez de {resource}")
        setattr(self, resource, current - amount)

    def get_resource(self, resource: str):
        return getattr(self, resource, 0)

    def to_string(self):
        return (f"food {self.food}, linemate {self.linemate}, "
                f"deraumere {self.deraumere}, sibur {self.sibur}, "
                f"mendiane {self.mendiane}, phiras {self.phiras}, "
                f"thystame {self.thystame}")

    def set_resource(self, resource: str, amount: int):
        if hasattr(self, resource):
            setattr(self, resource, int(amount))
        else:
            raise ValueError(f"Type de ressource invalide: {resource}")

    def set_from_list(self, inventory_list):
        for i, resource in enumerate(RESOURCE_TYPES):
            if i < len(inventory_list) and len(inventory_list[i]) >= 2:
                self.set_resource(resource, inventory_list[i][1])

class DetecteurBoucle:
    def __init__(self, longueur_pattern=3, max_repetitions=3):
        self.historique_commandes = []
        self.longueur_pattern = longueur_pattern
        self.max_repetitions = max_repetitions
        self.boucle_detectee = False
        self.max_historique = longueur_pattern * max_repetitions

    def ajouter_commande(self, commande):
        self.historique_commandes.append(commande)
        if len(self.historique_commandes) > self.max_historique:
            self.historique_commandes.pop(0)
        
        if len(self.historique_commandes) >= self.longueur_pattern * 2:
            self.boucle_detectee = self.detecter_pattern()
        
        return self.boucle_detectee

    def detecter_pattern(self):
        if len(self.historique_commandes) < self.longueur_pattern * 2:
            return False
        
        pattern = self.historique_commandes[-self.longueur_pattern:]
        pattern_precedent = self.historique_commandes[-self.longueur_pattern*2:-self.longueur_pattern]
        
        return pattern == pattern_precedent

    def casser_boucle(self):
        logger.warning("Boucle détectée! Cassage du pattern...")
        self.historique_commandes.clear()
        self.boucle_detectee = False
        
        # Forcer une action aléatoire pour casser la boucle
        actions = ["Forward", "Left", "Right", "Broadcast AIDE"]
        return random.choice(actions)

class GameLogic:
    def __init__(self, team_name: str, server_host, server_port):
        # Identificateurs
        self.player_id = 0
        self.level = 1
        self.team_name = team_name
        self.server_host = server_host
        self.server_port = server_port
        
        # État et inventaires
        self.state = AIState.INITIALISATION
        self.inventory = Inventory()
        self.team_inventories = [Inventory() for _ in range(7)]  # 6 players + total
        self.orientation = Orientation.NORTH
        
        # Données de jeu
        self.available_slots = 0
        self.vision_data = []
        self.inventory_data = []
        
        # Gestion des états et temporisation
        self.last_state_change = time.time()
        self.assessment_cycles = 0
        self.command_queue = []
        
        # Seuils critiques
        self.food_threshold_critical = 5
        self.food_threshold_safe = 20
        
        # Coordination d'équipe
        self.enough_players = False
        self.is_elevating = False
        self.broadcast_cooldown = {}
        
        # Détection de boucle
        self.detecteur_boucle = DetecteurBoucle()
        
        # Flags
        self.first_turn = True
        self.inventory_updated = False
        self.movement_done = False

    def transition_vers(self, nouvel_etat):
        """Transition d'état avec logging"""
        if nouvel_etat != self.state:
            logger.info(f"Transition d'état: {self.state.value} → {nouvel_etat.value}")
            self.state = nouvel_etat
            self.last_state_change = time.time()
            self.assessment_cycles = 0

    def determiner_prochain_etat(self):
        """Logique de décision critique avec seuils clairs"""
        current_food = self.inventory.get_resource("food")
        
        # Priorité 1: Survie critique
        if current_food < self.food_threshold_critical:
            return AIState.SURVIE
            
        # Priorité 2: Vérification de l'élévation
        if self.peut_tenter_elevation():
            return AIState.COORDINATION
            
        # Priorité 3: Collection de ressources
        ressources_manquantes = self.get_ressources_manquantes()
        if ressources_manquantes and current_food > self.food_threshold_safe:
            return AIState.COLLECTE
            
        # Priorité 4: Collecte de nourriture préventive
        if current_food < self.food_threshold_safe:
            return AIState.SURVIE
            
        # Par défaut: Explorer
        return AIState.EXPLORATION

    def forcer_progression_etat(self):
        """Force la progression d'état après timeout"""
        temps_ecoule = time.time() - self.last_state_change
        
        if temps_ecoule > 30:  # 30 secondes max par état
            logger.warning(f"Timeout d'état ({temps_ecoule:.1f}s), progression forcée")
            
            if self.state == AIState.EVALUATION:
                self.transition_vers(AIState.EXPLORATION)
            elif self.state == AIState.EXPLORATION:
                self.transition_vers(AIState.COLLECTE)
            elif self.state == AIState.COLLECTE:
                self.transition_vers(AIState.EXPLORATION)
            else:
                self.transition_vers(AIState.EVALUATION)

    def evaluer_situation(self):
        """Évaluation avec conditions de sortie claires"""
        self.command_queue.extend(["Inventory", "Look", "Connect_nbr"])
        self.assessment_cycles += 1
        
        # Déterminer le prochain état
        prochain_etat = self.determiner_prochain_etat()
        
        # Sortir de l'évaluation après 2 cycles max
        if prochain_etat != AIState.EVALUATION or self.assessment_cycles >= 2:
            self.transition_vers(prochain_etat)

    def assurer_survie(self):
        """Collecte prioritaire de nourriture"""
        logger.info("Mode survie: recherche de nourriture")
        
        # Chercher de la nourriture dans la vision
        nourriture_trouvee = self.find_item_in_vision("food")
        
        if nourriture_trouvee is not None:
            if nourriture_trouvee == 0:
                # Nourriture sur la case actuelle
                self.command_queue.append("Take food")
            else:
                # Se déplacer vers la nourriture
                self.naviguer_vers_case(nourriture_trouvee, "food")
        else:
            # Explorer pour trouver de la nourriture
            self.explorer_aleatoire()
        
        # Retourner à l'évaluation après action
        self.transition_vers(AIState.EVALUATION)

    def collecter_ressources(self):
        """Collection intelligente de ressources"""
        ressources_manquantes = self.get_ressources_manquantes()
        
        if not ressources_manquantes:
            logger.info("Toutes les ressources nécessaires collectées")
            self.transition_vers(AIState.COORDINATION)
            return
        
        # Chercher la ressource la plus prioritaire
        for ressource in ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]:
            if ressource in ressources_manquantes:
                position = self.find_item_in_vision(ressource)
                if position is not None:
                    logger.info(f"Collecte de {ressource} à la position {position}")
                    if position == 0:
                        self.command_queue.append(f"Take {ressource}")
                    else:
                        self.naviguer_vers_case(position, ressource)
                    self.transition_vers(AIState.EVALUATION)
                    return
        
        # Aucune ressource nécessaire visible, explorer
        self.explorer_aleatoire()
        self.transition_vers(AIState.EVALUATION)

    def coordonner_equipe(self):
        """Coordination d'équipe pour l'élévation"""
        logger.info("Coordination d'équipe pour élévation")
        
        # Diffuser l'état de préparation
        message_crypt = self.encrypt_message(f"PRET_LVL_{self.level}")
        self.command_queue.append(f"Broadcast {message_crypt}")
        
        # Vérifier si assez de joueurs sont prêts
        if self.enough_players or self.player_id >= 6:
            self.transition_vers(AIState.ELEVATION)
        else:
            # Attendre plus de joueurs
            self.transition_vers(AIState.EXPLORATION)

    def tenter_elevation(self):
        """Tentative d'élévation"""
        logger.info(f"Tentative d'élévation niveau {self.level}")
        
        # Déposer les ressources nécessaires
        self.deposer_ressources_elevation()
        
        # Vérifier les conditions et lancer l'incantation
        if self.peut_performer_elevation():
            self.command_queue.append("Incantation")
        
        self.transition_vers(AIState.EVALUATION)

    def explorer(self):
        """Exploration de la carte"""
        # Mouvement aléatoire occasionnel
        if random.randint(0, 3) == 0:
            actions = ["Left", "Right"]
            self.command_queue.append(random.choice(actions))
        
        self.command_queue.append("Forward")
        self.transition_vers(AIState.EVALUATION)

    def explorer_aleatoire(self):
        """Exploration aléatoire pour sortir des situations bloquées"""
        if random.randint(0, 4) == 0:
            self.command_queue.append(random.choice(["Left", "Right"]))
        self.command_queue.append("Forward")

    def peut_tenter_elevation(self):
        """Vérification si l'élévation est possible"""
        if self.level >= 8:
            return False
        
        ressources_manquantes = self.get_ressources_manquantes()
        return len(ressources_manquantes) == 0 and self.enough_players

    def get_ressources_manquantes(self):
        """Calcule les ressources manquantes pour l'élévation"""
        if self.level >= 8:
            return {}
        
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        manquantes = {}
        
        for i, ressource in enumerate(["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]):
            requis = requirements[i + 1]  # +1 car index 0 = nb_players
            actuel = self.inventory.get_resource(ressource)
            if actuel < requis:
                manquantes[ressource] = requis - actuel
        
        return manquantes

    def peut_performer_elevation(self):
        """Vérifie si l'élévation peut être exécutée immédiatement"""
        if not self.vision_data:
            return False
        
        # Compter les joueurs et ressources sur la case actuelle
        player_count = 0
        resources = {r: 0 for r in ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]}
        
        for item in self.vision_data[0]:
            if "player" in item:
                player_count += 1
            else:
                for resource in resources:
                    if resource in item:
                        resources[resource] += 1
        
        # Vérifier les exigences
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        return (player_count >= requirements[0] and
                all(resources[r] >= requirements[i+1] 
                    for i, r in enumerate(["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"])))

    def deposer_ressources_elevation(self):
        """Dépose les ressources nécessaires pour l'élévation"""
        if self.level >= 8:
            return
        
        requirements = ELEVATION_REQUIREMENTS[self.level - 1]
        for i, ressource in enumerate(["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]):
            requis = requirements[i + 1]
            for _ in range(requis):
                if self.inventory.get_resource(ressource) > 0:
                    self.command_queue.append(f"Set {ressource}")

    def find_item_in_vision(self, item: str):
        """Trouve un objet dans la vision"""
        for i, tile_data in enumerate(self.vision_data):
            if any(item in str(data) for data in tile_data):
                return i
        return None

    def naviguer_vers_case(self, tile_position: int, target_item: str):
        """Navigation vers une case spécifique"""
        if tile_position == 0:
            # Objet sur la case actuelle
            self.command_queue.append(f"Take {target_item}")
            return
        
        # Logique de navigation simplifiée
        self.command_queue.append("Forward")

    def encrypt_message(self, message: str):
        """Chiffre un message avec le nom d'équipe comme clé"""
        return GameUtils.encrypt_and_compress(str(self.player_id) + "|" + message, self.team_name)

    def decrypt_message(self, message: str):
        """Déchiffre un message"""
        return GameUtils.decrypt_and_decompress(message, self.team_name)

    def parse_vision_data(self, content: str):
        """Parse les données de vision"""
        content = content.split(",")
        self.vision_data = GameUtils.parse_string_to_list(content)

    def update_inventory(self, inventory_list):
        """Met à jour l'inventaire du joueur"""
        changed = False
        for item in inventory_list:
            if len(item) >= 2:
                old_value = self.inventory.get_resource(item[0])
                new_value = int(item[1])
                if old_value != new_value:
                    changed = True
                self.inventory.set_resource(item[0], new_value)
        
        if self.player_id > 0:
            self.team_inventories[self.player_id - 1].set_from_list(inventory_list)
        return changed

    def process_command_responses(self, responses: List[str]):
        """Traite les réponses des commandes envoyées"""
        for response in responses:
            if "|" in response:
                command_parts = response.split("|", 1)
                command = command_parts[0]
                result = command_parts[1] if len(command_parts) > 1 else ""
                
                if command == "Connect_nbr":
                    self.available_slots = int(result)
                elif command == "Look":
                    self.vision_data = GameUtils.parse_string_to_list(result)
                elif command == "Inventory":
                    inventory_parsed = GameUtils.parse_string_to_list(result)
                    self.update_inventory(inventory_parsed)
                elif command == "Incantation":
                    self.is_elevating = False
                    if "Current level:" in result:
                        self.level = int(result.split(":")[1].strip())
                        logger.info(f"Élévation réussie! Nouveau niveau: {self.level}")

    def process_broadcast_messages(self, messages: List[str]):
        """Traite les messages de diffusion reçus"""
        for message in messages:
            try:
                if ", " in message:
                    direction = message.split(", ")[0]
                    content = message.split(", ")[1].strip()
                    decrypted = self.decrypt_message(content)
                    
                    if decrypted != "error" and "|" in decrypted:
                        sender_id = int(decrypted.split("|", 1)[0])
                        message_content = decrypted.split("|", 1)[1]
                        
                        if "PRET_LVL_" in message_content:
                            self.enough_players = True
                            logger.info(f"Joueur {sender_id} prêt pour élévation")
                        elif "here" in message_content:
                            self.player_id += 1
                            if self.player_id >= 6:
                                ready_msg = self.encrypt_message("ready")
                                self.command_queue.append(f"Broadcast {ready_msg}")
            except Exception as e:
                logger.warning(f"Erreur traitement message: {e}")

    def initialize_player(self):
        """Initialise le joueur au premier tour"""
        here_msg = self.encrypt_message("here")
        self.command_queue.extend([
            f"Broadcast {here_msg}",
            "Inventory",
            "Look",
            "Connect_nbr"
        ])
        self.transition_vers(AIState.EVALUATION)

    def process_turn(self, responses: List[str], messages: List[str]) -> List[str]:
        """Traitement principal d'un tour de jeu avec machine à états"""
        self.command_queue = []
        
        # Détection de boucle
        if responses:
            derniere_commande = responses[-1].split("|")[0] if "|" in responses[-1] else responses[-1]
            if self.detecteur_boucle.ajouter_commande(derniere_commande):
                action_cassage = self.detecteur_boucle.casser_boucle()
                self.command_queue.append(action_cassage)
                return self.command_queue
        
        # Premier tour
        if self.first_turn:
            self.initialize_player()
            self.first_turn = False
            return self.command_queue
        
        # Traitement des réponses et messages
        self.process_command_responses(responses)
        self.process_broadcast_messages(messages)
        
        # Vérifier le timeout d'état
        self.forcer_progression_etat()
        
        # Exécution basée sur l'état actuel
        if self.state == AIState.EVALUATION:
            self.evaluer_situation()
        elif self.state == AIState.SURVIE:
            self.assurer_survie()
        elif self.state == AIState.COLLECTE:
            self.collecter_ressources()
        elif self.state == AIState.COORDINATION:
            self.coordonner_equipe()
        elif self.state == AIState.ELEVATION:
            self.tenter_elevation()
        elif self.state == AIState.EXPLORATION:
            self.explorer()
        
        # Reset des données temporaires
        self.inventory_data = []
        self.movement_done = False
        
        # Ajouter Connect_nbr à la fin pour surveiller les slots
        if self.command_queue and self.command_queue[-1] != "Connect_nbr":
            self.command_queue.append("Connect_nbr")
        
        return self.command_queue