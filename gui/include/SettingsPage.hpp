/*
** EPITECH PROJECT, 2025
** zappy_graphics
** File description:
** SettingsPage
*/

#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class SettingsPage {
public:
    SettingsPage(sf::Font& font, float& volume);
    void draw(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event);

private:
    sf::Font& font;
    float& volume;
    sf::RectangleShape sliderBar;
    sf::CircleShape sliderKnob;
    sf::Text volumeLabel;

    void updateLabel();
};
