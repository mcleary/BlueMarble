#pragma once

#include <glm/glm.hpp>

class SimpleCamera
{
public:
    SimpleCamera();

	void MoveForward(float Scale);
	void MoveRight(float Scale);
	void MouseMove(float X, float Y);
	void Update(float DeltaTime);
    void Reset();
    void SetViewportSize(std::int32_t Width, std::int32_t Height);
	glm::mat4 GetView();
	glm::mat4 GetViewProjection();

	bool bEnableMouseMovement = false;
	glm::vec2 PreviousCursor{ 0.0f };
	float ForwardScale = 0.0f;
	float RightScale = 0.0f;

	glm::vec3 Location;
	glm::vec3 Direction;
	glm::vec3 Up;

    bool bIsOrtho = false;

	float FieldOfView = glm::radians(45.0f);
	float AspectRatio = 1.0f;

    float Left = 0.0f;
    float Right = 1280.0f;
    float Bottom = 0.0f;
    float Top = 720.0f;

    float Near = 0.01f;
    float Far = 1000.0f;
};