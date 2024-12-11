#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <fstream>
#include <sstream>

#include "random.hpp"

struct WaterShader
{
    sf::Shader shader;
    sf::RectangleShape screenQuad;
    std::vector<sf::Vector2f> points;
    sf::Vector3f waterColor;
    sf::Vector3f foamColor;
    uint32_t randSeed = 42;
    float aspectRatio;

    void generateRandomPoints(float aspectRatio)
    {
        points.clear();

        for (int i = 0; i < 10; i++)
        {
            // Random point positions
            points.push_back({randFloat(randSeed) * aspectRatio, randFloat(randSeed)});
        }
    }

    WaterShader(std::string shaderPath, sf::Vector3f _waterColor, sf::Vector3f _foamColor, float _aspectRatio)
        : waterColor(_waterColor),
          foamColor(_foamColor),
          aspectRatio(_aspectRatio)
    {
        // Load shader from file
        if (!shader.loadFromFile(shaderPath, sf::Shader::Fragment))
        {
            throw std::runtime_error("Failed to load shader");
        }

        // Generate initial random points
        generateRandomPoints(_aspectRatio);
    }

    void update(const float dt)
    {
        float magnitude = 0.1f;
        for (int i = 0; i < points.size(); i++)
        {
            points[i] += {(randFloat(randSeed) * 2.0f * magnitude - magnitude) * dt,
                          (randFloat(randSeed) * 2.0f * magnitude - magnitude) * dt};
        }
    }

    void render(sf::RenderWindow &window, sf::View &view)
    {
        // Update quad
        sf::Vector2f viewSize = view.getSize();
        aspectRatio = viewSize.x / viewSize.y;
        screenQuad.setSize(viewSize);
        screenQuad.setPosition(-0.5f * viewSize);

        // Update shader uniforms
        sf::Vector2u windowSize = window.getSize();
        shader.setUniform("u_resolution", sf::Glsl::Vec2(windowSize.x, windowSize.y));
        shader.setUniform("u_primaryColor", waterColor);
        shader.setUniform("u_secondaryColor", foamColor);

        // Set point positions
        shader.setUniformArray("u_points", points.data(), points.size());

        // Clear and draw
        window.draw(screenQuad, &shader);
    }
};

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
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Water Shader");
    window.setFramerateLimit(maxFrameRate);
    sf::View view = window.getDefaultView();

    // Create camera view with fixed height
    float aspectRatio = static_cast<float>(window.getSize().x) / window.getSize().y;
    sf::View cameraView;
    cameraView.setSize(CAMERA_HEIGHT * aspectRatio, CAMERA_HEIGHT);
    cameraView.setCenter(0.0f, 0.0f);

    // Init water shader
    WaterShader waterShader = WaterShader("resources/shaders/water.frag", {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, aspectRatio);

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
        }

        // Update water shader
        waterShader.update(dt);

        // Clear screen
        window.clear(sf::Color::Black);

        // Render shader
        window.setView(cameraView);
        waterShader.render(window, cameraView);

        window.display();
    }

    return 0;
}