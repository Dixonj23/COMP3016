//STD
#include <iostream>

//GLAD
#include <glad/glad.h>

//GLM
#include "glm/ext/vector_float3.hpp"
#include <glm/gtc/type_ptr.hpp> 

//ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//LEARNOPENGL
#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>

//GENERAL
#include "main.h"
#include <ctime>

/*

    Quick not: im very new to OpenGL so i've included a frankly absurd amount of comments,
    just so i can remember what everything is/does. my apologies now for all the green text

*/

using namespace std;
using namespace glm;

//Window
int windowWidth;
int windowHeight;

//VAO vertex attribute positions in correspondence to vertex attribute type
enum VAO_IDs { Triangles, Indices, Colours, Textures, NumVAOs = 2 };
//VAOs
GLuint VAOs[NumVAOs];

//Buffer types
enum Buffer_IDs { ArrayBuffer, NumBuffers = 4 };
//Buffer objects
GLuint Buffers[NumBuffers];

//Transformations
//Relative position within world space
vec3 cameraPosition = vec3(0.0f, 0.0f, 3.0f);
//The direction of travel
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
//Up position within world space
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

//Camera sidways rotation
float cameraYaw = -90.0f;
//Camera vertical rotation
float cameraPitch = 0.0f;
//Determines if first entry of mouse into window
bool mouseFirstEntry = true;
//Positions of camera from given last frame
float cameraLastXPos = 800.0f / 2.0f;
float cameraLastYPos = 600.0f / 2.0f;

//Time
//Time change
float deltaTime = 0.0f;
//Last value of time change
float lastFrame = 0.0f;

//Runner movement
float forwardSpeed = 20.0f; 
int currentLane = 0;          // -1 = left, 0 = centre, 1 = right
const float laneWidth = 1.5f;

//Obstacle (rock)
vec3 rockPosition = vec3(0.0f, -1.0f, -15.0f);
int rockLane = 0; // -1, 0, 1

// Collision 
const float collisionZDistance = 1.5f;

// Lane tiles
const int NUM_LANES = 3;
const int TILES_PER_LANE = 8;

const float TILE_WORLD_LENGTH = 20.0f;
const float TILE_RECYCLE_BEHIND = 30.0f;
const float TILE_OVERLAP = 2.0f;

float tileZ[NUM_LANES][TILES_PER_LANE] =
{
    { 0.0f, -TILE_WORLD_LENGTH },  // Left lane
    { 0.0f, -TILE_WORLD_LENGTH },  // Centre lane
    { 0.0f, -TILE_WORLD_LENGTH }   // Right lane
};

const float tileLength = 20.0f;
const float tileHeight = 1.0f;

float GetFurthestTileZ(int lane)
{
    float furthest = tileZ[lane][0];
    for (int i = 1; i < TILES_PER_LANE; i++)
    {
        if (tileZ[lane][i] < furthest)
            furthest = tileZ[lane][i];
    }
    return furthest;
}

int main()
{
    srand((unsigned int)time(nullptr));

    //Initialisation of GLFW
    glfwInit();
    //Initialisation of 'GLFWwindow' object
    windowWidth = 1280;
    windowHeight = 720;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Lab9", NULL, NULL);

    //Checks if window has been successfully instantiated
    if (window == NULL)
    {
        cout << "GLFW Window did not instantiate\n";
        glfwTerminate();
        return -1;
    }


    //Sets cursor to automatically bind to window & hides cursor pointer
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //Binds OpenGL to window
    glfwMakeContextCurrent(window);

    //Initialisation of GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "GLAD failed to initialise\n";
        return -1;
    }


    //NEED THIS TO PROPERLY LOAD OBJECTS
    glEnable(GL_DEPTH_TEST);

    //Loading of shaders
    Shader Shaders("shaders/vertexShader.vert", "shaders/fragmentShader.frag");
    Model Rock("media/rock/Rock07-Base.obj");
    Model Tile("media/tiles/bridge-straight.obj");
    Shaders.use();

    //Sets the viewport size within the window to match the window size of 1280x720
    glViewport(0, 0, 1280, 720);

    //Sets the framebuffer_size_callback() function as the callback for the window resizing event
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //Sets the mouse_callback() function as the callback for the mouse movement event
    glfwSetCursorPosCallback(window, mouse_callback);
    

    //Projection matrix
    mat4 projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    //Render loop
    while (glfwWindowShouldClose(window) == false)
    {
        //Time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Automatic forward movement (Temple Runner style)
        cameraPosition.z -= forwardSpeed * deltaTime;

        // Snap camera X position to current lane
        cameraPosition.x = currentLane * laneWidth;

        //Obstacle position resets
        rockPosition.x = rockLane * laneWidth;

        //Model matrix
        mat4 model = mat4(1.0f);
        //Model position
        model = translate(model, rockPosition);
        //Scaling to zoom in
        model = scale(model, vec3(0.025f));

        // Collision check
        bool sameLane = (currentLane == rockLane);
        bool closeEnoughZ = abs(rockPosition.z - cameraPosition.z) < collisionZDistance;

        if (sameLane && closeEnoughZ)
        {
            // Reset player and obstacle
            cameraPosition.z = 0.0f;
            rockPosition.z = -15.0f;
            rockLane = (rand() % 3) - 1;

            std::cout << "Collision!" << std::endl;
        }

        //Input
        ProcessUserInput(window); //Takes user input

        //Rendering
        glClearColor(0.25f, 0.0f, 1.0f, 1.0f); //Colour to display on cleared window
        glClear(GL_COLOR_BUFFER_BIT); //Clears the colour buffer
        glClear(GL_DEPTH_BUFFER_BIT); //Clears the depth buffer

        glEnable(GL_CULL_FACE); //Discards all back-facing triangles

        //Transformations
        mat4 view = lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp); //Sets the position of the viewer, the movement direction in relation to it & the world up direction
        mat4 mvp = projection * view * model;
        Shaders.setMat4("mvpIn", mvp); //Setting of uniform with Shader class

        //Obstacle Positioning
        if (rockPosition.z > cameraPosition.z)
        {
            rockPosition.z = cameraPosition.z - 40.0f;

            // Randomise lane
            rockLane = (rand() % 3) - 1; // -1, 0, 1
        }

       // std::cout << "Camera Z: " << cameraPosition.z << " Rock Z: " << rockPosition.z << std::endl;

        //update floor positions
        for (int lane = 0; lane < NUM_LANES; lane++)
        {
            for (int i = 0; i < TILES_PER_LANE; i++)
            {
                if (tileZ[lane][i] > cameraPosition.z + TILE_RECYCLE_BEHIND)
                {
                    float furthestZ = GetFurthestTileZ(lane);
                    tileZ[lane][i] = (furthestZ + TILE_OVERLAP) - TILE_WORLD_LENGTH;
                }
            }
        }

        //Drawing
        for (int lane = 0; lane < NUM_LANES; lane++)
        {
            float laneX = (lane - 1) * laneWidth; // -1, 0, +1

            for (int i = 0; i < TILES_PER_LANE; i++)
            {
                mat4 tileModel = mat4(1.0f);

                mat4 T = translate(mat4(1.0f), vec3(laneX, -2.5f, tileZ[lane][i]));

                mat4 R = mat4(1.0f);

                 R = rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 1.0f, 0.0f));

                mat4 S = scale(mat4(1.0f), vec3(TILE_WORLD_LENGTH, tileHeight, laneWidth * 0.9f));

                // World-space translation 
                tileModel = T * R * S;


                

                mat4 tileMVP = projection * view * tileModel;
                Shaders.setMat4("mvpIn", tileMVP);

                Tile.Draw(Shaders);
            }
        }


        mat4 rockModel = mat4(1.0f);
        rockModel = translate(rockModel, rockPosition);
        rockModel = scale(rockModel, vec3(0.025f));

        Shaders.setMat4("mvpIn", projection * view * rockModel);
        Rock.Draw(Shaders);

        //Refreshing
        glfwSwapBuffers(window); //Swaps the colour buffer
        glfwPollEvents(); //Queries all GLFW events
    }

    //Safely terminates GLFW
    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    //Resizes window based on contemporary width & height values
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    //Initially no last positions, so sets last positions to current positions
    if (mouseFirstEntry)
    {
        cameraLastXPos = (float)xpos;
        cameraLastYPos = (float)ypos;
        mouseFirstEntry = false;
    }

    //Sets values for change in position since last frame to current frame
    float xOffset = (float)xpos - cameraLastXPos;
    float yOffset = cameraLastYPos - (float)ypos;

    //Sets last positions to current positions for next frame
    cameraLastXPos = (float)xpos;
    cameraLastYPos = (float)ypos;

    //Moderates the change in position based on sensitivity value
    const float sensitivity = 0.025f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    //Adjusts yaw & pitch values against changes in positions
    cameraYaw += xOffset;
    cameraPitch += yOffset;

    //Prevents turning up & down beyond 90 degrees to look backwards
    if (cameraPitch > 89.0f)
    {
        cameraPitch = 89.0f;
    }
    else if (cameraPitch < -89.0f)
    {
        cameraPitch = -89.0f;
    }

    //Modification of direction vector based on mouse turning
    vec3 direction;
    direction.x = cos(radians(cameraYaw)) * cos(radians(cameraPitch));
    direction.y = sin(radians(cameraPitch));
    direction.z = sin(radians(cameraYaw)) * cos(radians(cameraPitch));
    cameraFront = normalize(direction);
}

void ProcessUserInput(GLFWwindow* WindowIn)
{
    //Closes window on 'exit' key press
    if (glfwGetKey(WindowIn, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(WindowIn, true);
    }

    static bool aPressedLastFrame = false;
    static bool dPressedLastFrame = false;

    //Extent to which to move in one instance
    const float movementSpeed = 1.0f * deltaTime;
    //WASD controls
    if (glfwGetKey(WindowIn, GLFW_KEY_W) == GLFW_PRESS)
    {
        forwardSpeed += movementSpeed;
    }
    if (glfwGetKey(WindowIn, GLFW_KEY_S) == GLFW_PRESS)
    {
        forwardSpeed -= movementSpeed;
    }

    //different input style to prevent jittery lane swapping
    bool aPressed = glfwGetKey(WindowIn, GLFW_KEY_A) == GLFW_PRESS;
    bool dPressed = glfwGetKey(WindowIn, GLFW_KEY_D) == GLFW_PRESS;

    if (aPressed && !aPressedLastFrame && currentLane > -1)
    {
        currentLane--;
    }

    if (dPressed && !dPressedLastFrame && currentLane < 1)
    {
        currentLane++;
    }

    aPressedLastFrame = aPressed;
    dPressedLastFrame = dPressed;
}