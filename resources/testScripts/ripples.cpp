#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <fstream>
#include <sstream>

#include "ripple.hpp"
#include "random.hpp"


int main()
{
    // Settings
    // #################################
    int fixedUpdateRate = 500;
    int maxFrameRate = 60;
    const float CAMERA_HEIGHT = 10.0f;
    int numFish = 20;
    uint32_t randSeed = 42;
    // #################################

    // Init window
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Ripples");
    window.setFramerateLimit(maxFrameRate);
    sf::View view = window.getDefaultView();

    // Create camera view with fixed height
    float aspectRatio = static_cast<float>(window.getSize().x) / window.getSize().y;
    sf::View cameraView;
    cameraView.setSize(CAMERA_HEIGHT * aspectRatio, CAMERA_HEIGHT);
    cameraView.setCenter(0.0f, 0.0f);

    std::vector<Ripple> ripples;

    // Init game clock
    sf::Clock gameClock;

    while (window.isOpen())
    {
        const float dt = gameClock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::Resized)
            {
                // Update UI view
                view.setSize({(float)event.size.width, (float)event.size.height});
                view.setCenter({(float)event.size.width * 0.5f, (float)event.size.height * 0.5f});
                window.setView(view);

                // Update camera view maintaining fixed height
                float newAspectRatio = static_cast<float>(event.size.width) / event.size.height;
                cameraView.setSize(CAMERA_HEIGHT * newAspectRatio, CAMERA_HEIGHT);
            }
            else if (event.type == sf::Event::MouseButtonPressed &&
                     event.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i pixelCoords = {event.mouseButton.x, event.mouseButton.y};
                sf::Vector2f coords = window.mapPixelToCoords(pixelCoords, cameraView);
                ripples.push_back(Ripple(32, coords, 3.0f, 1.0f, randSeed));
            }
        }

        // Update ripples
        for (int i = ripples.size() - 1; i >= 0; i--)
        {
            ripples[i].update(dt);
            if (ripples[i].done())
            {
                ripples.erase(ripples.begin() + i);
            }
        }

        // Clear screen
        window.clear(sf::Color::Black);

        // Render
        window.setView(cameraView);
        for (Ripple &ripple : ripples)
        {
            ripple.render(window);
        }

        window.display();
    }

    return 0;
}