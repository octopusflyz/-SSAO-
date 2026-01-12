add_executable(08-ssao main.cpp material.cpp)

target_link_libraries(08-ssao PRIVATE common)
target_include_directories(08-ssao PRIVATE ${CMAKE_SOURCE_DIR}/data)