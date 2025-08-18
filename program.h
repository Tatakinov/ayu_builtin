#ifndef PROGRAM_H_
#define PROGRAM_H_

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

#include "glad/glad.h"
#include "texture.h"

class Program {
    private:
        GLuint id_;
        GLuint vao_;
        GLuint pos_vbo_;
        GLuint uv_vbo_;
    public:
        Program() {
            load();
        }
        ~Program();
        bool load();
        void use(const glm::mat4 &view);
        void set(GLuint id);
        operator GLuint() const {
            return id_;
        }
};
#endif // PROGRAM_H_
