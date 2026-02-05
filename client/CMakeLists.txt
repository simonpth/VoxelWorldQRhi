cmake_minimum_required(VERSION 3.16)

project(client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate Qml Quick ShaderTools)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appClient
    main.cpp
)

target_link_libraries(appClient PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
)

target_include_directories(appClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include)
target_link_libraries(appClient PRIVATE voxel-shared)

qt_add_qml_module(appClient
    URI Client
    QML_FILES
        Main.qml
    SOURCES
        engine.h engine.cpp
        gameloop.h gameloop.cpp
        rhirender.h rhirender.cpp
        rhiview.h rhiview.cpp
)

qt_add_shaders(appClient "client_shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        /
    BASE
        ./shader
    FILES
        ./shader/shader.vert
        ./shader/shader.frag
)

set_target_properties(appClient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS appClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
