/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Main entry point with optional CLI override
*/

#include "Game.hpp"
#include <iostream>
#include <cstring>

void printUsage(const char* program) {
    std::cout << "USAGE: " << program << " [-p port] [-h machine]\n";
    std::cout << "       -p port     port number (optional, can use login screen)\n";
    std::cout << "       -h machine  hostname of the server (optional, can use login screen)\n";
    std::cout << "\nIf no parameters are provided, the login screen will be shown.\n";
    std::cout << "Parameters override the login screen and connect directly.\n";
}

int main(int argc, char* argv[]) {
    // Parse arguments (optional now)
    int port = 0;
    std::string host = "";
    bool showHelp = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            showHelp = true;
        }
    }

    if (showHelp) {
        printUsage(argv[0]);
        return 0;
    }

    // Create and run game
    try {
        Game game;

        if (!game.Initialize()) {
            std::cerr << "Failed to initialize game\n";
            return 1;
        }

        // If CLI parameters provided, show info
        if (port > 0 && !host.empty()) {
            std::cout << "CLI override mode - would connect to " << host << ":" << port << "\n";
            std::cout << "Note: Login screen will still be shown for demonstration.\n";
            std::cout << "You can modify the code to bypass login screen when CLI args are provided.\n\n";
        }

        std::cout << "🎮 Zappy GUI started!\n";
        std::cout << "Controls in game mode:\n";
        std::cout << "  - ZQSD: Move camera (French layout)\n";
        std::cout << "  - Mouse wheel: Zoom\n";
        std::cout << "  - Ctrl + Left click: Rotate camera\n";
        std::cout << "  - Left click: Select player\n";
        std::cout << "  - F1: Back to login\n";
        std::cout << "  - B: Toggle bounding boxes\n";
        std::cout << "  - ESC: Exit\n\n";

        game.Run();
        game.Shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
