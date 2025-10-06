cmake_minimum_required(VERSION 3.31)
project(GL_Craft)

set(CMAKE_CXX_STANDARD 23)

set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")
set(IMGUI_DIR ${LIBS_DIR}/imgui)

if (UNIX)
    cmake_policy(SET CMP0072 NEW)
    find_package(OpenGL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)

    include_directories(
            ${GLFW_INCLUDE_DIRS}
            ${LIBS_DIR}/glad/include
            ${LIBS_DIR}
    )
    link_directories(${GLFW_LIBRARY_DIRS})

    set(EXTRA_LIBS
            ${GLFW_LIBRARIES}
            OpenGL::GL
    )

elseif (WIN32)
    include_directories(
            ${LIBS_DIR}/glad/include
            ${LIBS_DIR}/glfw/include
            ${LIBS_DIR}
    )

    link_directories(${LIBS_DIR}/glfw/lib-mingw-w64)

    set(EXTRA_LIBS
            glfw3
            opengl32
    )
endif ()

add_executable(GL_Craft
        ${LIBS_DIR}/glad/src/gl.c
        src/core/Application.cpp
        src/core/Application.h
        src/core/Main.cpp
        src/gl/Shader.cpp
        src/gl/Shader.cpp
        src/gl/Shader.h
        src/gl/VertexBuffer.h
        src/gl/VertexBuffer.cpp
        src/gl/VertexArray.h
        src/gl/VertexArray.cpp
        src/gl/IndexBuffer.cpp
        src/gl/IndexBuffer.h
        src/render/Renderer.cpp
        src/render/Renderer.h
        src/gl/Texture.cpp
        src/gl/Texture.h
        src/render/Camera.cpp
        src/render/Camera.h
        src/world/Block.cpp
        src/world/Block.h
        src/world/chunk/Chunk.cpp
        src/world/chunk/Chunk.h
        src/world/World.cpp
        src/world/World.h
        src/math/Frustum.cpp
        src/math/Frustum.h
        src/math/Plane.cpp
        src/math/Plane.h
        src/math/AABB.cpp
        src/math/AABB.h
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/imgui_impl_opengl3.cpp
        src/ui/DebugUI.cpp
        src/ui/DebugUI.h
        src/utils/ThreadSafeQueue.h
        src/utils/ThreadPool.h
        src/utils/ThreadPool.cpp
        src/world/chunk/Mesh.h
        src/world/chunk/Mesh.cpp
        src/world/generation/surfaceFeatures/grass/ShortGrass.cpp
        src/world/generation/surfaceFeatures/grass/ShortGrass.h
        src/world/generation/surfaceFeatures/flowers/Flower.h
        src/world/generation/surfaceFeatures/flowers/Flower.cpp
        src/world/generation/surfaceFeatures/flowers/Poppy.cpp
        src/world/generation/surfaceFeatures/flowers/Poppy.h
        src/render/InstanceRenderer.cpp
        src/render/InstanceRenderer.h
        src/world/chunk/MeshManager.h
        src/world/generation/surfaceFeatures/flowers/Cornflower.cpp
        src/world/generation/surfaceFeatures/flowers/Cornflower.h
        src/world/generation/surfaceFeatures/flowers/Allium.cpp
        src/world/generation/surfaceFeatures/flowers/Allium.h
        src/gl/FrameBuffer.cpp
        src/gl/FrameBuffer.h
        src/render/PostProcessingMesh.cpp
        src/render/PostProcessingMesh.h
        src/world/generation/surfaceFeatures/SurfaceFeature.h
        src/world/chunk/ChunkPosition.h
        src/world/chunk/PendingBlock.h
        src/math/Raycast.cpp
        src/math/Raycast.h
        src/math/CollisionDetector.cpp
        src/math/CollisionDetector.h
        src/render/HighlightedBlock.cpp
        src/render/HighlightedBlock.h
        src/ui/Crosshair.cpp
        src/ui/Crosshair.h
        src/gl/UniformBuffer.cpp
        src/gl/UniformBuffer.h
        src/entity/Player.cpp
        src/entity/Player.h
        src/gl/StorageBuffer.cpp
        src/gl/StorageBuffer.h
        src/render/IndirectRenderer.cpp
        src/render/IndirectRenderer.h
        src/gl/IndirectBuffer.cpp
        src/gl/IndirectBuffer.h
        src/utils/ScopedTimer.cpp
        src/utils/ScopedTimer.h
        src/world/WorldManager.cpp
        src/world/WorldManager.h
        src/world/generation/TerrainGenerator.cpp
        src/world/generation/TerrainGenerator.h
        src/render/WorldRenderer.cpp
        src/render/WorldRenderer.h
        src/gl/TextureArray.cpp
        src/gl/TextureArray.h
        src/world/generation/surfaceFeatures/SurfaceFeature.cpp
        src/world/chunk/RGBLight.h
        src/world/chunk/LightConstants.h
        src/world/chunk/ChunkLighting.cpp
        src/world/chunk/ChunkLighting.h
)

target_link_libraries(GL_Craft PRIVATE ${EXTRA_LIBS})

target_compile_definitions(GL_Craft PUBLIC
        $<$<CONFIG:Debug>:DEBUG_BUILD>
        $<$<CONFIG:Release>:RELEASE_BUILD>
        $<$<CONFIG:RelWithDebInfo>:RELWITHDEBINFO_BUILD>
)
