cmake_minimum_required (VERSION 3.8.1)

project (patchclamp)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Widgets)
find_package(Qt5Multimedia)

add_executable(patchclamp patchclamp/main.cpp)

target_link_libraries(patchclamp "patchclamp/mainwindow.cpp" Qt5::Widgets)

install(TARGETS patchclamp DESTINATION dist/)
