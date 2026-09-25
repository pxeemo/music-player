cmake_minimum_required(VERSION 3.21)

project(karaoke VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Quick
    Qml
    ShaderTools
)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(karaoke
	main.cpp
)

# Compile the GLSL sources to .qsb at build time instead of committing them.
# BATCHABLE is required for vertex shaders used with Qt Quick's ShaderEffect.
# The results are embedded as :/shaders/karaoke.vert.qsb and .frag.qsb
qt_add_shaders(karaoke "karaoke_shaders"
	PREFIX "/shaders"
	BATCHABLE
	BASE "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
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

    SOURCES
        src/playbackclock.h
        src/playbackclock.cpp
        src/lyrics.h
        src/lyrics.cpp
)

target_include_directories(karaoke
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(karaoke PRIVATE
    Qt6::Quick
    Qt6::Qml
)
