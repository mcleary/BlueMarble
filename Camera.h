#pragma once

#include <glm/glm.hpp>

class SimpleCamera
{
public:
	void MoveForward(float Scale);
	void MoveRight(float Scale);
	void MouseMove(float X, float Y);
	void Update(float DeltaTime);
    void SetViewportSize(std::int32_t Width, std::int32_t Height);
	glm::mat4 GetView();
	glm::mat4 GetViewProjection();

	bool bEnableMouseMovement = false;
	glm::vec2 PreviousCursor{ 0.0f };
	float ForwardScale = 0.0f;
	float RightScale = 0.0f;

	glm::vec3 Location = { 0.0f, 0.0f, 5.0f };
	glm::vec3 Direction = { 0.0f, 0.0f, -1.0f };
	glm::vec3 Up = { 0.0f, 1.0f, 0.0f };

	float FieldOfView = glm::radians(45.0f);
	float AspectRatio = 4.0f / 3.0f;
	float Near = 0.01f;
	float Far = 1000.0f;
};