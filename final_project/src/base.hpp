#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <numbers>
#include <vector>
#include <complex>
#include "collision.hpp"
#include "utils.hpp"

#define FPS 60
#define TIMER_INTERVAL (1000 / FPS)

// 그릴 수 있는 객체의 인터페이스
struct Drawable {
    virtual void draw() = 0;
    virtual ~Drawable() = default;
};

// 업데이트 가능한 객체의 인터페이스
struct Updatable {
    // 객체의 상태를 업데이트. 제거해야 할 경우 true 반환
    virtual bool update(int currentTime) = 0;
    virtual ~Updatable() = default;
};
