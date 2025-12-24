#version 460
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 textureVertex;

//Model-View-Projection Matrix
uniform mat4 mvpIn;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

//Texture to send
out vec2 textureFrag;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);

    FragPos = worldPos.xyz; // ? world space
    Normal = mat3(transpose(inverse(model))) * aNormal;
    textureFrag = textureVertex;

    gl_Position = projection * view * worldPos;
}