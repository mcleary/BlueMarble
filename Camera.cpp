
#include "Camera.h"

#include <glm/ext.hpp>

FSimpleCamera::FSimpleCamera()
{
    Reset();
}

void FSimpleCamera::MoveForward(float Scale)
{
	ForwardScale = Scale;
}

void FSimpleCamera::MoveRight(float Scale)
{
	RightScale = Scale;
}

void FSimpleCamera::MouseMove(float X, float Y)
{
	if (bEnableMouseMovement)
	{
		glm::vec2 CurrentCursor{ X, Y };
		glm::vec2 Delta = (CurrentCursor - PreviousCursor) / 10.0f;

		if (glm::length(Delta) < 5.0f)
		{
			glm::vec3 Right = glm::cross(Direction, Up);

			glm::mat4 RotationRight = glm::rotate(glm::identity<glm::mat4>(), glm::radians(-Delta.y), Right);
			glm::mat4 RotationUp = glm::rotate(glm::identity<glm::mat4>(), glm::radians(-Delta.x), Up);
			glm::mat4 Rotation = RotationRight * RotationUp;

			Up = Rotation * glm::vec4{ Up, 0.0f };
			Direction = Rotation * glm::vec4{ Direction, 0.0f };
		}

		PreviousCursor = CurrentCursor;
	}
}

void FSimpleCamera::Update(float DeltaTime)
{
	glm::vec3 Right = glm::cross(Direction, Up);

	Location += Direction * ForwardScale * DeltaTime;
	Location += Right * RightScale * DeltaTime;
}

void FSimpleCamera::Reset()
{
    Location = { 6.0f, 3.0f, 6.0f };
    // Direction = { 0.0f, 0.0f, -1.0f };
    Direction = glm::normalize(-Location);
    Up = { 0.0f, 1.0f, 0.0f };
}

void FSimpleCamera::SetViewportSize(std::int32_t Width, std::int32_t Height)
{
    AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

    Left = 0.0f;
    Right = 1.0f;
    Bottom = 0.0f;
    Top = 1.0f;
}

glm::mat4 FSimpleCamera::GetView()
{
	return glm::lookAt(Location, Location + Direction, Up);
}

glm::mat4 FSimpleCamera::GetProjection()
{
	glm::mat4 Projection;

    if (bIsOrtho)
    {
        Projection = glm::ortho(Left, Right, Bottom, Top, Near, Far);
    }
    else
    {
        Projection = glm::perspective(glm::radians(FieldOfView), AspectRatio, Near, Far);
    }

	return Projection;
}
