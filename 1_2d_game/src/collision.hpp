#pragma once
#include <glm/glm.hpp>
#include <variant>
#include <concepts>

// Forward declarations for collision shapes
struct CollisionCircle;
struct CollisionRectangle;
using CollisionShape = std::variant<CollisionCircle, CollisionRectangle>;

/// @brief Circle collision shape
struct CollisionCircle {
    glm::vec2 center;
    float raidus;
    CollisionCircle(glm::vec2 c, float r) : center(c), raidus(r) {}

    bool intersects(const CollisionCircle &other) const {
        float distance = glm::length(center - other.center);
        return distance < (raidus + other.raidus);
    }
    bool intersects(const CollisionRectangle &rect) const;
};

/// @brief Axis-aligned rectangle collision shape
struct CollisionRectangle {
    glm::vec2 topLeft;
    glm::vec2 bottomRight;
    CollisionRectangle(glm::vec2 tl, glm::vec2 br) : topLeft(tl), bottomRight(br) {}

    bool intersects(const CollisionCircle &circle) const { return circle.intersects(*this); }
    bool intersects(const CollisionRectangle &other) const {
        return !(topLeft.x > other.bottomRight.x || bottomRight.x < other.topLeft.x ||
                 topLeft.y > other.bottomRight.y || bottomRight.y < other.topLeft.y);
    }
};

inline bool CollisionCircle::intersects(const CollisionRectangle &rect) const {
    float closestX = glm::clamp(center.x, rect.topLeft.x, rect.bottomRight.x);
    float closestY = glm::clamp(center.y, rect.topLeft.y, rect.bottomRight.y);

    float distanceX = center.x - closestX;
    float distanceY = center.y - closestY;

    float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
    return distanceSquared < (raidus * raidus);
}

/// @brief Interface for objects that can be collided with
struct Collidable {
    /// @brief Get the collision shape of the object
    /// @return The collision shape
    virtual CollisionShape getShape() const = 0;
    virtual ~Collidable() = default;
};

/// @brief Concept for shapes that implement the Shape Interface
/// @tparam T Type to check
template <typename T>
concept ShapeConcept = requires(const T &a, const CollisionCircle &c, const CollisionRectangle &r) {
    { a.intersects(c) } -> std::same_as<bool>;
    { a.intersects(r) } -> std::same_as<bool>;
};

/// @brief Shape-to-shape collision detect
/// @tparam A Type of the first shape
/// @tparam B Type of the second shape
/// @param a The first shape
/// @param b The second shape
template <ShapeConcept A, ShapeConcept B> bool detectCollision(const A &a, const B &b) {
    return a.intersects(b);
}

/// @brief Concept for objects that implement the Collidable Interface
/// @tparam T Type to check
/// @return true if T is derived from Collidable
template <typename T>
concept CollidableObject = std::is_base_of_v<Collidable, T>;

/// @breif Object-to-object collision detection (using their shapes)
/// @tparam A Type of the first collidable objects
/// @tparam B Type of the second collidable objects
/// @param a The first collidable objects
/// @param b The second collidable objects
/// @return true if the objects collide
template <CollidableObject A, CollidableObject B> bool detectCollision(const A &a, const B &b) {
    const CollisionShape shapeA = a.getShape();
    const CollisionShape shapeB = b.getShape();

    return std::visit([](const auto &s1, const auto &s2) { return s1.intersects(s2); }, shapeA,
                      shapeB);
}