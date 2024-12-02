#include <glm/gtc/matrix_transform.hpp>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "random.hpp"

/// @brief Multiply radians with this constant to convert to degrees.
constexpr float rad2deg = (180.0f / M_PI);
/// @brief Multiply degrees with this constant to convert to radians.
constexpr float deg2rad = (M_PI / 180.0f);

/**
 * @brief Rotates a 2D vector by a specified number of degrees
 *
 * @param vector The input vector to rotate
 * @param degrees Rotation angle in degrees
 * @return glm::vec2 The rotated vector
 */
glm::vec2 rotate(const glm::vec2 &vector, float degrees)
{
    float radians = degrees * deg2rad; // Convert to radians
    float cs = cos(radians);
    float sn = sin(radians);

    return glm::vec2(
        vector.x * cs - vector.y * sn,
        vector.x * sn + vector.y * cs);
}

// Returns the rotation from the positive horizontal axis
float getRotation(glm::vec2 vector)
{
    float radians = std::atan2(vector.y, vector.x);
    return (radians >= 0.0f ? radians : (radians + 2 * M_PI)) * rad2deg; // (-180.0f, 180.0f) -> (0.0f, 360.0f)
}

// Returns the angle in degrees [-180, 180] between vec1 and vec2
float getAngle(glm::vec2 vec1, glm::vec2 vec2)
{
    float dot = vec1.x * vec2.x + vec1.y * vec2.y; // Dot product (cos)
    float det = vec1.x * vec2.y - vec1.y * vec2.x; // Determinant (sin)
    float radians = std::atan2(det, dot);          // atan2(y, x) or atan2(sin, cos)
    return radians * rad2deg;
}

// Smoothly (d/dx = 0 at 0 and 1) interpolates between 0 and 1 as t goes from 0 to 1.
float smoothstep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - t + t);
}

// Uses smoothstep to interpolate between a and b based on t.
float smoothstep(float t, float a, float b)
{
    t = smoothstep(t);
    return a * (1.0f - t) + b * t;
}

// Draw a smooth line through the points.
void drawSmoothLine(const std::vector<glm::vec2> &points, sf::RenderWindow &window,
                    bool loop = false, sf::Color color = sf::Color::White)
{
    if (points.size() < 2)
        return;

    // Number of segments between each pair of points
    const int smoothness = 20;

    // Calculate number of vertices needed
    size_t numVertices = (points.size() - !loop) * smoothness + 1;

    // Create vertex array for the curve
    sf::VertexArray curve(sf::LineStrip, numVertices);

    // Function to calculate catmull-rom spline point
    auto catmullRom = [](const glm::vec2 &p0, const glm::vec2 &p1,
                         const glm::vec2 &p2, const glm::vec2 &p3, float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        // Catmull-Rom matrix coefficients
        float b0 = -0.5f * t3 + t2 - 0.5f * t;
        float b1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
        float b2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        float b3 = 0.5f * t3 - 0.5f * t2;

        return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
    };

    // Helper function to safely get point with proper wrapping/clamping
    auto getPoint = [&points, loop](int index) -> glm::vec2
    {
        if (loop)
        {
            // Wrap around for looped lines
            return points[(index + points.size()) % points.size()];
        }
        else
        {
            // Clamp to endpoints for non-looped lines
            return points[std::clamp(index, 0, static_cast<int>(points.size()) - 1)];
        }
    };

    // For each point
    size_t numSegments = loop ? points.size() : points.size() - 1;
    for (size_t i = 0; i < numSegments; ++i)
    {
        // Get four points for the spline segment
        glm::vec2 p0 = getPoint(static_cast<int>(i) - 1);
        glm::vec2 p1 = getPoint(static_cast<int>(i));
        glm::vec2 p2 = getPoint(static_cast<int>(i) + 1);
        glm::vec2 p3 = getPoint(static_cast<int>(i) + 2);

        // Calculate points along the curve segment
        for (int j = 0; j < smoothness; ++j)
        {
            float t = static_cast<float>(j) / smoothness;
            glm::vec2 position = catmullRom(p0, p1, p2, p3, t);

            size_t index = i * smoothness + j;
            curve[index].position = sf::Vector2f(position.x, position.y);
            curve[index].color = color;
        }
    }

    // Add final point
    if (loop)
    {
        // Connect back to the start for looped lines
        curve[numVertices - 1].position = curve[0].position;
    }
    else
    {
        // Use final point for non-looped lines
        curve[numVertices - 1].position = sf::Vector2f(points.back().x, points.back().y);
    }
    curve[numVertices - 1].color = color;

    // Draw the curve
    window.draw(curve);
}

// Fill in the area of color {color} between a smooth line through the points.
void drawSmoothFillConvex(const std::vector<glm::vec2> &points, sf::RenderWindow &window, sf::Color color = sf::Color::White)
{
    if (points.size() < 2)
        return;

    // Number of segments between each pair of points
    const int smoothness = 20;

    // Create vertex array for the filled shape using triangle fan
    sf::VertexArray filledShape(sf::TriangleFan, points.size() * smoothness + 2);

    // Calculate center point as average of all points
    glm::vec2 center(0.0f, 0.0f);
    for (const auto &point : points)
    {
        center += point;
    }
    center /= static_cast<float>(points.size());

    // Set center point as first vertex
    filledShape[0].position = sf::Vector2f(center.x, center.y);
    filledShape[0].color = color; // Center point color

    // Function to calculate catmull-rom spline point
    auto catmullRom = [](const glm::vec2 &p0, const glm::vec2 &p1,
                         const glm::vec2 &p2, const glm::vec2 &p3, float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        // Catmull-Rom matrix coefficients
        float b0 = -0.5f * t3 + t2 - 0.5f * t;
        float b1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
        float b2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        float b3 = 0.5f * t3 - 0.5f * t2;

        return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
    };

    // For each point
    for (size_t i = 0; i < points.size(); ++i)
    {
        // Get four points for the spline segment
        glm::vec2 p0 = points[(i + points.size() - 1) % points.size()];
        glm::vec2 p1 = points[i];
        glm::vec2 p2 = points[(i + 1) % points.size()];
        glm::vec2 p3 = points[(i + 2) % points.size()];

        // Calculate points along the curve segment
        for (int j = 0; j < smoothness; ++j)
        {
            float t = static_cast<float>(j) / smoothness;
            glm::vec2 position = catmullRom(p0, p1, p2, p3, t);

            size_t index = i * smoothness + j + 1; // +1 because index 0 is center point
            filledShape[index].position = sf::Vector2f(position.x, position.y);
            filledShape[index].color = color; // You can modify the color as needed
        }
    }

    // Add final point to connect back to the start
    filledShape[points.size() * smoothness + 1].position = filledShape[1].position;
    filledShape[points.size() * smoothness + 1].color = color;

    // Draw the filled shape
    window.draw(filledShape);
}

// Fill in the area of color {color} between a smooth line through the points. May break for certain shapes, but works for some non-convex shapes, but is slower than drawing a convex shape due to the amount of triangles it has to draw.
void drawSmoothFillTube(const std::vector<glm::vec2> &points, sf::RenderWindow &window, sf::Color color = sf::Color::White)
{
    if (points.size() < 2)
        return;

    // Number of segments between each pair of points
    const int smoothness = 20;

    // Create vertex array for the filled shape using triangle fan
    sf::VertexArray filledShape(sf::TriangleStrip, points.size() * smoothness + 2);

    // Function to calculate catmull-rom spline point
    auto catmullRom = [](const glm::vec2 &p0, const glm::vec2 &p1,
                         const glm::vec2 &p2, const glm::vec2 &p3, float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        // Catmull-Rom matrix coefficients
        float b0 = -0.5f * t3 + t2 - 0.5f * t;
        float b1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
        float b2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        float b3 = 0.5f * t3 - 0.5f * t2;

        return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
    };

    // For each point
    for (size_t i = 0; i < (int)((points.size() + 1) / 2); ++i)
    {
        int idxA = i;
        int idxB = (-i + points.size()) % points.size();

        // Get four points for the first spline segment
        glm::vec2 pA0 = points[(i + points.size() - 1) % points.size()];
        glm::vec2 pA1 = points[i];
        glm::vec2 pA2 = points[(i + 1) % points.size()];
        glm::vec2 pA3 = points[(i + 2) % points.size()];

        // Get four points for the second spline segment
        glm::vec2 pB0 = points[(idxB + 1) % points.size()];
        glm::vec2 pB1 = points[idxB];
        glm::vec2 pB2 = points[(idxB - 1 + points.size()) % points.size()];
        glm::vec2 pB3 = points[(idxB - 2 + points.size()) % points.size()];

        // Calculate points along the curve segment
        for (int j = 0; j < smoothness; ++j)
        {
            float t = static_cast<float>(j) / smoothness;
            glm::vec2 positionA = catmullRom(pA0, pA1, pA2, pA3, t);
            glm::vec2 positionB = catmullRom(pB0, pB1, pB2, pB3, t);

            size_t index = 2 * (i * smoothness + j);
            filledShape[index].position = sf::Vector2f(positionA.x, positionA.y);
            filledShape[index].color = color;
            filledShape[index + 1].position = sf::Vector2f(positionB.x, positionB.y);
            filledShape[index + 1].color = color;
        }

        if (i == (int)((points.size() + 1) / 2) - 1)
        {
            glm::vec2 positionA = catmullRom(pA0, pA1, pA2, pA3, 1.0f);
            glm::vec2 positionB = catmullRom(pB0, pB1, pB2, pB3, 1.0f);

            size_t index = points.size() * smoothness;
            filledShape[index].position = sf::Vector2f(positionA.x, positionA.y);
            filledShape[index].color = color;
            filledShape[index + 1].position = sf::Vector2f(positionB.x, positionB.y);
            filledShape[index + 1].color = color;
        }
    }

    // Draw the filled shape
    window.draw(filledShape);
}

// Draw an ellipse at pos with {width, height} of size, rotated {rotation} degrees
void drawEllipse(sf::RenderWindow &window,
                 sf::Vector2f pos,
                 sf::Vector2f size,
                 float rotation = 0.f,
                 const sf::Color &fillColor = sf::Color::White)
{
    // Create a circle shape that we'll transform into an ellipse
    float baseRadius = std::max(size.x, size.y) / 2.0f;
    sf::CircleShape circle(baseRadius);
    circle.setFillColor(fillColor);

    // Transform
    circle.setOrigin(baseRadius, baseRadius);
    circle.setRotation(rotation);
    circle.setScale(size / (baseRadius));
    circle.setPosition(pos);

    // Draw the ellipse
    window.draw(circle);
}

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
    float maxTurnAngle;
    bool hooked = false;

    // Fish Appearance
    float eyeRadius = 0.025f;
    float normalFinRotation = 30.0f;
    float turnFinRotation = 20.0f;
    int finIndex;
    float finSize = FLT_MIN;

    Fish(float _linkDistance, float _moveSpeed, float _maxTurnAngle)
        : linkDistance(_linkDistance),
          moveSpeed(_moveSpeed),
          maxTurnAngle(_maxTurnAngle)
    {
    }

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
        drawEllipse(window, {finRight.x, finRight.y}, {finSize * 0.75f, finSize * 0.75f * 0.5f}, rightRotation - normalFinRotation - bodyAngle * turnFinRotation);

        glm::vec2 finLeft = jointLeft(finIndex);
        float leftRotation = getRotation(jointLeft(finIndex - 1) - finLeft);
        drawEllipse(window, {finLeft.x, finLeft.y}, {finSize * 0.75f, finSize * 0.75f * 0.5f}, leftRotation + normalFinRotation - bodyAngle * turnFinRotation);

        // Render tail fin
        int lastIdx = points.size() - 1;
        glm::vec2 lastPoint = points[lastIdx];
        glm::vec2 tailPoint = lastPoint - jointForward(lastIdx) * linkDistance;
        glm::vec2 tailMovePoint = tailPoint + (jointRight(lastIdx) - lastPoint) * 3.0f * curvature();
        drawSmoothFillConvex({lastPoint, tailPoint, tailMovePoint}, window, sf::Color::Cyan);
        drawSmoothLine({lastPoint, tailPoint, tailMovePoint}, window, true);

        // Render body
        drawSmoothFillTube(outlinePoints, window, sf::Color::Blue);

        // Render outline
        drawSmoothLine(outlinePoints, window, true);

        // Render eyes
        glm::vec2 rightEyePos = rotate(forward * sizes[0] * 0.5f, 90) + points[0];
        glm::vec2 leftEyePos = rotate(forward * sizes[0] * 0.5f, -90) + points[0];
        sf::CircleShape circle;
        circle.setRadius(eyeRadius);
        circle.setFillColor(sf::Color::Green);
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
    void addRandomFish()
    {
        float aspectRatio = worldWidth / worldHeight;
        float x = randFloat(randSeed) * worldHeight * aspectRatio - (worldHeight * aspectRatio / 2);
        float y = randFloat(randSeed) * worldHeight - (worldHeight / 2);

        float headSize = 0.15f;
        float linkDistance = headSize * (randFloat(randSeed) * 1.0f + 1.0f);
        float moveSpeed = randFloat(randSeed) * 3.0f + 0.5f;

        auto fish = std::make_unique<Fish>(linkDistance, moveSpeed, 30.0f);
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
    void update(float dt, glm::vec2 rodPosition)
    {
        accumulatedDt += dt;
        int nSteps = (int)(accumulatedDt / fixedDt);
        accumulatedDt -= nSteps * fixedDt;
        for (int i = 0; i < nSteps; i++)
        {
            updateAffectors(fixedDt);
            step(fixedDt);
            hookFish(rodPosition);
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

    void hookFish(glm::vec2 rodPosition)
    {
        for (auto &fish : allFish)
        {
            // Update fish position if hooked
            if (fish->hooked)
            {
                fish->setHeadPosition(rodPosition);
                continue;
            }

            // Get distance from rod
            glm::vec2 diff = fish->getHeadPosition() - rodPosition;
            float dist = glm::length(diff);

            // If close enough, hook fish
            if (dist < hookDist)
            {
                fish->setHooked(true);

                // Update fish position
                fish->setHeadPosition(rodPosition);
            }
        }
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
};
