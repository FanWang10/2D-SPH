#include <iostream>
#include "glad.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <cstring>
#include <exception>


class app{
    private:
        int windowWidth;
        int windowHeight;
        GLFWwindow * window;
        GLuint renderProgram;
        GLuint computeProgram[3]{0, 0, 0};
        GLuint VAO;
        GLuint VBO;
    
    private:
        void initWindow();
        void destroyWindow();
        void initGLAD();
        void initOpenGL();
        void destroyOpenGL();
        GLuint compileShader(const char * path, GLenum shaderType);
        void checkShaderProgram(GLuint id, const char * type);
        void mainLoop();
        void sph();
        void render();

    public:
        app(int width, int height);
        void execute();

};// end of class-app