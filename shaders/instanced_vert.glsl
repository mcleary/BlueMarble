#version 330 core

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InUV;
layout(location = 3) in mat4 InModelMatrix;

uniform float Time;
uniform int NumInstances;

uniform mat4 ViewProjection;

out vec3 Position;
out vec3 Normal;
out vec2 UV;

// https://github.com/dmnsgn/glsl-rotate/blob/main/rotation-3d.glsl
mat4 Rotation3D(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(
        oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
        oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
        oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
        0.0,                                0.0,                                0.0,                                1.0
    );
}

void main()
{
    float Speed = ( float(gl_InstanceID) / float(NumInstances)) * 0.01f;
    float Angle = Time * Speed;
    mat4 RotationMatrix = Rotation3D(vec3(0.0, 1.0, 0.0), Angle);

    Position = vec3(InModelMatrix * vec4(InPosition, 1.0));
    Normal = vec3(inverse(transpose(InModelMatrix)) * vec4(InNormal, 0.0));
    UV = InUV;

    gl_Position = ViewProjection * RotationMatrix * InModelMatrix * vec4(InPosition, 1.0);
}