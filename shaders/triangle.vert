#version 330 core

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InUV;

uniform mat4 ModelMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ModelViewProjection;

out VertexData
{
    vec3 Position;
    vec3 Normal;
    vec2 UV;
} Out;


void main()
{
    Out.Position = vec3(ModelMatrix * vec4(InPosition, 1.0));
    Out.Normal = vec3(NormalMatrix * vec4(InNormal, 0.0));
    Out.UV = InUV;

    gl_Position = ModelViewProjection * vec4(InPosition, 1.0);
}