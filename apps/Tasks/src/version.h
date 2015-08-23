#pragma once

#define PROGRAM_NAME "Tasks"
#define PROGRAM_COPYRIGHT_HOLDER "midnightBITS"

#define VERSION_STRINGIFY(n) VERSION_STRINGIFY_HELPER(n)
#define VERSION_STRINGIFY_HELPER(n) #n

#define PROGRAM_VERSION_MAJOR 0
#define PROGRAM_VERSION_MINOR 4
#define PROGRAM_VERSION_PATCH 7
#define PROGRAM_VERSION_BUILD 59
#define PROGRAM_VERSION_STABILITY "-alpha" // "-beta", "-rc", and ""

#define PROGRAM_VERSION_STRING \
     VERSION_STRINGIFY(PROGRAM_VERSION_MAJOR) "." \
     VERSION_STRINGIFY(PROGRAM_VERSION_MINOR) "." \
     VERSION_STRINGIFY(PROGRAM_VERSION_PATCH)

#define PROGRAM_VERSION_FULL \
     VERSION_STRINGIFY(PROGRAM_VERSION_MAJOR) "." \
     VERSION_STRINGIFY(PROGRAM_VERSION_MINOR) "." \
     VERSION_STRINGIFY(PROGRAM_VERSION_PATCH) \
	 PROGRAM_VERSION_STABILITY "+" \
	 VERSION_STRINGIFY(PROGRAM_VERSION_BUILD)

#define PROGRAM_VERSION_STRING_SHORT \
     VERSION_STRINGIFY(PROGRAM_VERSION_MAJOR) "." \
     VERSION_STRINGIFY(PROGRAM_VERSION_MINOR)

#define PROGRAM_VERSION_RC PROGRAM_VERSION_MAJOR ##, \
                         ##PROGRAM_VERSION_MINOR ##, \
                         ##PROGRAM_VERSION_PATCH
