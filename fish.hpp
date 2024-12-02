#ifndef FISH_HPP
#define FISH_HPP

#include "random.hpp"
#include "helperUtils.hpp"
#include "rod.hpp"

struct FishType
{
    std::string name;
    float headSize;
    float linkDistanceMultiplier;
    float moveSpeed;
    sf::Color bodyColor;
    sf::Color finColor;
    sf::Color tailColor;
    sf::Color eyeColor;

    // Constructor to make creating fish types easier
    FishType(const std::string _name,
             float _headSize,
             float _linkDistanceMultiplier,
             float _moveSpeed,
             const sf::Color _bodyColor,
             const sf::Color _finColor,
             const sf::Color _tailColor,
             const sf::Color _eyeColor)
        : name(_name),
          headSize(_headSize),
          linkDistanceMultiplier(_linkDistanceMultiplier),
          moveSpeed(_moveSpeed),
          bodyColor(_bodyColor),
          finColor(_finColor),
          tailColor(_tailColor),
          eyeColor(_eyeColor) {}
};

struct Fish
{
    // Misc
    uint32_t randSeed = 42;

    // Structure
    std::vector<glm::vec2> points;
    std::vector<float> sizes;
    float linkDistance;

    // Movement
    float moveSpeed;
    glm::vec2 forward = {-1.0f, 0.0f};
    float maxTurnAngle = 30.0f;

    // Hook
    bool hooked = false;
    float pullTimer = 3.0f; // The fish will get off the hook if it isn't pulled within this amount of seconds
    bool pulled = false;

    // Fish Appearance
    float eyeRadius = 0.025f;
    float normalFinRotation = 30.0f;
    float turnFinRotation = 20.0f;
    int finIndex;
    float finSize = FLT_MIN;

    sf::Color bodyColor;
    sf::Color finColor;
    sf::Color tailColor;
    sf::Color eyeColor;

    // Info
    std::string name;

    Fish(float _linkDistance, float _moveSpeed, sf::Color _bodyColor, sf::Color _finColor, sf::Color _tailColor, sf::Color _eyeColor, std::string _name)
        : linkDistance(_linkDistance),
          moveSpeed(_moveSpeed),
          bodyColor(_bodyColor),
          finColor(_finColor),
          tailColor(_tailColor),
          eyeColor(_eyeColor),
          name(_name) {}

    // Set the head of the fish to pos and constrain fish
    void setHeadPosition(glm::vec2 pos)
    {
        points[0] = pos;
        constrain();
    }

    // Sets the value of hooked
    void setHooked(bool value)
    {
        hooked = value;
    }

    // Sets the value of pulled
    void setPulled(bool value)
    {
        pulled = value;
    }

    // Add a joint to the end of a fish
    void addJoint(glm::vec2 pos, float size)
    {
        points.push_back(pos);
        sizes.push_back(size);
        constrain();
        if (size > finSize)
        {
            finIndex = points.size() - 1;
            finSize = size;
        }
    }

    // Update the fish joint positions based on the forward direction and the delta time
    void update(float dt)
    {
        // Move fish forward
        forward = glm::normalize(forward);
        points[0] += forward * moveSpeed * dt;

        // Constrain other joints
        constrain();

        // Update hook stuff
        if (hooked)
        {
            pullTimer -= dt;
            if (!pulled && pullTimer <= 0.0f)
            {
                setHooked(false);
            }
        }
    }

    // Constrain points based on linkDistance & turnAngle
    void constrain()
    {
        for (int i = 1; i < points.size(); i++)
        {
            // Constrain distance
            glm::vec2 diff = glm::normalize(points[i] - points[i - 1]);
            points[i] = points[i - 1] + diff * linkDistance;

            // Constrain angle
            float angle = jointAngle(i);
            angle = std::clamp(angle, -maxTurnAngle, maxTurnAngle);
            diff = -jointForward(i - 1);
            diff = rotate(diff, angle);
            points[i] = points[i - 1] + diff * linkDistance;
        }
    }

    // Get the unit forward direction for the joint at jointIndex
    glm::vec2 jointForward(int jointIndex)
    {
        if (jointIndex == 0)
        {
            return glm::normalize(forward);
        }
        return glm::normalize(points[jointIndex - 1] - points[jointIndex]);
    }

    // Get the position on the left side of the point at index jointIndex based on the size at index jointIndex
    glm::vec2 jointLeft(int jointIndex)
    {
        return rotate(jointForward(jointIndex) * sizes[jointIndex], -90) + points[jointIndex];
    }

    // Get the position on the right side of the point at index jointIndex based on the size at index jointIndex
    glm::vec2 jointRight(int jointIndex)
    {
        return rotate(jointForward(jointIndex) * sizes[jointIndex], 90) + points[jointIndex];
    }

    // Return the angle of the point at index jointIndex
    float jointAngle(int jointIndex)
    {
        float angle = getAngle(jointForward(jointIndex - 1), jointForward(jointIndex));
        return angle;
    }

    // Returns the curvature of the fish in range [-1, 1]
    float curvature()
    {
        float angle = 0.0f;
        for (int i = 1; i < points.size(); i++)
        {
            angle += jointAngle(i);
        }
        return angle / ((points.size() - 1) * maxTurnAngle);
    }

    // Render the fish
    void render(sf::RenderWindow &window)
    {
        if (points.size() < 2)
            return;

        // Draw outline
        std::vector<glm::vec2> outlinePoints;

        // Add points on head
        outlinePoints.push_back(rotate(forward * sizes[0], 30) + points[0]);
        outlinePoints.push_back(rotate(forward * sizes[0], 90) + points[0]);

        // Add other points (in order going around)
        for (int i = 1; i < points.size(); ++i)
        {
            outlinePoints.push_back(jointRight(i));
        }
        for (int i = points.size() - 1; i > 0; i--)
        {
            outlinePoints.push_back(jointLeft(i));
        }

        // Other head points
        outlinePoints.push_back(rotate(forward * sizes[0], -90) + points[0]);
        outlinePoints.push_back(rotate(forward * sizes[0], -30) + points[0]);

        // Render side fins
        float bodyAngle = curvature();
        glm::vec2 finRight = jointRight(finIndex);
        float rightRotation = getRotation(jointRight(finIndex - 1) - finRight);
        drawEllipse(window,
                    {finRight.x, finRight.y},
                    {finSize * 0.75f, finSize * 0.75f * 0.5f},
                    rightRotation - normalFinRotation - bodyAngle * turnFinRotation,
                    finColor);

        glm::vec2 finLeft = jointLeft(finIndex);
        float leftRotation = getRotation(jointLeft(finIndex - 1) - finLeft);
        drawEllipse(window,
                    {finLeft.x, finLeft.y},
                    {finSize * 0.75f, finSize * 0.75f * 0.5f},
                    leftRotation + normalFinRotation - bodyAngle * turnFinRotation,
                    finColor);

        // Render tail fin
        int lastIdx = points.size() - 1;
        glm::vec2 lastPoint = points[lastIdx];
        glm::vec2 tailPoint = lastPoint - jointForward(lastIdx) * linkDistance;
        glm::vec2 tailMovePoint = tailPoint + (jointRight(lastIdx) - lastPoint) * 3.0f * curvature();
        drawSmoothFillConvex(
            {lastPoint, tailPoint, tailMovePoint},
            window,
            tailColor);
        drawSmoothLine({lastPoint, tailPoint, tailMovePoint}, window, true);

        // Render body
        drawSmoothFillTube(outlinePoints, window, bodyColor);
        drawSmoothLine(outlinePoints, window, true); // Body outline

        // Render eyes
        glm::vec2 rightEyePos = rotate(forward * sizes[0] * 0.5f, 90) + points[0];
        glm::vec2 leftEyePos = rotate(forward * sizes[0] * 0.5f, -90) + points[0];
        sf::CircleShape circle;
        circle.setRadius(eyeRadius);
        circle.setFillColor(eyeColor);
        circle.setOrigin(eyeRadius, eyeRadius);
        circle.setPosition({rightEyePos.x, rightEyePos.y});
        window.draw(circle);
        circle.setPosition({leftEyePos.x, leftEyePos.y});
        window.draw(circle);

        // Render dorsal fin
        drawSmoothLine({points[1], points[2], points[3]}, window, false);                                                    // Base
        drawSmoothLine({points[1], points[2] + (jointRight(2) - points[2]) * curvature() * 1.0f, points[3]}, window, false); // Top
    }

    // Get the position of the fish's head
    glm::vec2 getHeadPosition() const
    {
        return points.empty() ? glm::vec2(0.0f) : points[0];
    }
};

struct Affector
{
    bool attractor;
    glm::vec2 pos;
    float lifetime;
    bool hasLifetime;

    Affector(bool _attractor, glm::vec2 _pos, float _lifetime = -1.0f)
        : attractor(_attractor),
          pos(_pos),
          lifetime(_lifetime)
    {
        hasLifetime = _lifetime >= 0.0f;
    }

    bool finished()
    {
        return hasLifetime && lifetime <= 0.0f;
    }

    void update(const float dt)
    {
        lifetime -= dt;
    }
};

struct Flock
{
    std::vector<std::unique_ptr<Fish>> allFish;
    std::vector<Affector> affectors;

    float hookDist = 1.0f;

    // Boids parameters
    float separationRadius = 1.0f;
    float alignmentRadius = 2.0f;
    float cohesionRadius = 3.0f;
    float repellorRadius = 5.0f;
    float attractorRadius = 1.0f;

    float separationWeight = 1.5f;
    float alignmentWeight = 1.0f;
    float cohesionWeight = 0.8f;
    float repellorWeight = 3.0f;
    float attractorWeight = 1.0f;

    float delta = 0.999f; // How much of the current velocity is maintained

    // World bounds for containing the flock
    float worldWidth = 20.0f;
    float worldHeight = 10.0f;

    uint32_t randSeed;
    float fixedDt;
    float accumulatedDt = 0.0f;

    Flock(uint32_t _randSeed, float _fixedDt)
        : randSeed(_randSeed),
          fixedDt(_fixedDt) {}

    // Calculate the direction a fish should move to move away from other fish that are too close (within the separationRadius)
    glm::vec2 calculateSeparation(const Fish &fish)
    {
        glm::vec2 separation(0.0f);
        int count = 0;

        for (const auto &other : allFish)
        {
            if (other.get() == &fish)
                continue;

            glm::vec2 diff = fish.getHeadPosition() - other->getHeadPosition();
            float distance = glm::length(diff);

            if (distance < separationRadius && distance > 0)
            {
                separation += glm::normalize(diff) / distance;
                count++;
            }
        }

        if (count > 0)
        {
            separation /= static_cast<float>(count);
        }

        return separation;
    }

    // Calculate the direction a fish should move to align it's direction with other fish within the alignmentRadius
    glm::vec2 calculateAlignment(const Fish &fish)
    {
        glm::vec2 averageVelocity(0.0f);
        int count = 0;

        for (const auto &other : allFish)
        {
            if (other.get() == &fish)
                continue;

            float distance = glm::length(fish.getHeadPosition() - other->getHeadPosition());

            if (distance < alignmentRadius)
            {
                averageVelocity += other->forward;
                count++;
            }
        }

        if (count > 0)
        {
            averageVelocity /= static_cast<float>(count);
            averageVelocity = glm::normalize(averageVelocity);
        }

        return averageVelocity;
    }

    // Calculate the direction a fish should move to go towards other fish that are within the cohesionRadius
    glm::vec2 calculateCohesion(const Fish &fish)
    {
        glm::vec2 center(0.0f);
        int count = 0;

        for (const auto &other : allFish)
        {
            if (other.get() == &fish)
                continue;

            float distance = glm::length(fish.getHeadPosition() - other->getHeadPosition());

            if (distance < cohesionRadius)
            {
                center += other->getHeadPosition();
                count++;
            }
        }

        if (count > 0)
        {
            center /= static_cast<float>(count);
            return glm::normalize(center - fish.getHeadPosition());
        }

        return glm::vec2(0.0f);
    }

    // Calculate the direction a fish should move to avoid boundaries
    glm::vec2 calculateBoundaryAvoidance(const Fish &fish)
    {
        glm::vec2 avoidance(0.0f);
        float margin = std::min(worldWidth * 0.1f, worldHeight * 0.1f);
        glm::vec2 pos = fish.getHeadPosition();

        if (pos.x < -worldWidth / 2 + margin)
            avoidance.x += 1.0f;
        if (pos.x > worldWidth / 2 - margin)
            avoidance.x -= 1.0f;
        if (pos.y < -worldHeight / 2 + margin)
            avoidance.y += 1.0f;
        if (pos.y > worldHeight / 2 - margin)
            avoidance.y -= 1.0f;

        if (glm::length(avoidance) > 0)
        {
            avoidance = glm::normalize(avoidance);
        }

        return avoidance;
    }

    glm::vec2 calculateAttractorInfluence(const Fish &fish)
    {
        glm::vec2 influence(0.0f);
        bool influenced = false;
        glm::vec2 fishPos = fish.getHeadPosition();

        for (const Affector affector : affectors)
        {
            if (!affector.attractor)
            {
                continue;
            }
            glm::vec2 diff = affector.pos - fishPos;
            if (glm::length(diff) < attractorRadius)
            {
                influence += glm::normalize(diff);
                influenced = true;
            }
        }

        if (influenced > 0)
        {
            return glm::normalize(influence);
        }

        return glm::vec2(0.0f);
    }

    glm::vec2 calculateRepellorInfluence(const Fish &fish)
    {
        glm::vec2 influence(0.0f);
        bool influenced = false;
        glm::vec2 fishPos = fish.getHeadPosition();

        for (const Affector affector : affectors)
        {
            if (affector.attractor)
            {
                continue;
            }
            glm::vec2 diff = affector.pos - fishPos;
            if (glm::length(diff) < attractorRadius)
            {
                influence -= glm::normalize(diff);
                influenced = true;
            }
        }

        if (influenced > 0)
        {
            return glm::normalize(influence);
        }

        return glm::vec2(0.0f);
    }

    // Creates a fish with random traits, then adds it to the flock
    // Method to add a random fish of a specific type
    void addRandomFish(const FishType &fishType)
    {
        float aspectRatio = worldWidth / worldHeight;
        float x = randFloat(randSeed) * worldHeight * aspectRatio - (worldHeight * aspectRatio / 2);
        float y = randFloat(randSeed) * worldHeight - (worldHeight / 2);

        float headSize = fishType.headSize;
        float linkDistance = headSize * (randFloat(randSeed) * 1.0f + 1.0f) * fishType.linkDistanceMultiplier;
        float moveSpeed = fishType.moveSpeed + (randFloat(randSeed) * 1.0f - 0.5f);

        auto fish = std::make_unique<Fish>(linkDistance, moveSpeed, fishType.bodyColor, fishType.finColor, fishType.tailColor, fishType.eyeColor, fishType.name);
        fish->randSeed = PCG_Hash(randSeed); // Give each fish a unique seed

        // Add joints to create the fish body
        fish->addJoint({x + 0 * linkDistance, y}, headSize);
        fish->addJoint({x + 1 * linkDistance, y}, headSize * (4.0f / 3.0f));
        fish->addJoint({x + 2 * linkDistance, y}, headSize);
        fish->addJoint({x + 3 * linkDistance, y}, headSize * (2.0f / 3.0f));
        fish->addJoint({x + 4 * linkDistance, y}, headSize * (1.0f / 3.0f));

        allFish.push_back(std::move(fish));
    }

    // Update the flock based on the amount of time passed
    void update(float dt, Rod &rod)
    {
        accumulatedDt += dt;
        int nSteps = (int)(accumulatedDt / fixedDt);
        accumulatedDt -= nSteps * fixedDt;
        for (int i = 0; i < nSteps; i++)
        {
            updateAffectors(fixedDt);
            step(fixedDt);
            if (rod.cast)
            {
                bool hooked = hookFish(rod.pos, rod.readyToHook());
                if (hooked)
                {
                    rod.timeSinceHooked = 0.0f;
                }
            }
        }
    }

    void updateAffectors(float dt)
    {
        for (int i = affectors.size() - 1; i >= 0; i--)
        {
            Affector &affector = affectors[i];
            affector.update(dt);
            if (affector.finished())
            {
                affectors.erase(affectors.begin() + i);
            }
        }
    }

    // Take one boids step
    void step(float dt)
    {
        for (auto &fish : allFish)
        {
            // Calculate flocking behaviors
            glm::vec2 separation = calculateSeparation(*fish) * separationWeight;
            glm::vec2 alignment = calculateAlignment(*fish) * alignmentWeight;
            glm::vec2 cohesion = calculateCohesion(*fish) * cohesionWeight;
            glm::vec2 boundaryAvoidance = calculateBoundaryAvoidance(*fish) * 2.0f;
            glm::vec2 attractorInfluence = calculateAttractorInfluence(*fish) * attractorWeight;
            glm::vec2 repellorInfluence = calculateRepellorInfluence(*fish) * repellorWeight;

            // Combine all behaviors
            glm::vec2 desiredDirection = separation + alignment + cohesion + boundaryAvoidance + attractorInfluence + repellorInfluence;

            if (glm::length(desiredDirection) > 0)
            {
                // Adjust fish direction based on the calculated behaviors
                fish->forward = fish->forward * delta + desiredDirection * (1 - delta);
            }

            // Update fish physics
            fish->update(dt);
        }
    }

    bool hookFish(glm::vec2 rodPosition, bool readyToHook)
    {
        // If a fish has already been hooked, update that fish
        for (auto &fish : allFish)
        {
            if (fish->hooked)
            {
                fish->setHeadPosition(rodPosition);
                return true;
            }
        }

        if (!readyToHook) {
            return false;
        }

        // Otherwise, check if a fish can be hooked
        for (auto &fish : allFish)
        {
            // Get distance from rod
            glm::vec2 diff = fish->getHeadPosition() - rodPosition;
            float dist = glm::length(diff);

            // If close enough, hook fish
            if (dist < hookDist)
            {
                fish->setHooked(true);

                // Update fish position
                fish->setHeadPosition(rodPosition);
                return true;
            }
        }

        return false;
    }

    // Render the flock
    void render(sf::RenderWindow &window)
    {
        for (auto &fish : allFish)
        {
            fish->render(window);
        }
    }

    // Set the boundaries for the fish
    void setWorldBounds(float width, float height)
    {
        worldWidth = width;
        worldHeight = height;
    }

    void addAffector(Affector affector)
    {
        affectors.push_back(affector);
    }

    void pull()
    {
        for (auto &fish : allFish)
        {
            if (fish->hooked)
            {
                fish->setPulled(true);
            }
        }
    }

    // Removes any fish that are being pulled from the flock. Returns the removed fish.
    std::vector<Fish> finishPull()
    {
        std::vector<Fish> pulledFish;
        for (int i = allFish.size() - 1; i >= 0; i--)
        {
            auto &fish = allFish[i];
            if (fish->pulled)
            {
                pulledFish.push_back(*fish);
                allFish.erase(allFish.begin() + i);
            }
        }
        return pulledFish;
    }
};

#endif