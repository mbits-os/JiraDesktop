find_package(CURL REQUIRED)

include_directories(inc)
include_directories(pch)
include_directories(src)
include_directories(${CURL_INCLUDE_DIR})

set(net_SRCS
	pch/pch.cpp
	src/curl_http.cpp
	src/uri.cpp
	src/utf8.cpp
	src/xhr.cpp
)

if (WIN32)

list(APPEND net_SRCS
	src/win32/win32_xhr.cpp
)

elseif(UNIX)

list(APPEND net_SRCS
	src/posix/posix_xhr.cpp
)

endif (WIN32)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(net STATIC ${net_SRCS})
set_target_properties(net PROPERTIES CXX_STANDARD 14)
