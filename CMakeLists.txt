cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
project(Chip8)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
file(GLOB_RECURSE SRCFILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cpp)
include_directories()

add_executable(CHIP8 ${SRCFILES})
target_include_directories(CHIP8 PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(CHIP8 ${SDL2_LIBRARIES})