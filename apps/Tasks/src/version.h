#pragma once

#define PROGRAM_NAME "Tasks"
#define PROGRAM_COPYRIGHT_HOLDER "midnightBITS"

#define VERSION_STRINGIFY(n) VERSION_STRINGIFY_HELPER(n)
#define VERSION_STRINGIFY_HELPER(n) #n

#define PROGRAM_VERSION_MAJOR 0
#define PROGRAM_VERSION_MINOR 1
#define PROGRAM_VERSION_PATCH 0

#define PROGRAM_VERSION_STRING \
     VERSION_STRINGIFY(PROGRAM_VERSION_MAJOR) "." \
     VERSION_STRINGIFY(PROGRAM_VERSION_MINOR) "." \
     VERSION_STRINGIFY(PROGRAM_VERSION_PATCH)

#define PROGRAM_VERSION_RC PROGRAM_VERSION_MAJOR ##, \
                         ##PROGRAM_VERSION_MINOR ##, \
                         ##PROGRAM_VERSION_PATCH
