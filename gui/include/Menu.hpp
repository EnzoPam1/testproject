/*
** EPITECH PROJECT, 2025
** zappy_graphics
** File description:
** menu
*/

#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include "state.hpp"
#include "Button.hpp"
#include <memory>
#include <SFML/Audio.hpp>
#include <iostream>
#include "SettingsPage.hpp"

class Menu {
public:
    Menu(sf::RenderWindow& window, AppState& state);
    void draw();
    void handleEvent(const sf::Event& event, bool& shouldClose);
    std::unique_ptr<Button> backButton;

private:
    sf::RenderWindow& window;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Font font;
    std::vector<sf::RectangleShape> buttons;
    std::vector<sf::Text> labels;
    std::vector<std::function<void()>> actions;
    AppState& currentState;
    sf::Music music;
    std::unique_ptr<SettingsPage> settingsPage;
    sf::Texture illustrationTexture;
    sf::Sprite illustrationSprite;
    sf::Texture settingsIllustrationTexture;
    sf::Sprite settingsIllustrationSprite;

    float musicVolume = 50.0f;
    void createButtons();
    void initBackButton();
    void updateMusic();
};
