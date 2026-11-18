#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

string readShader(const string& filepath)
{
    ifstream file(filepath);
    if (!file.is_open())
    {
        throw runtime_error("Couldn't Open shader " + filepath);
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint createShaderProgram(const string& vertexPath, const string& fragmentPath)
{
    GLint status;
    string vertexSource = readShader(vertexPath);
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexCode = vertexSource.c_str();
    glShaderSource(vertexShader, 1, &vertexCode, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Error:\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return 0;
    }

    string fragmentSource = readShader(fragmentPath);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentCode = fragmentSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentCode, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Error:\n" << infoLog << std::endl;
        glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Program Linking Error:\n" << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

struct Engine
{
    const int HEIGHT = 600;
    const int WIDTH = 800;
    GLuint VBO = 0;
    GLuint VAO = 0;
    GLFWwindow* window;
    int vertexCount = 0;   // number of vertices to draw

    Engine()
    {
        if (!glfwInit())
        {
            cout << "Couldn't Initialize GLFW";
            exit(EXIT_FAILURE);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Circle", nullptr, nullptr);
        if (window == nullptr)
        {
            cerr << "Unable to create Window";
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(window);
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();

        // Check for errors, but bypass the hard-coded Wayland/GLX false failure
        if (err != GLEW_OK && std::string((const char*)glewGetErrorString(err)) != "Unknown error")
        {
            std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        std::cout << "GLEW initialized successfully (Wayland fallback applied)." << std::endl;
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << '\n';
        std::cout << "GPU Renderer: " << glGetString(GL_RENDERER) << '\n';

        glViewport(0, 0, WIDTH, HEIGHT);
        makeCircle();
    }

void makeCircle()
{
    const int segments = 64;          // higher = smoother circle
    const float radius = 0.5f;
    const float cx = 0.0f;
    const float cy = 0.0f;

    // Correct for non-square window
    float aspect = static_cast<float>(HEIGHT) / static_cast<float>(WIDTH);  // 600/800 = 0.75

    std::vector<float> vertices;
    vertices.reserve((segments + 2) * 2);

    // Center vertex
    vertices.push_back(cx);
    vertices.push_back(cy);

    // Circumference vertices
    for (int i = 0; i <= segments; ++i)
    {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = cx + radius * cos(angle) * aspect;   // ← scale X by aspect
        float y = cy + radius * sin(angle);
        vertices.push_back(x);
        vertices.push_back(y);
    }

    vertexCount = static_cast<int>(vertices.size() / 2);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
};

int main()
{
    Engine engine;

    GLuint shaderProgram = createShaderProgram(
        "shaders/vertex.glsl",
        "shaders/fragment.glsl"
    );

    while (!glfwWindowShouldClose(engine.window))
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(engine.VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, engine.vertexCount);   // <-- triangle fan

        glfwSwapBuffers(engine.window);
        glfwPollEvents();
    }

    // Optional cleanup
    glDeleteVertexArrays(1, &engine.VAO);
    glDeleteBuffers(1, &engine.VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}