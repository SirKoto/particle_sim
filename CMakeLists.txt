cmake_minimum_required(VERSION 3.10)

project(particle_sim)

# Find and build libraries

add_subdirectory(src)

add_subdirectory(libs)


set(RESOURCES_PATH "resources")
set(PLY_PATH "${RESOURCES_PATH}/ply")
set(SHADER_PATH "${RESOURCES_PATH}/shaders")

set(COPY_DATA
    ${PLY_PATH}/icosahedron.ply

    ${SHADER_PATH}/simpl.vert   ${SHADER_PATH}/simpl.frag
)

foreach(data ${COPY_DATA})
    configure_file(${data} ${data} COPYONLY)
endforeach()