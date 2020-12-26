add_executable(controller main.cpp Computer.cpp Window/Window.cpp Camera/Camera.cpp Mesh/Mesh.cpp Shader/Shader.cpp Texture/Texture.cpp SDL-Helper-Libraries/sfstream/sfstream.cpp SDL-Helper-Libraries/KeyTracker/KeyTracker.cpp)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(nlohmann_json_cmake_fetchcontent)

target_include_directories(controller PRIVATE ./ ./websocketpp ${Boost_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
target_link_libraries(controller PRIVATE ${Boost_LIBRARIES} Threads::Threads nlohmann_json::nlohmann_json GLEW::GLEW ${SDL2_LIBRARIES} OpenGL::GL)
