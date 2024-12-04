#include "fishBook.hpp"
#include "ripple.hpp"

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
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Boids Fish Simulation");
    window.setFramerateLimit(maxFrameRate);
    sf::View view = window.getDefaultView();

    // Create camera view with fixed height
    float aspectRatio = static_cast<float>(window.getSize().x) / window.getSize().y;
    sf::View cameraView;
    cameraView.setSize(CAMERA_HEIGHT * aspectRatio, CAMERA_HEIGHT);
    cameraView.setCenter(0.0f, 0.0f);

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

    // Create flock and add fish
    Flock flock = Flock(randSeed, 1.0f / (float)fixedUpdateRate);
    flock.setWorldBounds(CAMERA_HEIGHT * aspectRatio, CAMERA_HEIGHT);

    // Predefined fish types
    std::vector<FishType> fishTypes = {
        FishType("Tiny Swift", 0.1f, 1.0f, 2.5f, sf::Color::Blue, sf::Color::Cyan, sf::Color::Cyan, sf::Color::Green),
        FishType("Medium Cruiser", 0.2f, 1.0f, 1.5f, {255, 127, 0}, sf::Color::Red, sf::Color::Red, sf::Color::Black),
        FishType("Large Slowpoke", 0.3f, 1.2f, 0.7f, sf::Color::Green, {0, 200, 0}, {0, 200, 0}, sf::Color::Red)};

    // Add fish at random positions
    for (int i = 0; i < numFish; ++i)
    {
        flock.addRandomFish(fishTypes[randInt(randSeed, 3)]);
    }

    // Init rod
    Rod rod = Rod({-0.5f * CAMERA_HEIGHT * aspectRatio, 0.0f}, 0.05f, 3.0f, 3.0f);

    // Init inventory
    int coins = 0;
    FishBook book;
    book.entries.push_back(FishEntry(fishTypes[0], 1));
    book.entries.push_back(FishEntry(fishTypes[1], 3));
    book.entries.push_back(FishEntry(fishTypes[2], 5));

    // Init ripples
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

                // Update world bounds
                flock.setWorldBounds(CAMERA_HEIGHT * newAspectRatio, CAMERA_HEIGHT);
            }
            else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i pixelCoords = {event.mouseButton.x, event.mouseButton.y};
                sf::Vector2f coords = window.mapPixelToCoords(pixelCoords, cameraView);
                ripples.push_back(Ripple(32, coords, 3.0f, 1.0f, randSeed));
                rod.setCastPos({coords.x, coords.y});
                flock.addAffector(Affector(false, rod.castPos, 1.0f));
            }
            else if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Space)
                {
                    flock.pull();
                    rod.startPulling();
                }
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

        // Handle rod pulling
        rod.update(dt);

        if (rod.finishedPulling())
        {
            // Get pulled fish
            std::vector<Fish> pulledFish = flock.finishPull();

            // Update fish book
            int newCoins = book.update(pulledFish);
            coins += newCoins;

            // Reset rod
            rod.reset();
        }

        // Update flock
        flock.update(dt, rod);

        // Update info text
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "Screen Resolution: " << window.getSize().x << "x" << window.getSize().y << "\n";
        ss << "FPS: " << 1.0f / dt << "\n";
        ss << "Camera Height: " << CAMERA_HEIGHT << "\n";
        ss << "Camera Width: " << CAMERA_HEIGHT * aspectRatio << "\n";
        ss << "# Fish: " << flock.allFish.size() << "\n";
        ss << "Coins: " << coins << "\n";
        infoText.setString(ss.str());

        // Clear screen
        window.clear(sf::Color::Black);

        // Draw fish with camera view
        window.setView(cameraView);
        flock.render(window);

        // Draw ripples
        for (Ripple &ripple : ripples)
        {
            ripple.render(window);
        }

        // Draw rod
        rod.render(window);

        // Draw UI with default view
        window.setView(view);
        window.draw(infoText);

        window.display();
    }

    return 0;
}