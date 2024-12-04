#ifndef RIPPLE_HPP
#define RIPPLE_HPP

#include "helperUtils.hpp"

struct RippleArc
{
    sf::Vector2f origin;
    float startAngle;
    float stopAngle;
    float radius = 0.0f;

    float lifetime;
    float radiusDelta;

    RippleArc(sf::Vector2f _origin, float _startAngle, float _stopAngle, float _lifetime, float _radiusDelta)
        : origin(_origin), startAngle(_startAngle), stopAngle(_stopAngle), lifetime(_lifetime), radiusDelta(_radiusDelta) {}

    void update(float dt)
    {
        lifetime -= dt;
        radius += radiusDelta * dt;
    }

    void render(sf::RenderWindow &window)
    {
        if (done())
        {
            return;
        }
        drawArc(window, origin, startAngle, stopAngle, radius);
    }

    bool done()
    {
        return lifetime <= 0.0f;
    }
};

struct Ripple
{
    std::vector<RippleArc> rippleArcs;

    Ripple(int numArcs, sf::Vector2f origin, float lifetime, float radiusDelta, uint32_t &randSeed)
    {
        for (int i = 0; i < numArcs; i++)
        {
            rippleArcs.push_back(RippleArc(origin,
                                           randFloat(randSeed) * 360.0f,
                                           randFloat(randSeed) * 360.0f,
                                           randFloat(randSeed) * lifetime,
                                           randFloat(randSeed) * radiusDelta));
        }
    }

    void update(float dt)
    {
        for (RippleArc &arc : rippleArcs)
        {
            arc.update(dt);
        }
    }

    void render(sf::RenderWindow &window)
    {
        for (RippleArc &arc : rippleArcs)
        {
            arc.render(window);
        }
    }

    bool done()
    {
        for (RippleArc &arc : rippleArcs)
        {
            if (!arc.done())
            {
                return false;
            }
        }
        return true;
    }
};

#endif