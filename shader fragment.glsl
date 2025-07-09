#version 330 core

in vec2 TexCoord; 

out vec4 FragColor;

uniform sampler2D modelTexture;
uniform int renderMode;

void main()
{
    if (renderMode == 1)
        FragColor = texture(modelTexture, TexCoord);
    else if (renderMode == 2)
        FragColor = vec4(0.3, 0.3, 0.3, 1.0);
    else if (renderMode == 3)
        FragColor = vec4(0.75, 0.75, 0.75, 1.0);
}