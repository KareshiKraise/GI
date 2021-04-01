#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

enum class Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

//default values
const float YAW = 0.0f;
const float PITCH = 0.0f;
const float SENSITIVTY = 0.1f;
const float ZOOM = 60.0f;

struct camera
{
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	// Eular Angles
	float Yaw;
	float Pitch;
	// Camera options
	float MovementSpeed;
	float MouseSensitivity;	
	

	camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f), float yaw = YAW, float pitch = PITCH, float spd = 3.5f) : MovementSpeed(spd), MouseSensitivity(SENSITIVTY) {
		Position = position;
		Front = front;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}	


	//camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3 right = glm::vec3(-1.0f, 0.0f, 0.0f), float yaw = 180.0, float pitch = PITCH, float spd = 3.5f) : MovementSpeed(spd), MouseSensitivity(SENSITIVTY) {
	//	Position = position;
	//	Front = front;
	//	Right = right;
	//	WorldUp = up;
	//	Yaw = yaw;
	//	Pitch = pitch;
	//	updateCameraVectors();
	//}

	void updateCameraVectors()
	{		
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);		
		Right = glm::normalize(glm::cross(Front, WorldUp)); 
		Up = glm::normalize(glm::cross(Right, Front));
	}
		
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAtRH(Position, Position + Front, Up);
	}

	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		//float velocity = MovementSpeed * deltaTime;
		float velocity = MovementSpeed * deltaTime;
		if (direction == Camera_Movement::FORWARD)
			Position += Front * velocity;
		if (direction == Camera_Movement::BACKWARD)
			Position -= Front * velocity;
		if (direction == Camera_Movement::LEFT)
			Position -= Right * velocity;
		if (direction == Camera_Movement::RIGHT)
			Position += Right * velocity;
		updateCameraVectors();
	}

	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		// Update Front, Right and Up Vectors using the updated Eular angles
		updateCameraVectors();
	}
	
};


