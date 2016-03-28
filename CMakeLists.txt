cmake_minimum_required (VERSION 2.8)
project (JiraDesktop)
set(CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
  ${CMAKE_MODULE_PATH})

set(PROGRAM_VERSION_MAJOR 0)
set(PROGRAM_VERSION_MINOR 4)
set(PROGRAM_VERSION_PATCH 7)
set(PROGRAM_VERSION_BUILD 59)
set(PROGRAM_VERSION_STABILITY "-alpha")
set(PROGRAM_VERSION ${PROGRAM_VERSION_MAJOR}.${PROGRAM_VERSION_MINOR}.${PROGRAM_VERSION_PATCH} )

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/program_version.h.in ${PROJECT_BINARY_DIR}/program_version.h)

include (BuildConfig)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

if (WIN32)
find_package (WTL)
include_directories(${WTL_INCLUDE_DIRS})
endif (WIN32)

add_subdirectory(libs)
add_subdirectory(gtest)
add_subdirectory(apps)
# add_subdirectory(lngs)
