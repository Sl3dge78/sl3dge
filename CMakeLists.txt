cmake_minimum_required(VERSION 3.15)
project(Vulkan CXX)

set(CMAKE_CXX_STANDARD 20)

if (${CMAKE_BUILD_TYPE} EQUAL Debug)
    add_compile_definitions(_DEBUG)
endif ()

include_directories(include)
include_directories(D:/VulkanSDK/1.2.162.0/Include)
link_directories(lib)
link_directories(D:/VulkanSDK/1.2.162.0/Lib)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

set(INCLUDE_DIRS "")
foreach (_headerFile ${HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach ()
list(REMOVE_DUPLICATES INCLUDE_DIRS)
include_directories(INCLUDE_DIRS)
#set (SOURCES
#       src/imgui/imgui.cpp
#       src/imgui/imgui_demo.cpp
#       src/imgui/imgui_draw.cpp
#       src/imgui/imgui_impl_sdl.cpp
#       src/imgui/imgui_impl_vulkan.cpp
#       src/imgui/imgui_stdlib.cpp
#       src/imgui/imgui_widgets.cpp
#       src/main.cpp
#       src/Camera.cpp
#       src/Input.cpp
#       src/vulkan/VulkanApplication.cpp
#       src/vulkan/Swapchain.cpp
#       src/vulkan/VulkanFrame.cpp
#       src/vulkan/VulkanHelper.cpp
#       )
add_executable(Vulkan ${SOURCES} ${HEADERS})
target_include_directories(Vulkan PRIVATE ${INCLUDE_DIRS})
target_link_libraries(Vulkan mingw32 SDL2main SDL2 SDL2_image vulkan-1)
