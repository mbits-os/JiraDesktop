include_directories(inc)
include_directories(pch)
include_directories(src)
include_directories(../libnet/inc)

set(json_SRCS
	pch/pch.cpp
	src/json.cpp
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(json STATIC ${json_SRCS})
set_target_properties(json PROPERTIES CXX_STANDARD 14)
