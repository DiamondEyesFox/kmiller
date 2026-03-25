cmake_minimum_required(VERSION 3.16)
project(KMiller2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(KF6 REQUIRED COMPONENTS KIO)
find_package(Poppler REQUIRED COMPONENTS Qt6)

add_executable(kmiller2
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/TabPage.cpp
    src/TabPage.h
    src/MillerView.cpp
    src/MillerView.h
    src/QuickLookDialog.cpp
    src/QuickLookDialog.h
)

target_include_directories(kmiller2 PRIVATE src)

target_link_libraries(kmiller2
    Qt6::Widgets
    Qt6::Multimedia
    KF6::KIOCore
    KF6::KIOWidgets
    Poppler::Qt6
)
