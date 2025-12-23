#version 460
//Colour value to send to next stage
out vec4 FragColor;

//Texture coordinates from last stage
in vec2 textureFrag;

uniform sampler2D texture_diffuse1;

uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
in vec3 FragPos;

void main()
{
    //Setting of colour coordinates to colour map
    FragColor = texture(texture_diffuse1, textureFrag);

    float distance = length(FragPos);

    // Linear fog factor
    float fogFactor = clamp(
        (fogEnd - distance) / (fogEnd - fogStart),
        0.0,
        1.0
    );

    // Mix fog with original color
    vec3 finalColor = mix(fogColor, FragColor.rgb, fogFactor);

    FragColor = vec4(finalColor, FragColor.a);
}