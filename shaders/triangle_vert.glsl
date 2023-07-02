#version 330 core

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InUV;

uniform mat4 ModelMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ModelViewProjection;

out vec3 Position;
out vec3 Normal;
out vec2 UV;

void main()
{
    Position = vec3(ModelMatrix * vec4(InPosition, 1.0));
    Normal = vec3(NormalMatrix * vec4(InNormal, 0.0));
    UV = InUV;

    gl_Position = ModelViewProjection * vec4(InPosition, 1.0);
}