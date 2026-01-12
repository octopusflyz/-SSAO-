add_executable(particle-system
    main.cpp
    particle_system.hpp
    particle_system.cpp
    particle_material.hpp
    particle_material.cpp
)


target_include_directories(particle-system PRIVATE ../05-pbr)


target_sources(particle-system PRIVATE
    ../05-pbr/material.cpp
    ../05-pbr/material.hpp
)

target_link_libraries(particle-system PRIVATE common)