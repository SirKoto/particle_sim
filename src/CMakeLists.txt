cmake_minimum_required(VERSION 3.10)

add_executable(${PROJECT_NAME}
	main.cpp
	Camera.cpp	Camera.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	glfw glad ImGui glm ${CMAKE_DL_LIBS}
)
