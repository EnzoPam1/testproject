/*
** EPITECH PROJECT, 2025
** zappy_graphics
** File description:
** button
*/

#include "Button.hpp"

Button::Button(sf::Font& font, const std::string& label, sf::Vector2f position, sf::Vector2f size) {
    shape.setSize(size);
    shape.setPosition(position);
    shape.setFillColor(sf::Color(100, 100, 100));
    shape.setOutlineThickness(2);
    shape.setOutlineColor(sf::Color::White);

    text.setFont(font);
    text.setString(label);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::White);
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setPosition(
        position.x + (size.x - textBounds.width) / 2.f - textBounds.left,
        position.y + (size.y - textBounds.height) / 2.f - textBounds.top
    );
}

void Button::draw(sf::RenderWindow& window) {
    window.draw(shape);
    window.draw(text);
}

bool Button::isClicked(sf::Vector2f mousePos) {
    return shape.getGlobalBounds().contains(mousePos);
}