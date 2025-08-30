cmake_minimum_required(VERSION 3.16)

# ===== Version detection =====
# Priority: -DKMILLER_VERSION=...  ->  $KMILLER_VERSION  ->  git describe  ->  0.0.0-dev
set(_VER "")
if(DEFINED KMILLER_VERSION AND NOT "${KMILLER_VERSION}" STREQUAL "")
  set(_VER "${KMILLER_VERSION}")
elseif(DEFINED ENV{KMILLER_VERSION} AND NOT "$ENV{KMILLER_VERSION}" STREQUAL "")
  set(_VER "$ENV{KMILLER_VERSION}")
else()
  execute_process(
    COMMAND git describe --tags --dirty --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _GITVER
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if(_GITVER)
    set(_VER "${_GITVER}")
  else()
    set(_VER "0.3.0")  # fallback default; change if you want
  endif()
endif()
project(KMiller VERSION ${_VER} LANGUAGES CXX)
message(STATUS "KMiller version: ${PROJECT_VERSION}")

# ===== Build settings =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ===== Dependencies =====
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)
find_package(KF6KIO REQUIRED)               # provides KF6::KIOCore/KIOWidgets/KIOFileWidgets
find_package(Poppler REQUIRED COMPONENTS Qt6)

# ===== Sources / target =====
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

target_include_directories(kmiller PRIVATE src)

target_link_libraries(kmiller
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets
    Poppler::Qt6
)

# ===== Install rules (versioned payload + stable launcher + desktop) =====
include(GNUInstallDirs)

# Versioned install location: /opt/kmiller/versions/<version>/bin/kmiller
set(KMILLER_BASE "/opt/kmiller")
set(KMILLER_VERSIONS_DIR "${KMILLER_BASE}/versions/${PROJECT_VERSION}")
install(TARGETS kmiller RUNTIME DESTINATION ${KMILLER_VERSIONS_DIR}/bin)

# Create/refresh stable launcher symlink at /usr/local/bin/kmiller -> current version
install(CODE "
  file(MAKE_DIRECTORY \"/usr/local/bin\")
  execute_process(COMMAND \"/bin/ln\" -sfn
                  \"${KMILLER_VERSIONS_DIR}/bin/kmiller\"
                  \"/usr/local/bin/kmiller\")
  message(STATUS \"kmiller symlink -> ${KMILLER_VERSIONS_DIR}/bin/kmiller\")
")

# Desktop entry
install(CODE "
  file(WRITE \"${CMAKE_BINARY_DIR}/kmiller.desktop\" \"[Desktop Entry]\\nType=Application\\nName=KMiller\\nGenericName=File Manager\\nComment=Finder-style file manager with Miller Columns and Quick Look\\nExec=kmiller %U\\nIcon=kmiller\\nTerminal=false\\nCategories=Qt;KDE;Utility;FileManager;\\nMimeType=inode/directory;\\n\")
")
install(FILES ${CMAKE_BINARY_DIR}/kmiller.desktop
        DESTINATION /usr/local/share/applications)

# --- also install the source snapshot alongside the binary ---
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src
        DESTINATION ${KMILLER_VERSIONS_DIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.cpp")

install(FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt
        DESTINATION ${KMILLER_VERSIONS_DIR})

# Provide version string to the code as KMILLER_VERSION_STR
if(NOT DEFINED PROJECT_VERSION)
  set(PROJECT_VERSION "${KMILLER_VERSION}")
endif()
target_compile_definitions(kmiller PRIVATE KMILLER_VERSION_STR="${PROJECT_VERSION}")
