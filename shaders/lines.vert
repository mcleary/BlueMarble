#version 330 core

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InColor;

out vec3 Color;

layout (std140) uniform FrameUBO
{
    mat4 View;
    mat4 Projection;
    float Time;
};

layout (std140) uniform ModelUBO
{
    mat4 Model;
    mat4 Normal;
};

void main()
{
    Color = InColor;
    gl_Position = Projection * View * Model * vec4(InPosition, 1.0);
}