cmake_minimum_required(VERSION 3.16)
project(KMiller3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)
find_package(KF6KIO REQUIRED)
find_package(Poppler REQUIRED COMPONENTS Qt6)

add_executable(kmiller3
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/Pane.cpp
    src/Pane.h
    src/MillerView.cpp
    src/MillerView.h
    src/QuickLookDialog.cpp
    src/QuickLookDialog.h
    src/ThumbCache.cpp
    src/ThumbCache.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
)

target_include_directories(kmiller3 PRIVATE src)

target_link_libraries(kmiller3
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets
    Poppler::Qt6
)
