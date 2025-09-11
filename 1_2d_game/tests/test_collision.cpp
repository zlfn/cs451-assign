#include <iostream>
#include "../src/collision.h"

int main() {
    int testsPassed = 0;
    int totalTests = 0;
    
    std::cout << "Running Collision Detection Tests\n";
    std::cout << "==================================\n";
    
    // Test 1: Circle-Circle collision (should collide)
    {
        totalTests++;
        CollisionCircle circle1(glm::vec2(0.0f, 0.0f), 5.0f);
        CollisionCircle circle2(glm::vec2(3.0f, 4.0f), 3.0f);
        bool result = circle1.intersects(circle2);
        if (result) {
            std::cout << "[PASS] Test 1: Circle-Circle collision (overlapping)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 1: Circle-Circle collision (overlapping) - Expected: true, Got: false\n";
        }
    }
    
    // Test 2: Circle-Circle no collision
    {
        totalTests++;
        CollisionCircle circle1(glm::vec2(0.0f, 0.0f), 2.0f);
        CollisionCircle circle2(glm::vec2(10.0f, 0.0f), 2.0f);
        bool result = circle1.intersects(circle2);
        if (!result) {
            std::cout << "[PASS] Test 2: Circle-Circle no collision (far apart)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 2: Circle-Circle no collision (far apart) - Expected: false, Got: true\n";
        }
    }
    
    // Test 3: Rectangle-Rectangle collision
    {
        totalTests++;
        CollisionRectangle rect1(glm::vec2(0.0f, 0.0f), glm::vec2(5.0f, 5.0f));
        CollisionRectangle rect2(glm::vec2(3.0f, 3.0f), glm::vec2(8.0f, 8.0f));
        bool result = rect1.intersects(rect2);
        if (result) {
            std::cout << "[PASS] Test 3: Rectangle-Rectangle collision (overlapping)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 3: Rectangle-Rectangle collision (overlapping) - Expected: true, Got: false\n";
        }
    }
    
    // Test 4: Rectangle-Rectangle no collision
    {
        totalTests++;
        CollisionRectangle rect1(glm::vec2(0.0f, 0.0f), glm::vec2(2.0f, 2.0f));
        CollisionRectangle rect2(glm::vec2(5.0f, 5.0f), glm::vec2(7.0f, 7.0f));
        bool result = rect1.intersects(rect2);
        if (!result) {
            std::cout << "[PASS] Test 4: Rectangle-Rectangle no collision (separate)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 4: Rectangle-Rectangle no collision (separate) - Expected: false, Got: true\n";
        }
    }
    
    // Test 5: Circle-Rectangle collision (circle inside rectangle)
    {
        totalTests++;
        CollisionCircle circle(glm::vec2(2.5f, 2.5f), 1.0f);
        CollisionRectangle rect(glm::vec2(0.0f, 0.0f), glm::vec2(5.0f, 5.0f));
        bool result = circle.intersects(rect);
        if (result) {
            std::cout << "[PASS] Test 5: Circle-Rectangle collision (circle inside)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 5: Circle-Rectangle collision (circle inside) - Expected: true, Got: false\n";
        }
    }
    
    // Test 6: Circle-Rectangle collision (circle overlapping edge)
    {
        totalTests++;
        CollisionCircle circle(glm::vec2(6.0f, 2.5f), 2.0f);
        CollisionRectangle rect(glm::vec2(0.0f, 0.0f), glm::vec2(5.0f, 5.0f));
        bool result = circle.intersects(rect);
        if (result) {
            std::cout << "[PASS] Test 6: Circle-Rectangle collision (overlapping edge)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 6: Circle-Rectangle collision (overlapping edge) - Expected: true, Got: false\n";
        }
    }
    
    // Test 7: Circle-Rectangle no collision
    {
        totalTests++;
        CollisionCircle circle(glm::vec2(10.0f, 10.0f), 2.0f);
        CollisionRectangle rect(glm::vec2(0.0f, 0.0f), glm::vec2(5.0f, 5.0f));
        bool result = circle.intersects(rect);
        if (!result) {
            std::cout << "[PASS] Test 7: Circle-Rectangle no collision (far apart)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 7: Circle-Rectangle no collision (far apart) - Expected: false, Got: true\n";
        }
    }
    
    // Test 8: Rectangle-Circle collision
    {
        totalTests++;
        CollisionCircle circle(glm::vec2(2.5f, 2.5f), 1.0f);
        CollisionRectangle rect(glm::vec2(0.0f, 0.0f), glm::vec2(5.0f, 5.0f));
        bool result = rect.intersects(circle);
        if (result) {
            std::cout << "[PASS] Test 8: Rectangle-Circle collision\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 8: Rectangle-Circle collision - Expected: true, Got: false\n";
        }
    }
    
    // Test 9: Circles touching at boundary
    {
        totalTests++;
        CollisionCircle circle1(glm::vec2(0.0f, 0.0f), 5.0f);
        CollisionCircle circle2(glm::vec2(10.0f, 0.0f), 5.0f);
        bool result = circle1.intersects(circle2);
        if (!result) {
            std::cout << "[PASS] Test 9: Circle-Circle touching at boundary\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 9: Circle-Circle touching at boundary - Expected: false, Got: true\n";
        }
    }
    
    // Test 10: Using detectCollision template
    {
        totalTests++;
        CollisionCircle circle(glm::vec2(2.0f, 2.0f), 2.0f);
        CollisionRectangle rect(glm::vec2(0.0f, 0.0f), glm::vec2(5.0f, 5.0f));
        bool result = detectCollision(circle, rect);
        if (result) {
            std::cout << "[PASS] Test 10: detectCollision template (circle-rect)\n";
            testsPassed++;
        } else {
            std::cout << "[FAIL] Test 10: detectCollision template (circle-rect) - Expected: true, Got: false\n";
        }
    }
    
    std::cout << "==================================\n";
    std::cout << "Tests passed: " << testsPassed << "/" << totalTests << "\n";
    
    return (testsPassed == totalTests) ? 0 : 1;
}