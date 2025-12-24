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

#pragma region Setup parameters




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

//skybox
unsigned int skyboxVAO, skyboxVBO;


#pragma endregion

#pragma region game parameters


//Runner movement
float forwardSpeed = 20.0f; 
int currentLane = 0;          // -1 = left, 0 = centre, 1 = right
const float laneWidth = 1.5f;

//Vertical movement
float playerBaseY = 0.0f;
float playerY = 0.0f;
float verticalVelocity = 0.0f;

//states
bool isGrounded = true;
bool isCrouching = false;

//jump/crouch
const float jumpForce = 7.5f;
const float gravity = 18.0f;
const float crouchOffset = -0.6f;

//Distance tracking
float startZ = 0.0f;
float distanceTravelled = 0.0f;

//Difficulty Scaling
const float baseSpeed = 12.0f;
const float maxSpeed = 40.0f;

const float SPEED_INCREASE_PER_METRE = 0.02f;

const int baseObstacles = 2;
const int maxObstacles = 6;

int activeObstacleCount = baseObstacles;

//Obstacle 
enum ObstacleType
{
    Low, //Jump over
    Overhead, //Crouch under
    Normal //Switch lane
};

struct Obstacle
{
    vec3 position;
    int lane;        // -1, 0, 1
    int modelIndex;  // 0, 1, 2
    ObstacleType type;

    vec3 rotationAxis;
    float rotationAngle;

    vec3 scale;
};

const float obstacleSpawnDistance = 80.0f;
Obstacle obstacles[maxObstacles];

// Collision 
const float collisionZDistance = 1.5f;

// Lane tiles
const int NUM_LANES = 3;
const int TILES_PER_LANE = 8;

const float TILE_RECYCLE_BEHIND = 30.0f;
const float TILE_OVERLAP = 5.0f;
float tileZ[NUM_LANES][TILES_PER_LANE];
const float FIRST_TILE_OFFSET = 5.0f;

const float tileLength = 20.0f;
const float tileHeight = 1.0f;

//Walls
const float wallHeight = 3.0f;
const float wallThickness = 0.2f;
float wallZ[TILES_PER_LANE];
const int WALL_SEGMENTS_PER_TILE = 10;
const float wallSegmentLength = tileLength / WALL_SEGMENTS_PER_TILE;

// Lighting attached to walls
const int MAX_WALL_LIGHTS = 16;
const float WALL_LIGHT_SPACING = 8.0f; // minimum Z distance between lights

struct WallLight
{
    vec3 position;
    vec3 color;
    float intensity;
};

WallLight wallLights[MAX_WALL_LIGHTS];
int activeWallLights = 0;

//wall segments
struct WallSegment
{
    float z;
    int modelIndex; // 0 = A, 1 = B, 2 = C
};

WallSegment leftWalls[TILES_PER_LANE * WALL_SEGMENTS_PER_TILE];
WallSegment rightWalls[TILES_PER_LANE * WALL_SEGMENTS_PER_TILE];

//Wall positions
const float leftWallX = -(laneWidth * 1.55f) - (wallThickness);
const float rightWallX = (laneWidth * 1.8f) - (wallThickness);

//lighting/torches


// Fog settings
vec3 fogColor = vec3(0.1f, 0.0f, 0.2f);
float fogStart = 20.0f;
float fogEnd = 60.0f + forwardSpeed * 0.5f;


#pragma endregion


//Helper functions


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

void ResetTiles()
{
    //reset Tiles
    for (int lane = 0; lane < NUM_LANES; lane++)
    {
        // Place the first tile relative to the camera
        tileZ[lane][0] = cameraPosition.z - FIRST_TILE_OFFSET;

        // Chain the rest based on the previous tile
        for (int i = 1; i < TILES_PER_LANE; i++)
        {
            tileZ[lane][i] = tileZ[lane][i - 1] - tileLength + TILE_OVERLAP;
        }
    }
}

void ResetWalls() {
    int index = 0;

    for (int tile = 0; tile < TILES_PER_LANE; tile++)
    {
        float baseZ = tileZ[0][tile];

        for (int seg = 0; seg < WALL_SEGMENTS_PER_TILE; seg++)
        {
            float zPos = baseZ - (seg * wallSegmentLength);

            leftWalls[index] = {
                zPos,
                rand() % 3
            };

            rightWalls[index] = {
                zPos,
                rand() % 3
            };

            index++;
        }
    }
}


void RespawnObstacle(int index)
{
    obstacles[index].lane = (rand() % 3) - 1;
    obstacles[index].modelIndex = rand() % 4;

    obstacles[index].position.x = obstacles[index].lane * laneWidth;
    obstacles[index].position.y = -1.2f;

    switch (obstacles[index].modelIndex)
    {
    case 0:
        obstacles[index].type = Low;     
        break;

    case 1:
        obstacles[index].type = Overhead; 
        break;

    case 2:
        obstacles[index].type = Normal;   
        break;
    case 3:
        obstacles[index].type = Normal;   
        break;
    }

    switch (obstacles[index].type)
    {
    case Low:
        obstacles[index].rotationAxis = vec3(0.0f, 1.0f, 0.0f);
        obstacles[index].rotationAngle = 0.0f;
        obstacles[index].scale = vec3(2.0f, 2.0f, 2.0f);
        break;

    case Overhead:
        obstacles[index].rotationAxis = vec3(0.0f, 1.0f, 0.0f);
        obstacles[index].rotationAngle = 90.0f;
        obstacles[index].scale = vec3(2.0f, 3.0f, 2.0f);
        break;

    case Normal:
        obstacles[index].rotationAxis = vec3(0.0f, 1.0f, 0.0f);
        obstacles[index].rotationAngle = 0.0f;
        obstacles[index].scale = vec3(2.0f, 2.0f, 2.0f);
        break;
    }


    // Stagger obstacles so they don't overlap
    obstacles[index].position.z =
        cameraPosition.z - obstacleSpawnDistance - (index * 20.0f);
}

unsigned int LoadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(
            faces[i].c_str(), &width, &height, &nrChannels, 0);

       // std::cout << faces[i] << " : " << width << "x" << height << " channels=" << nrChannels << std::endl;

        if (data)
        {
            GLenum format = GL_RGB;
            if (nrChannels == 4) format = GL_RGBA;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 1) format = GL_RED;

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0,
                format, GL_UNSIGNED_BYTE, data
            );

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load cubemap face: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


int main()
{
#pragma region GLFW

    srand((unsigned int)time(nullptr));

    //Initialisation of GLFW
    glfwInit();
    //Initialisation of 'GLFWwindow' object
    windowWidth = 1280;
    windowHeight = 720;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Dungeon Escape", NULL, NULL);

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

    

#pragma endregion


#pragma region GLAD


    //Initialisation of GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "GLAD failed to initialise\n";
        return -1;
    }

#pragma endregion

#pragma region skybox setup
    float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices),
        &skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), (void*)0);



#pragma endregion

#pragma region Initial Setup



    //NEED THIS TO PROPERLY LOAD OBJECTS
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_CULL_FACE);

    //Loading of shaders
    Shader objectShader("shaders/vertexShader.vert", "shaders/fragmentShader.frag");
    Model Tile("media/tiles/castle/bridge-straight.obj");
    Model TorchModel("media/torch/wall torch.obj");
    Model WallA("media/tiles/castle/wall.obj");
    Model WallB("media/tiles/castle/wall-doorway.obj");
    Model WallC("media/tiles/castle/wall-narrow-wood.obj");
    Model ObstacleA("media/obstacles/arena/bricks.obj"); //Low
    Model ObstacleB("media/obstacles/castle/flag-wide.obj"); // Overhead
    Model ObstacleC("media/obstacles/arena/column-damaged.obj"); // Normal
    Model ObstacleD("media/obstacles/arena/statue.obj"); // Normal
    objectShader.use();

    //Skybox shaders
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

    std::vector<std::string> skyboxFaces =
    {
        "media/skybox/right.jpg",
        "media/skybox/left.jpg",
        "media/skybox/top.jpg",
        "media/skybox/bottom.jpg",
        "media/skybox/front.jpg",
        "media/skybox/back.jpg"
    };

    unsigned int cubemapTexture = LoadCubemap(skyboxFaces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

   // std::cout << "Skybox shader ID: " << skyboxShader.ID << std::endl;

    //Sets the viewport size within the window to match the window size of 1280x720
    glViewport(0, 0, 1280, 720);

    //Sets the framebuffer_size_callback() function as the callback for the window resizing event
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //Sets the mouse_callback() function as the callback for the mouse movement event
    glfwSetCursorPosCallback(window, mouse_callback);
    

    //Projection matrix
    mat4 projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
#pragma endregion

    //Distance tracking defaulkts
    startZ = cameraPosition.z;
    distanceTravelled = 0.0f;


    //inital spawns
    ResetTiles();
    ResetWalls();
    for (int i = 0; i < activeObstacleCount; i++)
    {
        RespawnObstacle(i);
    }

    //Render loop
    while (glfwWindowShouldClose(window) == false)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //Time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //gravity
        verticalVelocity -= gravity * deltaTime;
        playerY += verticalVelocity * deltaTime;

        //ground collision
        if (playerY <= playerBaseY) {
            playerY = playerBaseY;
            verticalVelocity = 0.0f;
            isGrounded = true;
        }

        // Automatic forward movement 
        cameraPosition.z -= forwardSpeed * deltaTime;

        //toggle crouch
        cameraPosition.y = playerY + (isCrouching ? crouchOffset : 0.0f);

        // Snap camera X position to current lane
        cameraPosition.x = currentLane * laneWidth;


        //track distance travvelled
        distanceTravelled = startZ - cameraPosition.z;

        // Scale speed with distance
        forwardSpeed = baseSpeed + distanceTravelled * SPEED_INCREASE_PER_METRE;
        forwardSpeed = glm::clamp(forwardSpeed, baseSpeed, maxSpeed);

        //scale obstacles with distance
        const float DISTANCE_PER_OBSTACLE = 60.0f;

        int desiredObstacleCount =
            baseObstacles + static_cast<int>(distanceTravelled / DISTANCE_PER_OBSTACLE);

        activeObstacleCount = glm::clamp(
            desiredObstacleCount,
            baseObstacles,
            maxObstacles
        );


        // Collision check
        for (int i = 0; i < activeObstacleCount; i++)
        {
            bool sameLane = (currentLane == obstacles[i].lane);
            bool closeEnoughZ =
                abs(obstacles[i].position.z - cameraPosition.z) < collisionZDistance;

            bool collision = false;

            //vertical clearance
            if (sameLane && closeEnoughZ)
            {
                switch (obstacles[i].type)
                {
                case Low:
                    // Must jump
                    collision = (playerY < 1.0f);
                    break;

                case Overhead:
                    // Must crouch
                    collision = (!isCrouching);
                    break;

                case Normal:
                    // Always collide
                    collision = true;
                    break;
                }
            }

            if (collision) {
                // Reset player
                cameraPosition.z = 0.0f;
                currentLane = 0;
                forwardSpeed = baseSpeed;
                startZ = cameraPosition.z;
                distanceTravelled = 0.0f;

                //Reset obstacles
                activeObstacleCount = baseObstacles;
                RespawnObstacle(i);

                //Reset world
                ResetTiles();
                ResetWalls();
                //ResetTorches();


                std::cout << "Collision!" << std::endl;
            }
        }
        

        //Input
        ProcessUserInput(window); //Takes user input

        //UI
        static float titleUpdateTimer = 0.0f;
        titleUpdateTimer += deltaTime;

        if (titleUpdateTimer > 0.25f)
        {
            std::string title = "Dungeon Escape | Distance: " +
                std::to_string((int)distanceTravelled) + " m";

            glfwSetWindowTitle(window, title.c_str());
            titleUpdateTimer = 0.0f;
        }

        //Rendering
        glClearColor(0.25f, 0.0f, 1.0f, 1.0f); //Colour to display on cleared window
        glClear(GL_COLOR_BUFFER_BIT); //Clears the colour buffer
        glClear(GL_DEPTH_BUFFER_BIT); //Clears the depth buffer

        glEnable(GL_CULL_FACE); //Discards all back-facing triangles


        //Transformations
        mat4 view = lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp); //Sets the position of the viewer, the movement direction in relation to it & the world up direction
        mat4 model = mat4(1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        objectShader.use();
        objectShader.setVec3("viewPos", cameraPosition);
        objectShader.setMat4("view", view);
        objectShader.setMat4("projection", projection);

        objectShader.setVec3("fogColor", fogColor);
        objectShader.setFloat("fogStart", fogStart);
        objectShader.setFloat("fogEnd", fogEnd);


        objectShader.setInt("numLights", activeWallLights);

        for (int i = 0; i < activeWallLights; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "]";
            objectShader.setVec3(base + ".position", wallLights[i].position);
            objectShader.setVec3(base + ".color", wallLights[i].color);
            objectShader.setFloat(base + ".intensity", wallLights[i].intensity);
        }
        

        //Recycle obstacle positions individually
        for (int i = 0; i < activeObstacleCount; i++)
        {
            if (obstacles[i].position.z > cameraPosition.z + TILE_RECYCLE_BEHIND)
            {
                RespawnObstacle(i);
            }
        }
        
        //recycle tile positions
        for (int lane = 0; lane < NUM_LANES; lane++)
        {
            for (int i = 0; i < TILES_PER_LANE; i++)
            {
                if (tileZ[lane][i] > cameraPosition.z + TILE_RECYCLE_BEHIND)
                {
                    float furthestZ = GetFurthestTileZ(lane);
                    tileZ[lane][i] = furthestZ - (tileLength - TILE_OVERLAP);
                    //std::cout << "tile pos:" <<tileZ[lane][i] <<  std::endl;
                }
            }
        }

        //recycle wall positions
        for (int i = 0; i < TILES_PER_LANE * WALL_SEGMENTS_PER_TILE; i++)
        {
            if (leftWalls[i].z > cameraPosition.z + TILE_RECYCLE_BEHIND)
            {
                // Find furthest wall segment
                float furthestZ = leftWalls[0].z;
                for (int j = 1; j < TILES_PER_LANE * WALL_SEGMENTS_PER_TILE; j++)
                    furthestZ = std::min(furthestZ, leftWalls[j].z);

                leftWalls[i].z = furthestZ - wallSegmentLength;
                rightWalls[i].z = furthestZ - wallSegmentLength;

                leftWalls[i].modelIndex = rand() % 3;
                rightWalls[i].modelIndex = rand() % 3;
            }
        }
        
      
        activeWallLights = 0;
        float lastLightZ = 99999.0f;

        // LEFT WALLS
        for (int i = 0; i < TILES_PER_LANE * WALL_SEGMENTS_PER_TILE; i++)
        {
            if (leftWalls[i].modelIndex == 0)
            {
                float z = leftWalls[i].z;

                // spacing + limit
                if (abs(z - lastLightZ) > WALL_LIGHT_SPACING &&
                    activeWallLights < MAX_WALL_LIGHTS)
                {
                    wallLights[activeWallLights++] =
                    {
                        vec3(leftWallX + 0.5f, 0.0f, z),
                        vec3(1.0f, 0.7f, 0.3f), // warm torch colour
                        1.3f
                    };

                    lastLightZ = z;
                }
            }
        }

        // RIGHT WALLS
        for (int i = 0; i < TILES_PER_LANE * WALL_SEGMENTS_PER_TILE; i++)
        {
            if (rightWalls[i].modelIndex == 0)
            {
                float z = rightWalls[i].z;

                if (abs(z - lastLightZ) > WALL_LIGHT_SPACING &&
                    activeWallLights < MAX_WALL_LIGHTS)
                {
                    wallLights[activeWallLights++] =
                    {
                        vec3(rightWallX - 0.5f, 0.0f, z),
                        vec3(1.0f, 0.7f, 0.3f),
                        1.3f
                    };

                    lastLightZ = z;
                }
            }
        }


        //Drawing Tiles
        for (int lane = 0; lane < NUM_LANES; lane++)
        {
            float laneX = (lane - 1) * laneWidth; // -1, 0, +1

            for (int i = 0; i < TILES_PER_LANE; i++)
            {

                mat4 tileModel = mat4(1.0f);

                mat4 T = translate(mat4(1.0f), vec3(laneX, -2.5f, tileZ[lane][i]));

                mat4 R = mat4(1.0f);

                 R = rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 1.0f, 0.0f));

                mat4 S = scale(mat4(1.0f), vec3(tileLength, tileHeight, laneWidth * 0.9f));

                // World-space translation 
                tileModel = T * R * S;

                mat4 tileMVP = projection * view * tileModel;
                objectShader.setMat4("mvpIn", tileMVP);
                objectShader.setMat4("model", tileModel);

                Tile.Draw(objectShader);
            }
        }

        //Drawing Walls
        for (int i = 0; i < TILES_PER_LANE * WALL_SEGMENTS_PER_TILE; i++)
        {
            Model* wallModel = nullptr;

            // LEFT WALL
            switch (leftWalls[i].modelIndex)
            {
            case 0: wallModel = &WallA; break;
            case 1: wallModel = &WallB; break;
            case 2: wallModel = &WallC; break;
            }

            mat4 T = translate(mat4(1.0f),
                vec3(leftWallX, -wallHeight * 0.6f, leftWalls[i].z));
            mat4 S = scale(mat4(1.0f),
                vec3(1.0f, wallHeight, wallSegmentLength));

            mat4 model = T * S;
            objectShader.setMat4("mvpIn", projection * view * model);
            objectShader.setMat4("model", model);
            wallModel->Draw(objectShader);

            // RIGHT WALL 
            switch (rightWalls[i].modelIndex)
            {
            case 0: wallModel = &WallA; break;
            case 1: wallModel = &WallB; break;
            case 2: wallModel = &WallC; break;
            }

            T = translate(mat4(1.0f),
                vec3(rightWallX, -wallHeight * 0.6f, rightWalls[i].z));
            mat4 R = mat4(1.0f);
            R = rotate(mat4(1.0f), radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
            model = T * R * S;
            objectShader.setMat4("mvpIn", projection * view * model);
            objectShader.setMat4("model", model);
            wallModel->Draw(objectShader);
        }



        //Drawing Obstacles
        for (int i = 0; i < activeObstacleCount; i++)
        {
            Model* obstacleModel = nullptr;

            switch (obstacles[i].modelIndex)
            {
            case 0: obstacleModel = &ObstacleA; break;
            case 1: obstacleModel = &ObstacleB; break;
            case 2: obstacleModel = &ObstacleC; break;
            case 3: obstacleModel = &ObstacleD; break;
            }

            mat4 T = translate(mat4(1.0f), obstacles[i].position);
            mat4 R = rotate(mat4(1.0f),
                radians(obstacles[i].rotationAngle),
                obstacles[i].rotationAxis);
            mat4 S = scale(mat4(1.0f), obstacles[i].scale);
            mat4 model = T * R * S;

            objectShader.setMat4("mvpIn", projection * view * model);
            objectShader.setMat4("model", model);
            obstacleModel->Draw(objectShader);
        }


        //Drawing Skybox
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);    
        glDisable(GL_CULL_FACE);

        skyboxShader.use();

        // Remove translation from view matrix
        mat4 skyView = mat4(mat3(view));
        skyboxShader.setMat4("view", skyView);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);

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

    static bool wPressedLastFrame = false;
    static bool aPressedLastFrame = false;
    static bool dPressedLastFrame = false;

    //different input style to prevent jitter
    bool wPressed = glfwGetKey(WindowIn, GLFW_KEY_W) == GLFW_PRESS;
    bool aPressed = glfwGetKey(WindowIn, GLFW_KEY_A) == GLFW_PRESS;
    bool dPressed = glfwGetKey(WindowIn, GLFW_KEY_D) == GLFW_PRESS;



    //WASD controls

    if (wPressed && !wPressedLastFrame && isGrounded)
    {
        verticalVelocity = jumpForce;
        isGrounded = false;
    }
   
    if (glfwGetKey(WindowIn, GLFW_KEY_S) == GLFW_PRESS)
    {
        isCrouching = true;
    }
    else {
        isCrouching = false;
    }


    if (aPressed && !aPressedLastFrame && currentLane > -1)
    {
        currentLane--;
    }

    if (dPressed && !dPressedLastFrame && currentLane < 1)
    {
        currentLane++;
    }

    wPressedLastFrame = wPressed;
    aPressedLastFrame = aPressed;
    dPressedLastFrame = dPressed;


}