#version 460

#define maxTorchLights 8

struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
};

uniform int numLights;
uniform PointLight lights[maxTorchLights];
uniform vec3 viewPos;

//Colour value to send to next stage
out vec4 FragColor;

//Texture coordinates from last stage
in vec2 textureFrag;

uniform sampler2D texture_diffuse1;

uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;

in vec3 FragPos;
in vec3 Normal;

void main()
{
    vec3 texColor = texture(texture_diffuse1, textureFrag).rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 lighting = vec3(0.0);

    for (int i = 0; i < numLights; i++)
    {
        vec3 lightDir = normalize(lights[i].position - FragPos);

        // Ambient
        vec3 ambient = 0.08 * texColor;

        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * texColor;

        // Blinn–Phong specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
        vec3 specular = spec * lights[i].color;

        float dist = length(lights[i].position - FragPos);
        float radius = 8.0; 
        float attenuation = clamp(1.0 - dist / radius, 0.0, 1.0);
        attenuation *= attenuation; 

        lighting += (ambient + diffuse + specular)
                    * lights[i].intensity
                    * attenuation;
    }

    //Fog
    float distance = length(FragPos - viewPos);
    float fogFactor = clamp(
        (fogEnd - distance) / (fogEnd - fogStart),
        0.0,
        1.0
    );

    vec3 finalColor = mix(fogColor, lighting, fogFactor);

    FragColor = vec4(finalColor, 1.0);

}