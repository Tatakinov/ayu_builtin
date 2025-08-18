#include "program.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "logger.h"

namespace {
    const char *vertex =
        "#version 410 core\n"
        "uniform mat4 mvp;\n"
        "layout (location = 0) in vec3 vertex_position;\n"
        "layout (location = 1) in vec2 vertex_uv;\n"
        "out vec2 uv;\n"
        "void main() {\n"
        "    uv = vertex_uv;\n"
        "    gl_Position = mvp * vec4(vertex_position, 1.0);\n"
        "}\n";

    const char *frag =
        "#version 410 core\n"
        "in vec2 uv;\n"
        "uniform sampler2D tex;\n"
        "layout(location = 0) out vec4 color;\n"
        "void main() {\n"
        "    color = texture2D(tex, uv);\n"
        "}\n";

    const float vertex_position[] = {
         1,  1,  0,
        -1,  1,  0,
        -1, -1,  0,
         1, -1,  0,
    };

    const GLfloat vertex_uv[] = {
        1, 0,
        0, 0,
        0, 1,
        1, 1,
    };

    glm::mat4 projection = glm::ortho(-1.0, 1.0, -1.0, 1.0, -100.0, 100.0);
    glm::mat4 model = glm::mat4(1.0f);
}

bool Program::load() {
    int success;
    char log[1024];
    id_ = glCreateProgram();
    assert(glGetError() == GL_NO_ERROR);
    const GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    assert(glGetError() == GL_NO_ERROR);
    glShaderSource(vshader, 1, &vertex, NULL);
    assert(glGetError() == GL_NO_ERROR);
    glCompileShader(vshader);
    assert(glGetError() == GL_NO_ERROR);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    assert(glGetError() == GL_NO_ERROR);
    if (!success) {
        glGetShaderInfoLog(vshader, 1024, NULL, log);
        assert(glGetError() == GL_NO_ERROR);
        Logger::log("Error.Shader: ", log);
        return false;
    }

    glAttachShader(id_, vshader);
    assert(glGetError() == GL_NO_ERROR);
    glDeleteShader(vshader);
    assert(glGetError() == GL_NO_ERROR);

    const GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    assert(glGetError() == GL_NO_ERROR);
    glShaderSource(fshader, 1, &frag, NULL);
    assert(glGetError() == GL_NO_ERROR);
    glCompileShader(fshader);
    assert(glGetError() == GL_NO_ERROR);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    assert(glGetError() == GL_NO_ERROR);
    if (!success) {
        glGetShaderInfoLog(fshader, 1024, NULL, log);
        assert(glGetError() == GL_NO_ERROR);
        Logger::log("Error.Shader: ", log);
        return false;
    }
    glAttachShader(id_, fshader);
    assert(glGetError() == GL_NO_ERROR);
    glDeleteShader(fshader);
    assert(glGetError() == GL_NO_ERROR);

    glLinkProgram(id_);
    assert(glGetError() == GL_NO_ERROR);
    glGetProgramiv(id_, GL_LINK_STATUS, &success);
    assert(glGetError() == GL_NO_ERROR);
    if (!success) {
        glGetProgramInfoLog(id_, 1024, NULL, log);
        assert(glGetError() == GL_NO_ERROR);
        Logger::log("Error.Program: ", log);
        return false;
    }

    glGenVertexArrays(1, &vao_);
    assert(glGetError() == GL_NO_ERROR);
    glBindVertexArray(vao_);
    assert(glGetError() == GL_NO_ERROR);

    glGenBuffers(1, &pos_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, pos_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_position), vertex_position, GL_STATIC_DRAW);

    glGenBuffers(1, &uv_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, uv_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_uv), vertex_uv, GL_STATIC_DRAW);

    auto position_location = glGetAttribLocation(id_, "vertex_position");
    auto uv_location = glGetAttribLocation(id_, "vertex_uv");
    glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, 0, 0);
    assert(glGetError() == GL_NO_ERROR);
    glEnableVertexAttribArray(position_location);
    assert(glGetError() == GL_NO_ERROR);
    glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
    assert(glGetError() == GL_NO_ERROR);
    glEnableVertexAttribArray(uv_location);
    assert(glGetError() == GL_NO_ERROR);
    return true;
}

Program::~Program() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(id_);
}

void Program::use(const glm::mat4 &view) {
    assert(glIsProgram(id_) == GL_TRUE);
    glUseProgram(id_);
    assert(glGetError() == GL_NO_ERROR);
    glBindVertexArray(vao_);
    assert(glGetError() == GL_NO_ERROR);
    glEnableVertexAttribArray(0);
    assert(glGetError() == GL_NO_ERROR);
    glBindBuffer(GL_ARRAY_BUFFER, pos_vbo_);
    assert(glGetError() == GL_NO_ERROR);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    assert(glGetError() == GL_NO_ERROR);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NO_ERROR);
    glBindBuffer(GL_ARRAY_BUFFER, uv_vbo_);
    assert(glGetError() == GL_NO_ERROR);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    assert(glGetError() == GL_NO_ERROR);
    auto mvp = projection * view * model;
    glUniformMatrix4fv(glGetUniformLocation(id_, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    assert(glGetError() == GL_NO_ERROR);
}

void Program::set(GLuint id) {
    glUniform1i(glGetUniformLocation(id_, "tex"), 0);
    assert(glGetError() == GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_2D, id);
    assert(glGetError() == GL_NO_ERROR);
}
