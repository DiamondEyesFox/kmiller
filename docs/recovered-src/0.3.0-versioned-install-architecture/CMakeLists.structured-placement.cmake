cmake_minimum_required(VERSION 3.16)
project(KMiller VERSION 0.3.0 LANGUAGES CXX)  # <— bump this each release


add_executable(kmiller
    src/main.cpp
    src/MainWindow.cpp
    src/Pane.cpp
    src/MillerView.cpp
    src/QuickLookDialog.cpp
    src/ThumbCache.cpp
    src/SettingsDialog.cpp
    # and their headers
)

target_link_libraries(kmiller
    Qt6::Widgets
    Qt6::Multimedia
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets
    KF6::I18n
    Poppler::Qt6
)


include(GNUInstallDirs)

# install into /opt/kmiller/versions/<version>/bin
set(KMILLER_BASE "/opt/kmiller")
set(KMILLER_VERSIONS_DIR "${KMILLER_BASE}/versions/${PROJECT_VERSION}")
install(TARGETS kmiller RUNTIME DESTINATION ${KMILLER_VERSIONS_DIR}/bin)

# launcher shim (execs the current symlink target)
install(CODE "
file(WRITE \"${CMAKE_BINARY_DIR}/kmiller-launch\" \"#!/usr/bin/env bash
exec \\\"/usr/local/bin/kmiller\\\" \\\"\\$@\\\"
\")
")
install(PROGRAMS ${CMAKE_BINARY_DIR}/kmiller-launch DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME kmiller)

# desktop entry pointing at the stable launcher
install(CODE "
file(WRITE \"${CMAKE_BINARY_DIR}/kmiller.desktop\" \"[Desktop Entry]\\nType=Application\\nName=KMiller\\nGenericName=File Manager\\nComment=Finder-style file manager with Miller Columns and Quick Look\\nExec=kmiller %U\\nIcon=kmiller\\nTerminal=false\\nCategories=Qt;KDE;Utility;FileManager;\\nMimeType=inode/directory;\\n\")
")
install(FILES ${CMAKE_BINARY_DIR}/kmiller.desktop DESTINATION ${CMAKE_INSTALL_DATAROOT_DIR}/applications)
