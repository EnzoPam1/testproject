/*
** EPITECH PROJECT, 2025
** zappy_graphics
** File description:
** SettingsPage
*/

#include "SettingsPage.hpp"
#include <sstream>

SettingsPage::SettingsPage(sf::Font& fontRef, float& vol)
    : font(fontRef), volume(vol) {
    sliderBar.setSize({300, 5});
    sliderBar.setFillColor(sf::Color::White);
    sliderBar.setPosition(150, 200);

    sliderKnob.setRadius(10);
    sliderKnob.setFillColor(sf::Color::Red);
    sliderKnob.setOrigin(10, 10);
    sliderKnob.setPosition(sliderBar.getPosition().x + (volume / 100.0f) * 300, 202);

    volumeLabel.setFont(font);
    volumeLabel.setCharacterSize(20);
    volumeLabel.setPosition(150, 160);
    volumeLabel.setFillColor(sf::Color::White);

    updateLabel();
}

void SettingsPage::updateLabel() {
    std::ostringstream oss;
    oss << "Volume: " << static_cast<int>(volume) << "%";
    volumeLabel.setString(oss.str());
}

void SettingsPage::draw(sf::RenderWindow& window) {
    window.draw(volumeLabel);
    window.draw(sliderBar);
    window.draw(sliderKnob);
}

void SettingsPage::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (mousePos.y >= sliderBar.getPosition().y - 10 &&
                mousePos.y <= sliderBar.getPosition().y + 10 &&
                mousePos.x >= sliderBar.getPosition().x &&
                mousePos.x <= sliderBar.getPosition().x + sliderBar.getSize().x) {
                sliderKnob.setPosition(mousePos.x, sliderBar.getPosition().y + 2);
                volume = ((mousePos.x - sliderBar.getPosition().x) / sliderBar.getSize().x) * 100.0f;
                if (volume < 0) volume = 0;
                if (volume > 100) volume = 100;
                updateLabel();
            }
        }
    }
}
