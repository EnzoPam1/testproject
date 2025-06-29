/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Main entry point
*/

#include "Game.hpp"
#include <iostream>
#include <cstring>

void printUsage(const char* program) {
    std::cout << "USAGE: " << program << " -p port -h machine\n";
    std::cout << "       -p port     port number\n";
    std::cout << "       -h machine  hostname of the server\n";
}

int main(int argc, char* argv[]) {
    // Parse arguments
    int port = 0;
    std::string host = "localhost";
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    if (port == 0) {
        std::cerr << "Error: port number required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Create and run game
    try {
        Game game;
        
        if (!game.Initialize(host, port)) {
            std::cerr << "Failed to initialize game\n";
            return 1;
        }
        
        std::cout << "Connected to " << host << ":" << port << "\n";
        std::cout << "Controls:\n";
        std::cout << "  - WASD/Arrow keys: Move camera\n";
        std::cout << "  - Mouse wheel: Zoom\n";
        std::cout << "  - Right click + drag: Rotate camera\n";
        std::cout << "  - Left click: Select player\n";
        std::cout << "  - ESC: Exit\n\n";
        
        game.Run();
        game.Shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
} 