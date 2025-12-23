#version 460
//Triangle position with values retrieved from main.cpp
layout (location = 0) in vec3 position;
//Texture coordinates from last stage
layout (location = 2) in vec2 textureVertex;

layout (location = 1) in vec3 aNormal;

//Model-View-Projection Matrix
uniform mat4 mvpIn;

//Texture to send
out vec2 textureFrag;

out vec3 FragPos;

void main()
{
    //Transformation applied to vertices
    gl_Position = mvpIn * vec4(position.x, position.y, position.z, 1.0);
    //Sending texture coordinates to next stage
    textureFrag = textureVertex;
    FragPos = vec3(mvpIn * vec4(position, 1.0));
}