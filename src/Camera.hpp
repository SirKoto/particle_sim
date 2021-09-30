#pragma once

#include <glm/glm.hpp>

// Simple camera. Mostly extracted from LearnOpenGL
class Camera {
public:
	Camera();

	// Get input and update the camera state
	void update();

	// Get the Projection matrix mutliplied by the view matrix
	glm::mat4 getProjView() const;

	// Render some information into the UI
	void renderImGui();

private:
	glm::vec3 mPosition;
	glm::vec3 mFront;
	glm::vec3 mUp;
	glm::vec3 mRight;
	float mYaw, mPitch;

	float mMouseSensitivity;
	float mSpeed;
	float mZoom;

	void updateCameraVectors();
};