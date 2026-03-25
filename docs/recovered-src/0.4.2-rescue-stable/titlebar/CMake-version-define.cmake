project(KMiller VERSION 0.4.2 LANGUAGES CXX)

add_executable(kmiller
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

target_compile_definitions(kmiller PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
