/*
** EPITECH PROJECT, 2025
** zappy_graphics
** File description:
** Button
*/

#pragma once

#include <SFML/Graphics.hpp>

class Button {
public:
    Button(sf::Font& font, const std::string& label, sf::Vector2f position, sf::Vector2f size);
    void draw(sf::RenderWindow& window);
    bool isClicked(sf::Vector2f mousePos);
private:
    sf::RectangleShape shape;
    sf::Text text;
};