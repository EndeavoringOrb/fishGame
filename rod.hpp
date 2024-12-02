#ifndef ROD_HPP
#define ROD_HPP

#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

struct Rod
{
    glm::vec2 origin;
    glm::vec2 pos;
    glm::vec2 castPos;
    float radius = 0.1f;

    bool cast = false; // true if the rod has been cast

    bool pulling = false;
    float pullTimer = 0.0f;
    float pullTimeMax;

    Rod(glm::vec2 _origin, float _radius, float _pullTimeMax)
        : origin(_origin),
          pos(_origin),
          castPos(_origin),
          radius(_radius),
          pullTimeMax(_pullTimeMax) {}

    void setCastPos(glm::vec2 _pos)
    {
        pos = _pos;
        castPos = _pos;
        cast = true;
    }

    void startPulling()
    {
        pulling = true;
    }

    void update(const float dt)
    {
        if (pulling)
        {
            // Update rod position
            pullTimer += dt;
            float t = pullTimer / pullTimeMax;
            pos = castPos + t * (origin - castPos);
        }
    }

    bool finishedPulling()
    {
        return pullTimer > pullTimeMax;
    }

    void reset()
    {
        pulling = false;
        pullTimer = 0.0f;
        cast = false;
    }

    void render(sf::RenderWindow &window)
    {
        if (cast)
        {
            sf::CircleShape circle(radius);
            circle.setFillColor(sf::Color::Red);
            circle.setOrigin(radius, radius);
            circle.setPosition(pos.x, pos.y);
            window.draw(circle);
        }
    }
};

#endif