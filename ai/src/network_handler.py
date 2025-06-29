##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## network_handler - VERSION CORRIGÉE AVEC GESTION ROBUSTE DES COMMUNICATIONS
##

import socket
import threading
import sys
import os
import time
import logging
from collections import deque

# Configuration du logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[logging.StreamHandler(sys.stdout)]
)
logger = logging.getLogger(__name__)

sys.path.insert(0, os.path.dirname(__file__))

from game_logic import GameLogic

class ErreurProtocole(Exception):
    """Exception pour les erreurs de protocole"""
    pass

class ErreurTimeout(Exception):
    """Exception pour les timeouts"""
    pass

class NetworkHandler:
    def __init__(self, team_name, port, host="localhost"):
        """Initialise le gestionnaire réseau avec gestion d'erreurs robuste."""
        self.game_logic = None
        
        # Données d'équipe
        self.team_name = team_name
        self.world_width = 0
        self.world_height = 0
        
        # Configuration réseau
        self.server_address = (host, port)
        self.network_thread = threading.Thread(target=self._network_task, daemon=True)
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.available_slots = 0
        
        # Synchronisation des threads
        self.data_lock = threading.Lock()
        self.is_connected = False
        self.is_running = True
        self.receive_buffer = b''
        self.message_ready = threading.Event()
        
        # File de messages avec limite
        self.message_queue = deque(maxlen=100)
        
        # Gestion des timeouts et reconnexions
        self.connection_timeout = 30
        self.response_timeout = 10
        self.max_reconnect_attempts = 3
        self.reconnect_delay = 5
        
        # Statistiques et debugging
        self.messages_sent = 0
        self.messages_received = 0
        self.last_activity = time.time()
        
        # Démarrer le thread réseau
        self.network_thread.start()
    
    def _network_task(self):
        """Tâche principale du thread réseau avec gestion de reconnexion."""
        attempt = 0
        
        while self.is_running and attempt < self.max_reconnect_attempts:
            try:
                self._connect_and_run()
                break  # Connexion réussie, sortir de la boucle
            except Exception as e:
                attempt += 1
                logger.error(f"Tentative de connexion {attempt}/{self.max_reconnect_attempts} échouée: {e}")
                
                if attempt < self.max_reconnect_attempts:
                    logger.info(f"Nouvelle tentative dans {self.reconnect_delay} secondes...")
                    time.sleep(self.reconnect_delay)
                else:
                    logger.error("Impossible de se connecter après toutes les tentatives")
                    self.is_running = False
    
    def _connect_and_run(self):
        """Connexion et boucle principale avec gestion d'erreurs."""
        try:
            # Connexion au serveur
            with self.data_lock:
                try:
                    logger.info(f"Connexion à {self.server_address[0]}:{self.server_address[1]}")
                    self.client_socket.settimeout(self.connection_timeout)
                    self.client_socket.connect(self.server_address)
                    logger.info("Connecté au serveur")
                except Exception as e:
                    logger.error(f"Erreur de connexion: {e}")
                    raise
                
                # Séquence de bienvenue
                welcome_msg = self._receive_complete_message()
                if not welcome_msg or welcome_msg.strip() != "WELCOME":
                    raise ErreurProtocole(f"Message de bienvenue invalide: '{welcome_msg}'")
                
                logger.info(f"Bienvenue reçu: '{welcome_msg}'")
                
                # Envoi du nom d'équipe
                self._send_message(self.team_name)
                logger.info(f"Nom d'équipe envoyé: '{self.team_name}'")

                # Lecture de la réponse du nombre de clients
                client_response = self._receive_complete_message()
                if not client_response:
                    raise ErreurProtocole("Échec de réception du nombre de clients")
                
                logger.info(f"Réponse client reçue: '{client_response}'")
                
                try:
                    self.available_slots = int(client_response.strip())
                    if self.available_slots <= 0:
                        raise ErreurProtocole(f"Aucun slot disponible: {self.available_slots}")
                except ValueError as e:
                    raise ErreurProtocole(f"Erreur parsing nombre de clients '{client_response}': {e}")

                # Lecture des dimensions du monde
                world_response = self._receive_complete_message()
                if not world_response:
                    raise ErreurProtocole("Échec de réception des dimensions du monde")
                
                logger.info(f"Réponse monde reçue: '{world_response}'")
                
                try:
                    dimensions = world_response.strip().split()
                    if len(dimensions) == 2:
                        self.world_width, self.world_height = [int(x) for x in dimensions]
                        logger.info(f"Dimensions du monde: {self.world_width}x{self.world_height}")
                    else:
                        raise ErreurProtocole(f"Format des dimensions invalide: {world_response}")
                except ValueError as e:
                    raise ErreurProtocole(f"Erreur parsing dimensions '{world_response}': {e}")

                # Initialisation de la logique de jeu
                self.game_logic = GameLogic(
                    self.team_name, self.server_address[0], self.server_address[1]
                )
                
                self.is_connected = True
                logger.info("Initialisation terminée, démarrage de la boucle principale")
            
            # Configuration du socket pour le mode non-bloquant avec timeout
            self.client_socket.settimeout(0.5)
            
            # Boucle réseau principale
            while self.is_running and self.is_connected:
                try:
                    data = self.client_socket.recv(4096)
                    if not data:  # Connexion fermée
                        logger.info("Serveur a fermé la connexion")
                        break
                    
                    # Traitement des données reçues
                    with self.data_lock:
                        self.receive_buffer += data
                        self._process_buffer()
                        self.last_activity = time.time()
                        
                except socket.timeout:
                    # Timeout normal, vérifier la santé de la connexion
                    if time.time() - self.last_activity > 60:  # 60 secondes sans activité
                        logger.warning("Connexion inactive, test de connectivité")
                        try:
                            self._send_message("Connect_nbr")  # Commande de test
                        except:
                            logger.error("Test de connectivité échoué")
                            break
                    continue
                except Exception as e:
                    logger.error(f"Erreur réseau: {e}")
                    break
                    
        except Exception as e:
            logger.error(f"Erreur dans la tâche réseau: {e}")
        finally:
            self.is_connected = False
            self.is_running = False
            logger.info("Tâche réseau terminée")

    def _receive_complete_message(self):
        """Reçoit un message complet (bloquant) avec timeout."""
        temp_buffer = b''
        start_time = time.time()
        
        while time.time() - start_time < self.response_timeout:
            try:
                data = self.client_socket.recv(1024)
                if not data:
                    raise ErreurProtocole("Connexion fermée par le serveur")
                
                temp_buffer += data
                
                if b'\n' in temp_buffer:
                    line, remaining = temp_buffer.split(b'\n', 1)
                    # Remettre le reste dans le buffer principal
                    self.receive_buffer = remaining + self.receive_buffer
                    message = line.decode('utf-8').strip()
                    self.messages_received += 1
                    return message
                    
            except socket.timeout:
                continue
            except Exception as e:
                logger.error(f"Erreur lors de la réception du message: {e}")
                raise
        
        raise ErreurTimeout(f"Aucun message reçu dans les {self.response_timeout}s")

    def _process_buffer(self):
        """Traite le buffer de réception et extrait les messages complets."""
        while b'\n' in self.receive_buffer:
            line, self.receive_buffer = self.receive_buffer.split(b'\n', 1)
            message = line.decode('utf-8').strip()
            
            if message:  # Ajouter seulement les messages non-vides
                self.message_queue.append(message)
                self.messages_received += 1
                logger.debug(f"Message en file: '{message}'")
                self.message_ready.set()

    def _send_message(self, message):
        """Envoie un message avec format correct et gestion d'erreurs."""
        try:
            # S'assurer que le message n'a pas déjà un retour à la ligne
            message = message.strip()
            # Ajouter le retour à la ligne et encoder
            formatted_message = f"{message}\n".encode('utf-8')
            
            # Utiliser sendall pour assurer la transmission complète
            self.client_socket.sendall(formatted_message)
            self.messages_sent += 1
            self.last_activity = time.time()
            logger.debug(f"ENVOYÉ: '{message}'")
            return True
        except Exception as e:
            logger.error(f"Erreur envoi message '{message}': {e}")
            return False

    def get_next_message(self):
        """Récupère le prochain message complet de la file."""
        with self.data_lock:
            if self.message_queue:
                message = self.message_queue.popleft()
                if not self.message_queue:
                    self.message_ready.clear()
                return message
            return None

    def send_command(self, command):
        """Envoie une commande au serveur."""
        return self._send_message(command)

    def get_message_count(self):
        """Récupère le nombre de messages complets en file."""
        with self.data_lock:
            return len(self.message_queue)

    def wait_for_message(self, timeout=5):
        """Attend qu'un message soit disponible avec timeout."""
        if self.message_ready.wait(timeout):
            return self.get_next_message()
        return None

    def get_connection_stats(self):
        """Retourne les statistiques de connexion."""
        return {
            "connected": self.is_connected,
            "messages_sent": self.messages_sent,
            "messages_received": self.messages_received,
            "queue_size": len(self.message_queue),
            "last_activity": self.last_activity
        }

    def __del__(self):
        """Nettoyage du gestionnaire réseau."""
        self.is_running = False
        self.is_connected = False
        if hasattr(self, 'network_thread') and threading.current_thread() != self.network_thread:
            self.network_thread.join(timeout=2)
        try:
            self.client_socket.close()
        except:
            pass

    def start(self):
        """Démarre la boucle principale de l'IA avec gestion d'erreurs robuste."""
        # Vérification de la connexion
        connection_wait_time = 0
        while not self.is_connected and self.is_running and connection_wait_time < 30:
            time.sleep(0.5)
            connection_wait_time += 0.5
        
        if not self.is_connected:
            logger.error("Impossible de se connecter au serveur")
            return
        
        logger.info("Démarrage de la boucle principale de l'IA")
        
        received_responses = []
        broadcast_messages = []
        pending_commands = []
        
        # Compteurs pour éviter les boucles infinies
        tours_sans_progression = 0
        max_tours_sans_progression = 20
        
        while self.is_running and self.is_connected:
            try:
                # Traitement des réponses pour les commandes envoyées
                commands_processed = 0
                max_commands = min(len(pending_commands), 10)
                
                # Traitement des messages avec timeout adaptatif
                timeout_message = 2.0 if pending_commands else 0.5
                
                while commands_processed < max_commands:
                    server_message = None
                    
                    # Récupération de message avec gestion intelligente
                    if self.message_ready.is_set():
                        server_message = self.wait_for_message(timeout_message)
                    else:
                        temp_message = self.get_next_message()
                        if temp_message is None:
                            server_message = self.wait_for_message(timeout_message)
                        else:
                            server_message = temp_message
                    
                    if server_message is None:
                        logger.debug("Aucun message reçu dans le timeout")
                        break
                    
                    logger.debug(f"Traitement message serveur: '{server_message}'")
                    
                    # Gestion des messages de diffusion
                    if server_message.startswith("message "):
                        broadcast_part = server_message[8:]  # Retirer "message "
                        broadcast_messages.append(broadcast_part)
                        logger.debug(f"Diffusion ajoutée: '{broadcast_part}'")
                    elif "Elevation underway" in server_message:
                        received_responses.append("Incantation|" + server_message)
                        commands_processed += 1
                    elif "Current level" in server_message:
                        received_responses.append("Incantation|" + server_message)
                        commands_processed += 1
                    elif server_message.startswith("eject:"):
                        logger.info(f"Éjecté: {server_message}")
                        # Ce n'est pas une réponse de commande, ne pas l'incrémenter
                    elif server_message.startswith("dead"):
                        logger.error("Joueur mort!")
                        self.is_running = False
                        break
                    else:
                        # Réponse de commande régulière
                        if commands_processed < len(pending_commands):
                            command_sent = pending_commands[commands_processed].strip()
                            response_pair = f"{command_sent}|{server_message}"
                            received_responses.append(response_pair)
                            logger.debug(f"Réponse commande: '{response_pair}'")
                            commands_processed += 1
                        else:
                            logger.warning(f"Message serveur inattendu: '{server_message}'")
                
                # Nettoyage des commandes traitées
                if len(pending_commands) > 10:
                    pending_commands = pending_commands[10:]
                else:
                    pending_commands = []
                
                # Récupération de nouvelles commandes de la logique de jeu
                if not pending_commands:
                    try:
                        new_commands = self.game_logic.process_turn(received_responses, broadcast_messages)
                        if new_commands:
                            pending_commands = new_commands
                            logger.debug(f"Reçu {len(pending_commands)} nouvelles commandes")
                            tours_sans_progression = 0  # Reset du compteur
                        else:
                            tours_sans_progression += 1
                            
                        received_responses = []
                        broadcast_messages = []
                    except Exception as e:
                        logger.error(f"Erreur dans la logique de jeu: {e}")
                        tours_sans_progression += 1
                        time.sleep(1)
                        continue
                
                # Détection de blocage
                if tours_sans_progression >= max_tours_sans_progression:
                    logger.warning("Détection de blocage, redémarrage de la logique")
                    pending_commands = ["Look", "Inventory", "Forward"]
                    tours_sans_progression = 0
                
                # Envoi jusqu'à 10 commandes
                commands_to_send = min(len(pending_commands), 10)
                for i in range(commands_to_send):
                    if not self._send_message(pending_commands[i]):
                        logger.error(f"Échec envoi commande: {pending_commands[i]}")
                        break
                        
                # Suppression immédiate des commandes d'incantation après envoi
                for i in range(commands_to_send):
                    if i < len(pending_commands) and "Incantation" in pending_commands[i]:
                        pending_commands.pop(i)
                        break
                
                # Délai court pour éviter la surcharge
                time.sleep(0.05)
                
            except Exception as e:
                logger.error(f"Erreur dans la boucle principale: {e}")
                time.sleep(1)
        
        logger.info("Boucle principale de l'IA terminée")
        
        # Affichage des statistiques finales
        stats = self.get_connection_stats()
        logger.info(f"Statistiques finales: {stats}")