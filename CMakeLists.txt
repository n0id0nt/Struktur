# The engine on its own. Games live in their own folders and include cmake/Struktur.cmake; building this project builds
# the library checks: the engine compiled for each library, and for combinations of them, linked into an empty program,
# so code that uses a library a game leaves out fails here.
#
#   cmake -S . -B build-checks -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-checks --config Debug --parallel
#
# The standalone test projects are separate: src/Engine/Jobs and src/Engine/Fitness.
cmake_minimum_required(VERSION 3.24)
project(Struktur C CXX)

include(cmake/Struktur.cmake)

# Each library with only what it requires.
struktur_add_check(core)
struktur_add_check(ecs LIBRARIES ecs)
struktur_add_check(wren LIBRARIES wren)
struktur_add_check(ui LIBRARIES ui)
struktur_add_check(physics LIBRARIES physics)
struktur_add_check(world2d LIBRARIES world2d)
struktur_add_check(dialogue LIBRARIES dialogue)
struktur_add_check(flag LIBRARIES flag)
struktur_add_check(fitness LIBRARIES fitness)

# Libraries that use each other, in the combinations games choose.
struktur_add_check(wren_ui_fitness LIBRARIES ecs wren ui fitness)
struktur_add_check(wren_ui_physics_dialogue_flag LIBRARIES ecs wren ui physics dialogue flag)
struktur_add_check(world2d_physics LIBRARIES world2d physics)
struktur_add_check(wren_ui_physics_world2d_dialogue_flag LIBRARIES ecs wren ui physics world2d dialogue flag)
struktur_add_check(all LIBRARIES ${STRUKTUR_ALL_LIBRARIES})

# The editor, which uses whichever of ui, physics, world2d, dialogue and wren a game has.
if(EDITOR)
    struktur_add_check(editor EDITOR)
    struktur_add_check(editor_wren LIBRARIES wren EDITOR)
    struktur_add_check(editor_ui LIBRARIES ui EDITOR)
    struktur_add_check(editor_physics LIBRARIES physics EDITOR)
    struktur_add_check(editor_world2d LIBRARIES world2d EDITOR)
    struktur_add_check(editor_dialogue LIBRARIES dialogue EDITOR)
    struktur_add_check(editor_wren_ui_fitness LIBRARIES ecs wren ui fitness EDITOR)
    struktur_add_check(editor_wren_ui_physics_dialogue_flag LIBRARIES ecs wren ui physics dialogue flag EDITOR)
    struktur_add_check(editor_wren_ui_physics_world2d_dialogue_flag
        LIBRARIES ecs wren ui physics world2d dialogue flag EDITOR)
    struktur_add_check(editor_all LIBRARIES ${STRUKTUR_ALL_LIBRARIES} EDITOR)
endif()
