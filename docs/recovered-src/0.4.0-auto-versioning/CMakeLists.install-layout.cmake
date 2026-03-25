include(GNUInstallDirs)

# versioned payload
set(KMILLER_BASE "/opt/kmiller")
set(KMILLER_VERSIONS_DIR "${KMILLER_BASE}/versions/${PROJECT_VERSION}")
install(TARGETS kmiller RUNTIME DESTINATION ${KMILLER_VERSIONS_DIR}/bin)

# stable launcher wrapper
install(CODE "
file(WRITE \"${CMAKE_BINARY_DIR}/kmiller-launch\" \"#!/usr/bin/env bash
exec \\\"/usr/local/bin/kmiller\\\" \\\"\\$@\\\"
\")
")
install(PROGRAMS ${CMAKE_BINARY_DIR}/kmiller-launch
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME kmiller)

# desktop entry -> correct absolute path
install(CODE "
file(WRITE \"${CMAKE_BINARY_DIR}/kmiller.desktop\" \"[Desktop Entry]\\nType=Application\\nName=KMiller\\nGenericName=File Manager\\nComment=Finder-style file manager with Miller Columns and Quick Look\\nExec=kmiller %U\\nIcon=kmiller\\nTerminal=false\\nCategories=Qt;KDE;Utility;FileManager;\\nMimeType=inode/directory;\\n\")
")
install(FILES ${CMAKE_BINARY_DIR}/kmiller.desktop
        DESTINATION /usr/local/share/applications)
