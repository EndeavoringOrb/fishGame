// main.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "FluidSolver.hpp"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int GRID_SIZE = 128;

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Fluid Dynamics Simulation");
    //window.setFramerateLimit(60);

    // Fluid solver parameters
    float viscosity = 0.0f;
    float diffusion = 0.001f;
    float mouseForce = 5000.0f;
    float timeStep = 0.01f;

    FluidSolver solver(GRID_SIZE, viscosity, diffusion, timeStep);

    sf::Image fluidImage;
    fluidImage.create(GRID_SIZE, GRID_SIZE, sf::Color::Black);
    sf::Texture fluidTexture;
    fluidTexture.loadFromImage(fluidImage);
    sf::Sprite fluidSprite(fluidTexture);
    fluidSprite.setScale((float)WINDOW_WIDTH / (float)GRID_SIZE, (float)WINDOW_HEIGHT / (float)GRID_SIZE);

    bool isMousePressed = false;
    sf::Vector2i lastMousePos;

    // Init text
    sf::Font font;
    if (!font.loadFromFile("resources/fonts/arial/arial.ttf"))
    {
        return -1;
    }
    sf::Text infoText;
    infoText.setFont(font);
    infoText.setCharacterSize(18);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition(10, 10);

    // Init clock
    sf::Clock clock;

    // Init source
    Source source = Source(GRID_SIZE / 2, GRID_SIZE / 2, 0.05f);
    bool sourceFlowing = true;

    while (window.isOpen())
    {
        const float dt = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                isMousePressed = true;
                lastMousePos = sf::Mouse::getPosition(window);
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                isMousePressed = false;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                sourceFlowing = !sourceFlowing;
            }
        }

        // Add density and velocity when mouse is pressed
        if (isMousePressed)
        {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            // Map window coordinates to grid
            int gridX = (int)(((float)mousePos.x / (float)WINDOW_WIDTH) * (float)GRID_SIZE);
            int gridY = (int)(((float)mousePos.y / (float)WINDOW_HEIGHT) * (float)GRID_SIZE);

            // Add density source
            source.x = gridX;
            source.y = gridY;

            lastMousePos = mousePos;
        }

        // Update fluid solver
        solver.addSource(Source(0, GRID_SIZE / 2, 0.0f, mouseForce));
        solver.addSource(Source(GRID_SIZE / 2, GRID_SIZE, mouseForce, 0.0f));
        solver.addSource(Source(GRID_SIZE, GRID_SIZE / 2, 0.0f, -mouseForce));
        solver.addSource(Source(GRID_SIZE / 2, 0, -mouseForce, 0.0f));
        if (sourceFlowing) {
            solver.addSource(source);
        }
        solver.update();

        // Get density bounds
        sf::Vector2f densityBounds = solver.densityBounds();
        float densityRange = (densityBounds.y - densityBounds.x);
        const float invDensityRange = 1.0f / densityRange;

        // Update image based on density
        for (int i = 1; i <= GRID_SIZE; i++)
        {
            for (int j = 1; j <= GRID_SIZE; j++)
            {
                float density = (solver.dens[solver.IX(i, j)] - densityBounds.x) * invDensityRange;
                uint8_t colorVal = static_cast<uint8_t>(std::min(std::max(0.0f, density * 255.0f), 255.0f));
                fluidImage.setPixel(i - 1, j - 1, sf::Color(colorVal, colorVal, colorVal));
            }
        }

        fluidTexture.loadFromImage(fluidImage);
        fluidSprite.setTexture(fluidTexture, true);

        // Update info text
        std::ostringstream ss;
        // ss << std::fixed << std::setprecision(2);
        ss << "Screen Resolution: " << window.getSize().x << "x" << window.getSize().y << "\n";
        ss << "FPS: " << 1.0f / dt << "\n";
        ss << "Min Density: " << densityBounds.x << "\n";
        ss << "Max Density: " << densityBounds.y << "\n";
        infoText.setString(ss.str());

        window.clear();
        window.draw(fluidSprite);
        window.draw(infoText);
        window.display();
    }

    return 0;
}