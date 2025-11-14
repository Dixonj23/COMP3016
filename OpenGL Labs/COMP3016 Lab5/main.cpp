//STD
#include <iostream>

//GLEW
#include <GL/glew.h>

//GLFW
#include <GLFW/glfw3.h>

#include "shaders/LoadShaders.h"

GLuint program;

using namespace std;

// Callback function for window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Update the OpenGL viewport
    glViewport(0, 0, width, height);
}

void ProcessUserInput(GLFWwindow* WindowIn)
{
    //Closes window on 'exit' key press
    if (glfwGetKey(WindowIn, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(WindowIn, true);
    }
}

//VAO vertex attribute positions in correspondence to vertex attribute type
enum VAO_IDs { Triangles, Indices, Colours, Textures, NumVAOs = 2 };
//VAOs
GLuint VAOs[NumVAOs];

//Buffer types
enum Buffer_IDs { ArrayBuffer, NumBuffers = 4 };
//Buffer objects
GLuint Buffers[NumBuffers];



int main()
{
    // ----- Initialize GLFW -----
    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW\n";
        return -1;
    }

    // ----- Create Window -----
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Lab5", NULL, NULL);

    if (window == NULL)
    {
        cout << "GLFW Window did not instantiate\n";
        glfwTerminate();
        return -1;
    }

    // Bind OpenGL context to this window
    glfwMakeContextCurrent(window);

    // ----- Initialize GLEW -----
    if (glewInit() != GLEW_OK)
    {
        cout << "Failed to initialize GLEW\n";
        glfwTerminate();
        return -1;
    }

    //Load shaders
    ShaderInfo shaders[] =
    {
        { GL_VERTEX_SHADER, "shaders/vertexShader.vert" },
        { GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag" },
        { GL_NONE, NULL }
    };

    program = LoadShaders(shaders);
    glUseProgram(program);

    // Set the initial viewport size
    glViewport(0, 0, 1280, 720);

    // Set the callback for window resizing
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    float vertices[] = {
        //positions             //colours
        0.5f, 0.5f, 0.0f,       1.0f, 0.0f, 0.0f, //top right
        0.5f, -0.5f, 0.0f,      0.0f, 1.0f, 0.0f, //bottom right
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f, //bottom left
        -0.5f, 0.5f, 0.0f,      1.0f, 1.0f, 1.0f //top left
    };

    unsigned int indices[] = {
        0, 1, 3, //first triangle
        1, 2, 3 //second triangle
    };

    //Declaration of index of VBO
    unsigned int vertexBufferObject;
    //Sets index of VBO
    glGenBuffers(1, &vertexBufferObject);
    //Binds VBO to array buffer for drawing vertices
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    //Allocates buffer memory for the vertices
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //Sets index of VAO
    glGenVertexArrays(NumVAOs, VAOs); //NumVAOs, VAOs
    //Binds VAO to a buffer
    glBindVertexArray(VAOs[0]); //VAOs[0]
    //Sets indexes of all required buffer objects
    glGenBuffers(NumBuffers, Buffers); //NumBuffers, Buffers
    //glGenBuffers(1, &EBO);

    //Binds vertex object to array buffer
    glBindBuffer(GL_ARRAY_BUFFER, Buffers[Triangles]);
    //Allocates buffer memory for the vertices of the 'Triangles' buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //Binding & allocation for indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[Indices]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    //Allocation & indexing of vertex attribute memory for vertex shader
//Positions
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

//Colours
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
glEnableVertexAttribArray(1);

    //Unbinding
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // ----- Main Loop -----
    while (!glfwWindowShouldClose(window))
    {
        //Input
        ProcessUserInput(window); //Takes user input

        //Rendering
        glClearColor(0.25f, 0.0f, 1.0f, 1.0f); //Colour to display on cleared window
        glClear(GL_COLOR_BUFFER_BIT); //Clears the colour buffer

        glBindVertexArray(VAOs[0]); //Bind buffer object to render
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //Refreshing
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ----- Cleanup -----
    glfwTerminate();
    return 0;
}

