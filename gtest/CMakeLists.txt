include_directories(
  ${PROJECT_SOURCE_DIR}/libs/libnet/inc
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(gtest_SRCS
  src/gtest-all.cc
  src/gtest_main.cc
  ../libs/libnet/ut/uri.cc
)

set(ut_LIBS net)
if (WIN32)
list(APPEND ut_LIBS
  gui
  jira
)
else (WIN32)
add_definitions(-pthread)
list(APPEND ut_LIBS
  pthread
  rt
)
endif (WIN32)

if (NOT CMAKE_HAVE_TS_FILESYSTEM)
list(APPEND ut_LIBS
  boost_system
  boost_filesystem
)
endif (NOT CMAKE_HAVE_TS_FILESYSTEM)

add_executable(libnet-ut ${gtest_SRCS})
target_link_libraries(libnet-ut ${ut_LIBS})
set_target_properties(libnet-ut PROPERTIES CXX_STANDARD 14)
