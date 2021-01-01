cmake_minimum_required(VERSION 3.17)
project(Vulkan)

set(CMAKE_CXX_STANDARD 20)

if(${CMAKE_BUILD_TYPE} EQUAL Debug)
    add_compile_definitions(_DEBUG)
endif()

include_directories(src)
include_directories(src/imgui)
include_directories(src/vulkan)
include_directories(include)
include_directories(D:/VulkanSDK/1.2.162.0/Include)
link_directories(lib)
link_directories(D:/VulkanSDK/1.2.162.0/Lib)

add_executable(Vulkan
        src/imgui/imconfig.h
        src/imgui/imgui.cpp
        src/imgui/imgui.h
        src/imgui/imgui_demo.cpp
        src/imgui/imgui_draw.cpp
        src/imgui/imgui_impl_sdl.cpp
        src/imgui/imgui_impl_sdl.h
        src/imgui/imgui_impl_vulkan.cpp
        src/imgui/imgui_impl_vulkan.h
        src/imgui/imgui_internal.h
        src/imgui/imgui_stdlib.cpp
        src/imgui/imgui_stdlib.h
        src/imgui/imgui_widgets.cpp
        src/imgui/imstb_rectpack.h
        src/imgui/imstb_textedit.h
        src/imgui/imstb_truetype.h
        src/vulkan/UniformBufferObject.h
        src/vulkan/Vertex.h
        src/vulkan/VulkanApplication.cpp
        src/vulkan/VulkanApplication.h
        src/vulkan/VulkanFrame.cpp
        src/vulkan/VulkanFrame.h
        src/vulkan/VulkanHelper.cpp
        src/vulkan/VulkanHelper.h
        src/Camera.cpp
        src/Camera.h
        src/Debug.h
        src/imgui_custom.cpp
        src/imgui_custom.h
        src/Input.cpp
        src/Input.h
        src/main.cpp)
target_link_libraries(Vulkan mingw32 SDL2main SDL2 SDL2_image vulkan-1)
