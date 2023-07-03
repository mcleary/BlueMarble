#version 330 core

in vec3 Color;

out vec4 OutColor;

void main()
{
    OutColor = vec4(Color, 1.0);
}