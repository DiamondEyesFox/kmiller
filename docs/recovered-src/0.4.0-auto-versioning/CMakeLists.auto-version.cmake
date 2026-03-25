cmake_minimum_required(VERSION 3.16)

# Determine version in this order:
#  - -DKMILLER_VERSION=... (cmake arg)
#  - $ENV{KMILLER_VERSION}
#  - git describe --tags (if in a git repo)
#  - fallback
set(_VER "")
if(DEFINED KMILLER_VERSION AND NOT "${KMILLER_VERSION}" STREQUAL "")
  set(_VER "${KMILLER_VERSION}")
elseif(DEFINED ENV{KMILLER_VERSION} AND NOT "$ENV{KMILLER_VERSION}" STREQUAL "")
  set(_VER "$ENV{KMILLER_VERSION}")
else()
  # Try git
  execute_process(
    COMMAND git describe --tags --dirty --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _GITVER
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if(_GITVER)
    set(_VER "${_GITVER}")
  else()
    set(_VER "0.0.0-dev")
  endif()
endif()

project(KMiller VERSION ${_VER} LANGUAGES CXX)
message(STATUS "KMiller version: ${PROJECT_VERSION}")
