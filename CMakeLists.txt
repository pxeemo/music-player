cmake_minimum_required(VERSION 3.21)

project(karaoke VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Emit build/compile_commands.json so clangd and other LSPs get the exact
# flags (Qt include paths, moc defines) instead of guessing them.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Qml ShaderTools)

# Use Qt 6.5+'s recommended :/qt/qml/ prefix for QML module resources, so the
# URL in main.cpp is stable.
qt_policy(SET QTP0001 NEW)

# QML files in qml/ sit in a subdirectory of the module root. Without this the
# engine would only implicitly import that subdirectory, and none of the C++
# types registered in the Karaoke module would be visible to them.
qt_policy(SET QTP0004 NEW)

qt_add_executable(karaoke
    main.cpp
    lyrics.h
    lyrics.cpp
    playbackclock.h
    playbackclock.cpp
)

# Compile the GLSL sources to .qsb at build time instead of committing them.
# BATCHABLE is required for vertex shaders used with Qt Quick's ShaderEffect.
# The results are embedded as :/shaders/karaoke.vert.qsb and .frag.qsb
qt_add_shaders(karaoke "shaders"
    BATCHABLE
    PREFIX "/shaders"
    BASE   "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    FILES
        shaders/karaoke.vert
        shaders/karaoke.frag
)

# The QML files plus every C++ type marked with QML_ELEMENT().
qt_add_qml_module(karaoke
    URI Karaoke
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/KaraokeLine.qml
)

target_link_libraries(karaoke PRIVATE Qt6::Quick Qt6::Qml)
