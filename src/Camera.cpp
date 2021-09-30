#include "Camera.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>


Camera::Camera() : mPosition(0,0,5), mFront(0,0,-1), mUp(0,1,0), mYaw(-90.f),mPitch(0), mMouseSensitivity(0.1f), mSpeed(2.5f), mZoom(45.f)
{
	updateCameraVectors();
}

void Camera::update()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		return;
	}

	// movement
	if (io.MouseDown[ImGuiMouseButton_Right]) {

		// Keyboard
		if (!io.WantCaptureKeyboard) {
			float speed = mSpeed * io.DeltaTime;
			if (io.KeysDown[GLFW_KEY_W]) {
				mPosition += mFront * speed;
			}
			if (io.KeysDown[GLFW_KEY_S]) {
				mPosition -= mFront * speed;
			}
			if (io.KeysDown[GLFW_KEY_D]) {
				mPosition += mRight * speed;
			}
			if (io.KeysDown[GLFW_KEY_A]) {
				mPosition -= mRight * speed;
			}
		}

		// Mouse
		mYaw += mMouseSensitivity * io.MouseDelta.x;
		mPitch -= mMouseSensitivity * io.MouseDelta.y;

		mPitch = glm::clamp(mPitch, -89.0f, 89.0f);

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}


	// zoom
	mZoom -= io.MouseWheel;
	mZoom = glm::clamp(mZoom, 1.0f, 45.0f);
}

glm::mat4 Camera::getProjView() const
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.DisplaySize.y == 0 || io.DisplaySize.x == 0) {
		return glm::mat4(1.0f);
	}
	return glm::perspective(glm::radians(mZoom), io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.0f)  *
		glm::lookAt(mPosition, mPosition + mFront, mUp);
}

void Camera::renderImGui()
{
	ImGui::Text("Position (%.3f, %.3f, %.3f)", mPosition.x, mPosition.y, mPosition.z);
	ImGui::Text("Front (%.3f, %.3f, %.3f)", mFront.x, mFront.y, mFront.z);
	ImGui::Text("Up (%.3f, %.3f, %.3f)", mUp.x, mUp.y, mUp.z);
	ImGui::Text("Right (%.3f, %.3f, %.3f)", mRight.x, mRight.y, mRight.z);
	ImGui::Text("Yaw %.f", mYaw);
	ImGui::Text("Pitch %.f", mPitch);
	ImGui::Text("Zoom %.f", mZoom);

}

void Camera::updateCameraVectors()
{
	glm::vec3 front;
	front.x = std::cos(glm::radians(mYaw)) * std::cos(glm::radians(mPitch));
	front.y = std::sin(glm::radians(mPitch));
	front.z = std::sin(glm::radians(mYaw)) * std::cos(glm::radians(mPitch));
	mFront = glm::normalize(front);
	// also re-calculate the Right and Up vector
	mRight = glm::normalize(glm::cross(mFront, glm::vec3(0,1,0)));
	mUp = glm::normalize(glm::cross(mRight, mFront));
}
