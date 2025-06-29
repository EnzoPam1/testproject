#!/usr/bin/env python3

##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## main - VERSION CORRIGÉE AVEC GESTION D'ERREURS
##

import sys
import argparse
import logging
import signal
import time
from network_handler import NetworkHandler

# Configuration du logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler(f'zappy_ai_{int(time.time())}.log')
    ]
)

logger = logging.getLogger(__name__)

def parse_arguments():
    """Parse les arguments de ligne de commande avec validation."""
    parser = argparse.ArgumentParser(
        prog='./zappy_ai',
        description='Client IA Zappy - Contrôle un joueur automatiquement', 
        add_help=False
    )
    
    parser.add_argument('-p', '--port', type=int, required=True, 
                       help='Numéro de port du serveur')
    parser.add_argument('-n', '--name', type=str, required=True, 
                       help='Nom de l\'équipe')
    parser.add_argument('-h', '--host', type=str, default='localhost', 
                       help='Nom d\'hôte du serveur (défaut: localhost)')
    parser.add_argument('--help', action='help', 
                       help='Affiche ce message d\'aide et quitte')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Mode verbeux pour plus de logs')
    parser.add_argument('--debug', action='store_true',
                       help='Mode debug avec logs détaillés')
    
    return parser.parse_args()

def validate_arguments(args):
    """Valide les arguments parsés."""
    errors = []
    
    # Validation du port
    if args.port < 1 or args.port > 65535:
        errors.append(f"Port invalide: {args.port} (doit être entre 1 et 65535)")
    
    # Validation du nom d'équipe
    if not args.name or len(args.name.strip()) == 0:
        errors.append("Nom d'équipe ne peut pas être vide")
    elif len(args.name) > 50:
        errors.append(f"Nom d'équipe trop long: {len(args.name)} caractères (max 50)")
    elif args.name.upper() == "GRAPHIC":
        errors.append("Le nom 'GRAPHIC' est réservé pour la GUI")
    
    # Validation du nom d'hôte
    if not args.host or len(args.host.strip()) == 0:
        errors.append("Nom d'hôte ne peut pas être vide")
    
    if errors:
        logger.error("Erreurs de validation des arguments:")
        for error in errors:
            logger.error(f"  - {error}")
        return False
    
    return True

def setup_signal_handlers(ai_client):
    """Configure les gestionnaires de signaux pour un arrêt propre."""
    def signal_handler(signum, frame):
        logger.info(f"Signal {signum} reçu, arrêt de l'IA...")
        if ai_client:
            ai_client.is_running = False
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)   # Ctrl+C
    signal.signal(signal.SIGTERM, signal_handler)  # Terminaison
    
    # Sur Unix, gérer aussi SIGHUP
    if hasattr(signal, 'SIGHUP'):
        signal.signal(signal.SIGHUP, signal_handler)

def print_startup_info(args):
    """Affiche les informations de démarrage."""
    logger.info("=" * 50)
    logger.info("DÉMARRAGE CLIENT IA ZAPPY")
    logger.info("=" * 50)
    logger.info(f"Équipe: {args.name}")
    logger.info(f"Serveur: {args.host}:{args.port}")
    logger.info(f"Mode verbeux: {'Activé' if args.verbose else 'Désactivé'}")
    logger.info(f"Mode debug: {'Activé' if args.debug else 'Désactivé'}")
    logger.info("=" * 50)

def main():
    """Fonction principale avec gestion d'erreurs complète."""
    ai_client = None
    
    try:
        # Parse et validation des arguments
        args = parse_arguments()
        
        # Configuration du niveau de logging
        if args.debug:
            logging.getLogger().setLevel(logging.DEBUG)
        elif args.verbose:
            logging.getLogger().setLevel(logging.INFO)
        else:
            logging.getLogger().setLevel(logging.WARNING)
        
        # Validation des arguments
        if not validate_arguments(args):
            sys.exit(1)
        
        # Affichage des informations de démarrage
        print_startup_info(args)
        
        # Création et démarrage de l'IA
        logger.info("Initialisation du client IA...")
        ai_client = NetworkHandler(args.name, args.port, args.host)
        
        # Configuration des gestionnaires de signaux
        setup_signal_handlers(ai_client)
        
        logger.info("Démarrage du client IA...")
        ai_client.start()
        
        # Si on arrive ici, c'est que l'IA s'est arrêtée normalement
        logger.info("Client IA arrêté normalement")
        
    except KeyboardInterrupt:
        logger.info("Interruption clavier (Ctrl+C) détectée")
        if ai_client:
            ai_client.is_running = False
        sys.exit(0)
        
    except ConnectionRefusedError:
        logger.error(f"Impossible de se connecter au serveur {args.host}:{args.port}")
        logger.error("Vérifiez que le serveur est démarré et accessible")
        sys.exit(2)
        
    except OSError as e:
        if e.errno == 111:  # Connection refused
            logger.error(f"Connexion refusée par {args.host}:{args.port}")
            logger.error("Le serveur n'est peut-être pas démarré")
        elif e.errno == 113:  # No route to host
            logger.error(f"Impossible de joindre l'hôte {args.host}")
            logger.error("Vérifiez l'adresse du serveur")
        else:
            logger.error(f"Erreur réseau: {e}")
        sys.exit(2)
        
    except ValueError as e:
        logger.error(f"Erreur de valeur: {e}")
        logger.error("Vérifiez les paramètres de connexion")
        sys.exit(1)
        
    except Exception as e:
        logger.error(f"Erreur inattendue: {e}")
        logger.debug("Détails de l'erreur:", exc_info=True)
        sys.exit(3)
        
    finally:
        # Nettoyage final
        if ai_client:
            try:
                ai_client.is_running = False
                logger.info("Nettoyage des ressources...")
            except:
                pass
        
        logger.info("Fin du programme")

if __name__ == "__main__":
    main()