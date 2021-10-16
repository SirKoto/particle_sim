cmake_minimum_required(VERSION 3.10)

project(particle_sim)

# Find and build libraries

add_subdirectory(src)

add_subdirectory(libs)

# include resources to project
set(RESOURCES_PATH "resources")
set(PLY_PATH "${RESOURCES_PATH}/ply")
set(SHADER_PATH "${RESOURCES_PATH}/shaders")
set(SHADER_INCLUDE_PATH "${RESOURCES_PATH}/shader_includes")

target_include_directories(${PROJECT_NAME} PRIVATE ${SHADER_INCLUDE_PATH})

set(COPY_DATA
    ${PLY_PATH}/icosahedron.ply
    ${PLY_PATH}/sphere.ply

    ${SHADER_PATH}/simpl.vert   ${SHADER_PATH}/simpl.frag
    ${SHADER_PATH}/sphere.vert   ${SHADER_PATH}/sphere.frag
    ${SHADER_PATH}/mesh.vert   ${SHADER_PATH}/mesh.frag
    ${SHADER_PATH}/floor.vert   ${SHADER_PATH}/floor.frag
    ${SHADER_PATH}/spring_point.vert   ${SHADER_PATH}/spring_point.frag

    ${SHADER_PATH}/advect_particles.comp
    ${SHADER_PATH}/simple_spawner.comp
    ${SHADER_PATH}/advect_particles_springs.comp

    ${SHADER_INCLUDE_PATH}/particle_types.in
    ${SHADER_INCLUDE_PATH}/spring_types.in
    ${SHADER_INCLUDE_PATH}/intersections.comp.in
)

foreach(data ${COPY_DATA})
    configure_file(${data} ${data} COPYONLY)
endforeach()