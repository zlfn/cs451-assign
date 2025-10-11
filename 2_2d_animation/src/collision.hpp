#pragma once

#include <glm/glm.hpp>
#include <variant>
#include <concepts>

struct CollisionCircle;
struct CollisionRectangle;
using CollisionShape = std::variant<CollisionCircle, CollisionRectangle>;

// 원형 충돌 영역
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

// 축에 정렬된 사각형 충돌 영역
struct CollisionRectangle {
    glm::vec2 topLeft;
    glm::vec2 bottomRight;
    CollisionRectangle(const glm::vec2 &tl, const glm::vec2 &br) : topLeft(tl), bottomRight(br) {}

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

// 충돌 가능한 객체의 인터페이스
struct Collidable {
    virtual CollisionShape getShape() const = 0;
    virtual ~Collidable() = default;
};

// Shape 인터페이스를 구현하는 도형의 콘셉트
template <typename T>
concept ShapeConcept = requires(const T &a, const CollisionCircle &c, const CollisionRectangle &r) {
    { a.intersects(c) } -> std::same_as<bool>;
    { a.intersects(r) } -> std::same_as<bool>;
};

// 도형 간 충돌 감지
template <ShapeConcept A, ShapeConcept B> bool detectCollision(const A &a, const B &b) {
    return a.intersects(b);
}

// Collidable 인터페이스를 구현하는 객체의 콘셉트
template <typename T>
concept CollidableObject = std::is_base_of_v<Collidable, T>;

// 객체 간 충돌 감지 (충돌 영역 사용)
template <CollidableObject A, CollidableObject B> bool detectCollision(const A &a, const B &b) {
    const CollisionShape SHAPE_A = a.getShape();
    const CollisionShape SHAPE_B = b.getShape();

    return std::visit([](const auto &s1, const auto &s2) { return s1.intersects(s2); }, SHAPE_A,
                      SHAPE_B);
}