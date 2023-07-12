#version 330 core

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InUV;

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

out VertexData
{
    vec3 Position;
    vec3 Normal;
    vec2 UV;
} Out;

void main()
{
    Out.Position = vec3(Model * vec4(InPosition, 1.0));
    Out.Normal = vec3(Normal * vec4(InNormal, 0.0));
    Out.UV = InUV;

    gl_Position = Projection * View * Model * vec4(InPosition, 1.0);
}