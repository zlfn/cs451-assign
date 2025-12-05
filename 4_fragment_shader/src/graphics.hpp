#pragma once

#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <functional>

// 단일 셰이더 (vertex, fragment 등)
class Shader {
public:
    enum class Type: std::uint16_t {
        VERTEX = GL_VERTEX_SHADER,
        FRAGMENT = GL_FRAGMENT_SHADER,
        GEOMETRY = GL_GEOMETRY_SHADER
    };

    // 파일에서 셰이더 생성
    static Shader fromFile(Type type, const std::string& file_path);

    // 소스 코드로 셰이더 생성
    static Shader fromSource(Type type, const std::string& source_code);

    ~Shader();

    // 복사 금지
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // 이동 허용
    // 셰이더를 이동할 때 항상 std::move 사용
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    GLuint getId() const { return shaderId_; }
    bool isCompiled() const { return compiled_; }

private:
    Shader(Type type);  // private 생성자

    GLuint shaderId_;
    bool compiled_;

    void compile(const std::string& source);
    static std::string readFile(const std::string& filePath);
};

// 셰이더 프로그램 (vertex + fragment 조합)
class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    // 복사 금지
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // 이동 허용
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void attachShader(const Shader& shader);
    void link();
    void use() const;

    GLuint getId() const { return programId_; }
    bool isLinked() const { return linked_; }

    // Uniform 설정 헬퍼 함수들
    void setUniform(const std::string& name, int value) const;
    void setUniform(const std::string& name, float value) const;
    void setUniform(const std::string& name, const glm::vec2& value) const;
    void setUniform(const std::string& name, const glm::vec3& value) const;
    void setUniform(const std::string& name, const glm::vec4& value) const;
    void setUniform(const std::string& name, const glm::mat4& value) const;

private:
    GLuint programId_;
    bool linked_;

    GLint getUniformLocation(const std::string& name) const;
};

// VBO (Vertex Buffer Object)
// 정점 데이터를 GPU 메모리에 저장하는 버퍼
class VertexBuffer {
public:
    VertexBuffer();
    virtual ~VertexBuffer();

    // 복사 금지
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    // 이동 허용
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    void bind() const;
    void setData(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW);

    GLuint getId() const { return vbo_; }
    bool hasData() const { return hasData_; }

protected:
    GLuint vbo_;
    bool hasData_;
};

// VAO (Vertex Array Object)
// VBO의 정점 속성 설정을 저장하고 관리
//
// VAO와 VBO 연결 및 사용 방법:
// 1. VAO 생성 및 바인딩
//    VertexArray vao;
//    vao.bind();
//
// 2. VBO 생성 및 데이터 업로드
//    VertexBuffer vbo;
//    vbo.setData(vertices, sizeof(vertices));
//
// 3. VAO에 정점 속성 설정 (VAO가 바인딩된 상태에서 VBO도 바인딩되어야 함)
//    vao.enableAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);  // position
//    vao.enableAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));  // color
//
// 4. 렌더링 시 VAO만 바인딩하면 모든 설정이 자동으로 적용됨
//    vao.bind();
//    glDrawArrays(GL_TRIANGLES, 0, 3);
//
// 핵심: VAO가 바인딩된 상태에서 enableAttribute를 호출하면
//       현재 바인딩된 VBO와 속성 설정이 VAO에 저장됨
//
// **VAO는 VBO의 데이터를 복사하지 않고 참조만 저장함**
// 따라서 VAO를 사용하는 동안 VBO도 반드시 살아있어야 함!
// VBO가 먼저 삭제되면 VAO는 유효하지 않은 메모리를 참조하게 됨
class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    // 복사 금지
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    // 이동 허용
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind() const;

    // 정점 속성 활성화 및 설정
    // index: shader의 location (layout(location = 0) in vec3 pos 면 0)
    // size: 컴포넌트 개수 (vec3면 3, vec2면 2)
    // type: 데이터 타입 (GL_FLOAT, GL_INT 등)
    // stride: 다음 정점까지의 바이트 간격 (interleaved 배열에서 사용)
    // offset: 버퍼 내에서 이 속성의 시작 위치
    void enableAttribute(GLuint index, GLint size, GLenum type,
                        GLsizei stride, const void* offset);

    GLuint getId() const { return vao_; }
    bool hasAttribute() const { return hasAttribute_; }

private:
    GLuint vao_;
    bool hasAttribute_;
};

// Mesh - VAO와 VBO를 함께 관리하는 클래스
// VAO와 VBO의 생명주기를 자동으로 관리하여 안전하게 사용 가능
//
// 사용 예시:
//    Mesh mesh;
//    mesh.setData(vertices, sizeof(vertices));
//    mesh.setAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
//    mesh.setAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
//    mesh.setDrawMode(GL_TRIANGLES, 36);
//
//    // 렌더링
//    draw(mesh, shader, [&](const ShaderProgram& prog) { ... });
class Mesh {
public:
    Mesh();
    ~Mesh() = default;

    // 복사 금지
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // 이동 허용
    Mesh(Mesh&& other) noexcept = default;
    Mesh& operator=(Mesh&& other) noexcept = default;

    // VBO에 데이터 업로드
    void setData(const void* data, size_t size, GLenum usage = GL_STATIC_DRAW);

    // 정점 속성 설정
    void setAttribute(GLuint index, GLint size, GLenum type,
                     GLsizei stride, const void* offset);

    // 렌더링 모드 및 정점 개수 설정
    void setDrawMode(GLenum mode, GLsizei count, GLint first = 0);

    // 렌더링 시 호출
    void bind() const;

    VertexArray& getVAO() { return vao_; }
    VertexBuffer& getVBO() { return vbo_; }

    GLenum getMode() const { return mode_; }
    GLsizei getCount() const { return count_; }
    GLint getFirst() const { return first_; }

    bool isReady() const { return hasData_ && hasAttribute_ && hasDrawMode_; }

private:
    VertexBuffer vbo_;
    VertexArray vao_;
    GLenum mode_;
    GLsizei count_;
    GLint first_;

    bool hasData_;
    bool hasAttribute_;
    bool hasDrawMode_;
};

// 렌더링 헬퍼 함수
// Mesh와 ShaderProgram을 사용하여 렌더링
// Mesh에 저장된 mode, count, first를 사용
//
// 사용 예시:
//    mesh.setDrawMode(GL_TRIANGLES, 36);
//    draw(mesh, shader, [&](const ShaderProgram& prog) {
//        prog.setUniform("model", modelMatrix);
//        prog.setUniform("view", viewMatrix);
//        prog.setUniform("projection", projectionMatrix);
//    });
//
// setupUniforms가 nullptr이면 uniform 설정을 건너뜁니다.
void drawMesh(const Mesh& mesh, const ShaderProgram& shader,
              const std::function<void(const ShaderProgram&)>& setupUniforms = nullptr);
