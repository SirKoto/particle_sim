cmake_minimum_required(VERSION 3.10)

add_executable(${PROJECT_NAME}
	main.cpp
	Camera.cpp	Camera.hpp
	GlobalContext.cpp	GlobalContext.hpp

	graphics/TriangleMesh.cpp	graphics/TriangleMesh.hpp
	graphics/Shader.cpp	graphics/Shader.hpp
	graphics/ShaderProgram.cpp graphics/ShaderProgram.hpp
	graphics/my_gl_header.hpp


	particle_system/ParticleSystem.cpp	particle_system/ParticleSystem.hpp
	particle_system/SpringSystem.cpp	particle_system/SpringSystem.hpp
	particle_system/ClothSystem.cpp	particle_system/ClothSystem.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE "./")

target_link_libraries(${PROJECT_NAME} PRIVATE
	tinyply glfw glad ImGui glm ${CMAKE_DL_LIBS}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_compile_definitions(${PROJECT_NAME} PUBLIC PROJECT_DIR="${CMAKE_BINARY_DIR}")