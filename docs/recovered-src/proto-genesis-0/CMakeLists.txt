cmake_minimum_required(VERSION 3.16)
project(KMillerFM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(KF6 REQUIRED COMPONENTS KIO)

add_executable(kmiller
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(kmiller
    Qt6::Widgets
    KF6::KIOCore
    KF6::KIOWidgets
)
