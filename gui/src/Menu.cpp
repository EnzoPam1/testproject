/*
** EPITECH PROJECT, 2025
** zappy_graphics
** File description:
** draw_menu
*/

#include "Menu.hpp"
#include <iostream> // pour std::cerr

Menu::Menu(sf::RenderWindow& win, AppState& state) 
    : window(win), currentState(state) {
    backgroundTexture.loadFromFile("assets/aoe.jpg");
    backgroundSprite.setTexture(backgroundTexture);

    sf::Vector2u textureSize = backgroundTexture.getSize();
    sf::Vector2u windowSize = window.getSize();
    backgroundSprite.setScale(
        static_cast<float>(windowSize.x) / textureSize.x,
        static_cast<float>(windowSize.y) / textureSize.y
    );

    if (!music.openFromFile("assets/aoe2.ogg")) {
        std::cerr << "Erreur : impossible de charger la musique." << std::endl;
    }
    music.setLoop(true);
    music.setVolume(50);
    music.play();

    font.loadFromFile("assets/font.ttf");

    createButtons();
    initBackButton();
    settingsPage = std::make_unique<SettingsPage>(font, musicVolume);

    // Chargement illustration
    if (!illustrationTexture.loadFromFile("assets/solo.jpg")) {
        std::cerr << "Erreur : impossible de charger l'illustration." << std::endl;
    }
    illustrationSprite.setTexture(illustrationTexture);
    illustrationSprite.setScale(0.5f, 0.5f);

    if (!buttons.empty()) {
        sf::Vector2f connectPos = buttons[0].getPosition();
        float illustrationWidth = illustrationSprite.getGlobalBounds().width;
        float centerX = connectPos.x + (buttons[0].getSize().x - illustrationWidth) / 2.f;
        float yAbove = connectPos.y - illustrationSprite.getGlobalBounds().height;
        illustrationSprite.setPosition(centerX, yAbove);
    }
}

void Menu::createButtons() {
    std::vector<std::string> buttonTexts = {
        "Connect", "Settings", "How to Play", "Credits", "Exit"
    };

    float buttonWidth = 362;
    float buttonHeight = 50;
    float spacing = 20;

    float startX = 40.0f;
    float currentY = 100.0f;  // Position de départ haute

    // Illustration 1 : au-dessus de "Connect"
    if (!illustrationTexture.loadFromFile("assets/solo.jpg")) {
        std::cerr << "Erreur : impossible de charger l'illustration 1" << std::endl;
    } else {
        illustrationSprite.setTexture(illustrationTexture);

        // Échelle dynamique pour correspondre à la largeur du bouton
        float scaleFactor = buttonWidth / illustrationTexture.getSize().x;
        illustrationSprite.setScale(scaleFactor, scaleFactor);

        sf::FloatRect illu1Bounds = illustrationSprite.getGlobalBounds();
        float illu1X = startX + (buttonWidth - illu1Bounds.width) / 2.f;
        illustrationSprite.setPosition(illu1X, currentY);
        currentY += illu1Bounds.height + spacing;
    }

    for (size_t i = 0; i < buttonTexts.size(); ++i) {
        // Illustration 2 : au-dessus de "Settings"
        if (buttonTexts[i] == "Settings") {
            if (!settingsIllustrationTexture.loadFromFile("assets/settings.png")) {
                std::cerr << "Erreur : impossible de charger l'illustration 2" << std::endl;
            } else {
                settingsIllustrationSprite.setTexture(settingsIllustrationTexture);

                float scaleFactor2 = buttonWidth / settingsIllustrationTexture.getSize().x;
                settingsIllustrationSprite.setScale(scaleFactor2, scaleFactor2);

                sf::FloatRect illu2Bounds = settingsIllustrationSprite.getGlobalBounds();
                float illu2X = startX + (buttonWidth - illu2Bounds.width) / 2.f;
                settingsIllustrationSprite.setPosition(illu2X, currentY);
                currentY += illu2Bounds.height + spacing;
            }
        }

        // Création du bouton
        sf::RectangleShape button(sf::Vector2f(buttonWidth, buttonHeight));
        button.setPosition(startX, currentY);
        button.setFillColor(sf::Color(80, 0, 0));
        button.setOutlineColor(sf::Color(212, 175, 55));
        button.setOutlineThickness(4);
        buttons.push_back(button);

        sf::Text text;
        text.setFont(font);
        text.setString(buttonTexts[i]);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color(255, 230, 140));

        sf::FloatRect textBounds = text.getLocalBounds();
        float textX = button.getPosition().x + (buttonWidth - textBounds.width) / 2.f - textBounds.left;
        float textY = button.getPosition().y + (buttonHeight - textBounds.height) / 2.f - textBounds.top;
        text.setPosition(textX, textY);
        labels.push_back(text);

        actions.push_back([this, textStr = buttonTexts[i]] {
            if (textStr == "Exit") window.close();
            else if (textStr == "Connect") currentState = AppState::CONNECT;
            else if (textStr == "Settings") currentState = AppState::SETTINGS;
            else if (textStr == "How to Play") currentState = AppState::HOW_TO_PLAY;
            else if (textStr == "Credits") currentState = AppState::CREDITS;
        });

        currentY += buttonHeight + spacing;
    }
}


void Menu::draw() {
    window.draw(backgroundSprite);

    switch (currentState) {
        case AppState::MENU: {
            for (size_t i = 0; i < buttons.size(); ++i) {
                std::string labelText = labels[i].getString();

                // Dessine illustration 1 juste avant le bouton "Connect"
                if (labelText == "Connect")
                    window.draw(illustrationSprite);

                // Dessine illustration 2 juste avant le bouton "Settings"
                if (labelText == "Settings")
                    window.draw(settingsIllustrationSprite);

                window.draw(buttons[i]);
                window.draw(labels[i]);
            }
            break;
        }

        case AppState::CONNECT:
        case AppState::SETTINGS:
        case AppState::HOW_TO_PLAY:
        case AppState::CREDITS: {
            sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
            overlay.setFillColor(sf::Color::Black);
            window.draw(overlay);

            sf::Text info;
            info.setFont(font);
            info.setCharacterSize(28);
            info.setFillColor(sf::Color::White);
            info.setString("Page: " + std::string(
                (currentState == AppState::CONNECT) ? "Connect" :
                (currentState == AppState::SETTINGS) ? "Settings" :
                (currentState == AppState::HOW_TO_PLAY) ? "How to Play" :
                "Credits"
            ));
            info.setPosition(100, 100);
            window.draw(info);

            if (currentState == AppState::SETTINGS && settingsPage)
                settingsPage->draw(window);

            if (backButton)
                backButton->draw(window);
            break;
        }
    }

    updateMusic();
}


void Menu::handleEvent(const sf::Event& event, bool& shouldClose) {
    if (event.type == sf::Event::Closed)
        shouldClose = true;

    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        if (currentState != AppState::MENU && backButton && backButton->isClicked(mousePos)) {
            currentState = AppState::MENU;
            return;
        }
        for (size_t i = 0; i < buttons.size(); ++i) {
            if (buttons[i].getGlobalBounds().contains(mousePos)) {
                if (i < actions.size())
                    actions[i]();
            }
        }
    }
}

void Menu::initBackButton() {
    backButton = std::make_unique<Button>(font, "Retour", sf::Vector2f{40, 40}, sf::Vector2f{120, 50});
}

void Menu::updateMusic() {
    if (currentState == AppState::MENU && music.getStatus() != sf::Music::Playing)
        music.play();
    else if (currentState != AppState::MENU && music.getStatus() == sf::Music::Playing)
        music.pause();
}