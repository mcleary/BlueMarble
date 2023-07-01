
#include "Camera.h"

#include <glm/ext.hpp>

void SimpleCamera::MoveForward(float Scale)
{
	ForwardScale = Scale;
}

void SimpleCamera::MoveRight(float Scale)
{
	RightScale = Scale;
}

void SimpleCamera::MouseMove(float X, float Y)
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

void SimpleCamera::Update(float DeltaTime)
{
	glm::vec3 Right = glm::cross(Direction, Up);

	Location += Direction * ForwardScale * DeltaTime;
	Location += Right * RightScale * DeltaTime;
}

void SimpleCamera::SetViewportSize(std::int32_t Width, std::int32_t Height)
{
    AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

    Left = 0.0f;
    Right = static_cast<float>(Width);
    Bottom = 0.0f;
    Top = static_cast<float>(Height);
}

glm::mat4 SimpleCamera::GetView()
{
	return glm::lookAt(Location, Location + Direction, Up);
}

glm::mat4 SimpleCamera::GetViewProjection()
{
	glm::mat4 View = GetView();
	glm::mat4 Projection;

    if (bIsOrtho)
    {
        Projection = glm::ortho(Left, Right, Bottom, Top, Near, Far);
    }
    else
    {
        Projection = glm::perspective(FieldOfView, AspectRatio, Near, Far);
    }

	return Projection * View;
}
