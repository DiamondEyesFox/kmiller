cmake_minimum_required(VERSION 3.16)

# ===== Version detection =====
# Priority: -DKMILLER_VERSION=...  ->  $KMILLER_VERSION  ->  git describe  ->  release fallback
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
    set(_VER "5.25.1")  # release fallback
  endif()
endif()

# Normalize git/tag strings like "v5.25-3-g<hash>" into semver "5.25".
string(REGEX REPLACE "^v" "" _VER "${_VER}")
string(REGEX MATCH "^[0-9]+(\\.[0-9]+){0,2}" _VER_SEMVER "${_VER}")
if(_VER_SEMVER)
  set(_VER "${_VER_SEMVER}")
else()
  set(_VER "5.25.1")
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

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets DBus)
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
    src/FileOpsService.cpp
    src/FileOpsService.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/PropertiesDialog.cpp
    src/PropertiesDialog.h
    src/FileChooserPortal.cpp
    src/FileChooserPortal.h
    src/FileChooserDialog.cpp
    src/FileChooserDialog.h
)

target_include_directories(kmiller PRIVATE src)

target_link_libraries(kmiller
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::DBus
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

# Install patch notes when present
if(EXISTS ${CMAKE_SOURCE_DIR}/PATCHNOTES.md)
  install(FILES ${CMAKE_SOURCE_DIR}/PATCHNOTES.md
          DESTINATION /opt/kmiller
          RENAME PATCHNOTES-latest.md)
else()
  message(WARNING "PATCHNOTES.md not found; skipping patch note installation")
endif()

# ===== FileChooser Portal files =====
# Portal registration
if(EXISTS ${CMAKE_SOURCE_DIR}/data/kmiller.portal)
  install(FILES ${CMAKE_SOURCE_DIR}/data/kmiller.portal
          DESTINATION /usr/share/xdg-desktop-portal/portals)
else()
  message(WARNING "data/kmiller.portal not found; skipping portal file installation")
endif()

# D-Bus service for portal auto-activation
if(EXISTS ${CMAKE_SOURCE_DIR}/data/org.freedesktop.impl.portal.desktop.kmiller.service)
  install(FILES ${CMAKE_SOURCE_DIR}/data/org.freedesktop.impl.portal.desktop.kmiller.service
          DESTINATION /usr/share/dbus-1/services)
else()
  message(WARNING "portal D-Bus service file not found; skipping service installation")
endif()

# Provide version string to the code as KMILLER_VERSION_STR
if(NOT DEFINED PROJECT_VERSION)
  set(PROJECT_VERSION "${KMILLER_VERSION}")
endif()
target_compile_definitions(kmiller PRIVATE KMILLER_VERSION_STR="${PROJECT_VERSION}")

option(KMILLER_ENABLE_TESTS "Enable lightweight smoke tests" ON)
if(KMILLER_ENABLE_TESTS)
  enable_testing()
  add_test(
    NAME kmiller_linkage_smoke
    COMMAND ${CMAKE_SOURCE_DIR}/tests/smoke/ldd_no_missing.sh $<TARGET_FILE:kmiller>
  )
endif()
