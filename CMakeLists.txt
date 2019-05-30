cmake_minimum_required(VERSION 3.10)
project(vienna_vulkan_engine_cmake)

set(CMAKE_CXX_STANDARD 14)

add_executable(game
        main.cpp
        VulkanEngine/VEEngine.h
        VulkanEngine/VEEngine.cpp
        VulkanEngine/VEEntity.h
        VulkanEngine/VEEntity.cpp
        VulkanEngine/VEEventListenerGLFW.h
        VulkanEngine/VEEventListenerGLFW.cpp
        VulkanEngine/VEEventListener.h
        VulkanEngine/VEEventListener.cpp
        VulkanEngine/VEInclude.h
        VulkanEngine/VENamedClass.h
        VulkanEngine/VENamedClass.cpp
        VulkanEngine/VERenderer.h
        VulkanEngine/VERenderer.cpp
        VulkanEngine/VERendererForward.h
        VulkanEngine/VERendererForward.cpp
        VulkanEngine/VESceneManager.h
        VulkanEngine/VESceneManager.cpp
        VulkanEngine/VESubrender.h
        VulkanEngine/VESubrender.cpp
        VulkanEngine/VESubrenderFW_C1.h
        VulkanEngine/VESubrenderFW_C1.cpp
        VulkanEngine/VESubrenderFW_Cubemap.h
        VulkanEngine/VESubrenderFW_Cubemap.cpp
        VulkanEngine/VESubrenderFW_D.h
        VulkanEngine/VESubrenderFW_D.cpp
        VulkanEngine/VESubrenderFW_DN.h
        VulkanEngine/VESubrenderFW_DN.cpp
        VulkanEngine/VESubrenderFW_Shadow.h
        VulkanEngine/VESubrenderFW_Shadow.cpp
        VulkanEngine/VESubrenderFW_Skyplane.h
        VulkanEngine/VESubrenderFW_Skyplane.cpp
        VulkanEngine/VESubrenderFW_Cubemap2.h
        VulkanEngine/VESubrenderFW_Cubemap2.cpp
        VulkanEngine/VEMaterial.h
        VulkanEngine/VEMaterial.cpp
        VulkanEngine/VEWindow.h
        VulkanEngine/VEWindow.cpp
        VulkanEngine/VEWindowGLFW.h
        VulkanEngine/VEWindowGLFW.cpp
        VulkanEngine/VHBuffer.cpp
        VulkanEngine/VHCommand.cpp
        VulkanEngine/VHDebug.cpp
        VulkanEngine/VHDevice.cpp
        VulkanEngine/VHFile.cpp
        VulkanEngine/VHHelper.h
        VulkanEngine/VHMemory.cpp
        VulkanEngine/VHRender.cpp
        VulkanEngine/VHSwapchain.cpp
        VulkanEngine/vk_mem_alloc.h
)

target_link_libraries(game vulkan glfw assimp pthread)
