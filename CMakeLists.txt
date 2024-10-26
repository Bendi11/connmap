cmake_minimum_required(VERSION 3.30)

project(
    connmap
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_library(MNL mnl REQUIRED)

file(
    GLOB
    SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(${CMAKE_PROJECT_NAME} ${SRC})

target_link_libraries(${CMAKE_PROJECT_NAME} ${MNL})
