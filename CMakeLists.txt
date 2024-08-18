cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(memDump)

include_directories(include)

set(IMGUI_DIR external/imgui)

file(GLOB IMGUI_SOURCE ${IMGUI_DIR}/*.cpp)

list(APPEND IMGUI_SOURCE 
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCE})

target_include_directories(
    imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

set(SOURCE_FILES
    src/main.cpp
    src/memDump.cpp
)

add_definitions(-DUSE_GUI)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} d3d11 imgui)
