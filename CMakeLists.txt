cmake_minimum_required(VERSION 3.10)

project(particle_sim)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# Find and build libraries

add_subdirectory(src)

add_subdirectory(libs)


set(COPY_DATA
    
)

foreach(data ${COPY_DATA})
    configure_file(${data} ${data} COPYONLY)
endforeach()