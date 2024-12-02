#ifndef HELPER_UTILS_HPP
#define HELPER_UTILS_HPP

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

#endif