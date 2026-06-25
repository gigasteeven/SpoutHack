cmake_minimum_required(VERSION 3.20)

project(dual_layout VERSION 1.0.0)

find_package(Geode REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/geode)

include(FetchContent)
FetchContent_Declare(
    Spout2
    GIT_REPOSITORY https://github.com/leadedge/Spout2.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Spout2)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.hpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${spout2_SOURCE_DIR}/SPOUTSDK/SpoutGL)
target_link_libraries(${PROJECT_NAME} PRIVATE SpoutLibrary)

setup_geode_mod(${PROJECT_NAME})
