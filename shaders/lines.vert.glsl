#version 330 core

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InColor;

out vec3 Color;

uniform mat4 ModelViewProjection;

void main()
{
    Color = InColor;
    gl_Position = ModelViewProjection * vec4(InPosition, 1.0);
}