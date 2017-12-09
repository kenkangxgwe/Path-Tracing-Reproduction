//===============================================================================
// Basic Shader Program
// learnopengl.com
//===============================================================================

#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>



class Shader
{
public:
    GLuint Program;

    Shader(const char *vertexPath, const char *fragmentPath)
    {
        //declarations
        std::string vertexShaderCode;
        std::string fragmentShaderCode;
        std::ifstream vertexShaderFile;
        std::ifstream fragmentShaderFile;
        
        //opening from path
        vertexShaderFile.open(vertexPath);
        fragmentShaderFile.open(fragmentPath);

        //ss
        std::stringstream vShaderStream, fShaderStream;

        //read from buffer into stream
        vShaderStream << vertexShaderFile.rdbuf();
        fShaderStream << fragmentShaderFile.rdbuf();

        //closing to clean
        vertexShaderFile.close();
        fragmentShaderFile.close();

        //stream to string, string to c string
        vertexShaderCode = vShaderStream.str();
        fragmentShaderCode = fShaderStream.str();
        const char *vCode = vertexShaderCode.c_str();
        const char *fCode = fragmentShaderCode.c_str();

        //initializing shaders
        GLuint vertexShader, fragmentShader;
        GLint success;
        char infoLog[512];
        
        //creating vertex shader
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vCode, NULL);
        glCompileShader(vertexShader);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            //output error message on compilation failure
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "Error in vertex shader compilation:\n" << infoLog << std::endl;
        }
        //creating fragment shader
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fCode, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "Error in fragment shader compilation:\n" << infoLog << std::endl;
        }

        //creating program and linking
        this->Program = glCreateProgram();
        glAttachShader(this->Program, vertexShader);
        glAttachShader(this->Program, fragmentShader);
        glLinkProgram(this->Program);
        glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
            std::cout << "Error linking shader program:\n" << infoLog << std::endl;
        }
        //USELESS! GET RID OF THEM!
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

    }
    //a way to use the shader
    void Use()
    {
        glUseProgram(this->Program);
    }
};

#endif

