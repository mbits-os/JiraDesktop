INCLUDE (CheckIncludeFileCXX)

if (NOT MSVC)
set(CMAKE_REQUIRED_FLAGS "-std=c++1z")
endif(NOT MSVC)

check_include_file_cxx(experimental/filesystem CMAKE_HAVE_TS_FILESYSTEM)
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmakeconfig.h.in ${PROJECT_BINARY_DIR}/cmakeconfig.h)

if (NOT CMAKE_HAVE_TS_FILESYSTEM)
find_package (Boost)
include_directories(${Boost_INCLUDE_DIRS})
endif (NOT CMAKE_HAVE_TS_FILESYSTEM)

if (WIN32)
add_definitions(-DUNICODE -D_UNICODE -DWIN32 -D_WIN32)
endif(WIN32)
