include_directories(inc)
include_directories(pch)
include_directories(src)
include_directories(../libnet/inc)
include_directories(../libjson/inc)
include_directories(../libgui/inc)

set(jira_SRCS
  pch/pch.cpp

  src/jira.cpp
  src/server.cpp
  src/types.cpp
)

set(jira_INCS
  inc/jira/jira.hpp
  inc/jira/server.hpp
  pch/pch.h
  src/types.hpp
)

if (WIN32)
list(APPEND jira_SRCS
  src/win32/win32_secure.cpp
)

list(APPEND jira_INCS
)
endif(WIN32)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(jira STATIC ${jira_SRCS} ${jira_INCS})
set_target_properties(jira PROPERTIES CXX_STANDARD 14)
