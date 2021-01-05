add_executable(controller main.cpp Computer.cpp Window/Window.cpp Camera/Camera.cpp Mesh/Mesh.cpp Shader/Shader.cpp Texture/Texture.cpp SDL-Helper-Libraries/sfstream/sfstream.cpp SDL-Helper-Libraries/KeyTracker/KeyTracker.cpp Shader/Shader.cpp Camera/Camera.hpp TexturedMesh/TexturedMesh.cpp imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_sdl.cpp imgui/backends/imgui_impl_opengl3.cpp imgui/misc/cpp/imgui_stdlib.cpp)

find_package(Boost REQUIRED serialization)
find_package(Threads REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(nlohmann_json_cmake_fetchcontent)

target_include_directories(controller PRIVATE ./ ./websocketpp ${Boost_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} imgui/ imgui/backends/)
target_link_libraries(controller PRIVATE ${Boost_LIBRARIES} Threads::Threads nlohmann_json::nlohmann_json GLEW::GLEW ${SDL2_LIBRARIES} OpenGL::GL)

add_custom_target(controller_CopyFiles COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res)

add_dependencies(controller controller_CopyFiles)
