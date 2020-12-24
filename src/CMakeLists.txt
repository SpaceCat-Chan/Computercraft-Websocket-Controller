add_executable(controller main.cpp)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(nlohmann_json_cmake_fetchcontent)

target_include_directories(controller PRIVATE ./websocketpp ${Boost_INCLUDE_DIRS})
target_link_libraries(controller PRIVATE ${Boost_LIBRARIES} Threads::Threads nlohmann_json::nlohmann_json)
