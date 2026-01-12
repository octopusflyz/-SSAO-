add_executable(07-normal-mapping main.cpp normal_mapping_material.cpp)

target_link_libraries(07-normal-mapping PRIVATE common)
target_include_directories(07-normal-mapping PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})