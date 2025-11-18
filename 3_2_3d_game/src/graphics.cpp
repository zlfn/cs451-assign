#include "graphics.hpp"
#include <glm/gtc/type_ptr.hpp>

// Shader

Shader::Shader(Type type)
    : shaderId_(0), compiled_(false) {
    shaderId_ = glCreateShader(static_cast<GLenum>(type));
    if (shaderId_ == 0) {
        throw std::runtime_error("Failed to create shader");
    }
}

Shader Shader::fromFile(Type type, const std::string& file_path) {
    Shader shader(type);
    std::string source = readFile(file_path);
    shader.compile(source);
    return shader;
}

Shader Shader::fromSource(Type type, const std::string& source_code) {
    Shader shader(type);
    shader.compile(source_code);
    return shader;
}

Shader::~Shader() {
    if (shaderId_ != 0) {
        glDeleteShader(shaderId_);
    }
}

Shader::Shader(Shader&& other) noexcept
    : shaderId_(other.shaderId_), compiled_(other.compiled_) {
    other.shaderId_ = 0;
    other.compiled_ = false;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (shaderId_ != 0) {
            glDeleteShader(shaderId_);
        }
        shaderId_ = other.shaderId_;
        compiled_ = other.compiled_;
        other.shaderId_ = 0;
        other.compiled_ = false;
    }
    return *this;
}

void Shader::compile(const std::string& source) {
    const char* sourcePtr = source.c_str();
    glShaderSource(shaderId_, 1, &sourcePtr, nullptr);
    glCompileShader(shaderId_);

    GLint success = false;
    glGetShaderiv(shaderId_, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shaderId_, 512, nullptr, infoLog);
        std::string errorMsg = "Shader compilation failed: ";
        errorMsg += infoLog;
        throw std::runtime_error(errorMsg);
    }

    compiled_ = true;
}

std::string Shader::readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ShaderProgram

ShaderProgram::ShaderProgram()
    : programId_(0), linked_(false) {
    programId_ = glCreateProgram();
    if (programId_ == 0) {
        throw std::runtime_error("Failed to create shader program");
    }
}

ShaderProgram::~ShaderProgram() {
    if (programId_ != 0) {
        glDeleteProgram(programId_);
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : programId_(other.programId_), linked_(other.linked_) {
    other.programId_ = 0;
    other.linked_ = false;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        if (programId_ != 0) {
            glDeleteProgram(programId_);
        }
        programId_ = other.programId_;
        linked_ = other.linked_;
        other.programId_ = 0;
        other.linked_ = false;
    }
    return *this;
}

void ShaderProgram::attachShader(const Shader& shader) {
    if (!shader.isCompiled()) {
        throw std::runtime_error("Cannot attach uncompiled shader");
    }
    glAttachShader(programId_, shader.getId());
}

void ShaderProgram::link() {
    glLinkProgram(programId_);

    GLint success = false;
    glGetProgramiv(programId_, GL_LINK_STATUS, &success);

    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(programId_, 512, nullptr, infoLog);
        std::string errorMsg = "Shader program linking failed: ";
        errorMsg += infoLog;
        throw std::runtime_error(errorMsg);
    }

    linked_ = true;
}

void ShaderProgram::use() const {
    if (!linked_) {
        throw std::runtime_error("Cannot use unlinked shader program");
    }
    glUseProgram(programId_);
}

GLint ShaderProgram::getUniformLocation(const std::string& name) const {
    GLint location = glGetUniformLocation(programId_, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' not found in shader program" << '\n';
    }
    return location;
}

void ShaderProgram::setUniform(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void ShaderProgram::setUniform(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void ShaderProgram::setUniform(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

// VertexBuffer

VertexBuffer::VertexBuffer()
    : vbo_(0), hasData_(false) {
    glGenBuffers(1, &vbo_);
    if (vbo_ == 0) {
        throw std::runtime_error("Failed to create vertex buffer");
    }
}

VertexBuffer::~VertexBuffer() {
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
    }
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : vbo_(other.vbo_), hasData_(other.hasData_) {
    other.vbo_ = 0;
    other.hasData_ = false;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        if (vbo_ != 0) {
            glDeleteBuffers(1, &vbo_);
        }
        vbo_ = other.vbo_;
        hasData_ = other.hasData_;
        other.vbo_ = 0;
        other.hasData_ = false;
    }
    return *this;
}

void VertexBuffer::bind() const {
    if (!hasData_) {
        throw std::runtime_error("Cannot bind VertexBuffer: no data has been set");
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
}

void VertexBuffer::setData(const void* data, size_t size, GLenum usage) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, (long)size, data, usage);
    hasData_ = true;
}

// VertexArray

VertexArray::VertexArray()
    : vao_(0), hasAttribute_(false) {
    glGenVertexArrays(1, &vao_);
    if (vao_ == 0) {
        throw std::runtime_error("Failed to create vertex array");
    }
}

VertexArray::~VertexArray() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
    }
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : vao_(other.vao_), hasAttribute_(other.hasAttribute_) {
    other.vao_ = 0;
    other.hasAttribute_ = false;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (vao_ != 0) {
            glDeleteVertexArrays(1, &vao_);
        }
        vao_ = other.vao_;
        hasAttribute_ = other.hasAttribute_;
        other.vao_ = 0;
        other.hasAttribute_ = false;
    }
    return *this;
}

void VertexArray::bind() const {
    if (!hasAttribute_) {
        throw std::runtime_error("Cannot bind VertexArray: no attributes have been set");
    }
    glBindVertexArray(vao_);
}

void VertexArray::enableAttribute(GLuint index, GLint size, GLenum type,
                                  GLsizei stride, const void* offset) {
    glBindVertexArray(vao_);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, GL_FALSE, stride, offset);
    hasAttribute_ = true;
}

// Mesh

Mesh::Mesh()
    : vbo_(), vao_(), mode_(GL_TRIANGLES), count_(0), first_(0),
      hasData_(false), hasAttribute_(false), hasDrawMode_(false) {
}

void Mesh::setData(const void* data, size_t size, GLenum usage) {
    vbo_.setData(data, size, usage);
    hasData_ = true;
}

void Mesh::setAttribute(GLuint index, GLint size, GLenum type,
                       GLsizei stride, const void* offset) {
    glBindVertexArray(vao_.getId());
    glBindBuffer(GL_ARRAY_BUFFER, vbo_.getId());
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, GL_FALSE, stride, offset);
    hasAttribute_ = true;
}

void Mesh::setDrawMode(GLenum mode, GLsizei count, GLint first) {
    mode_ = mode;
    count_ = count;
    first_ = first;
    hasDrawMode_ = true;
}

void Mesh::bind() const {
    if (!isReady()) {
        std::string msg = "Cannot bind Mesh: missing required setup - ";
        if (!hasData_) msg += "setData() ";
        if (!hasAttribute_) msg += "setAttribute() ";
        if (!hasDrawMode_) msg += "setDrawMode() ";
        throw std::runtime_error(msg);
    }
    glBindVertexArray(vao_.getId());
}

// Draw functions

void drawMesh(const Mesh& mesh, const ShaderProgram& shader,
              const std::function<void(const ShaderProgram&)>& setupUniforms) {
    shader.use();

    if (setupUniforms) {
        setupUniforms(shader);
    }

    mesh.bind();
    glDrawArrays(mesh.getMode(), mesh.getFirst(), mesh.getCount());
}
