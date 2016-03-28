/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
// inspired by WTF  Platform.h

/* HAVE() - specific system features (headers, functions or similar) that are present or not */
#define HAVE(FEATURE) (defined HAVE_##FEATURE  && HAVE_##FEATURE)

/* OS() - underlying operating system */
#define OS(FEATURE) (defined BUILD_OS_##FEATURE  && BUILD_OS_##FEATURE)

/* ==== OS() - underlying operating system; only to be used for mandated low-level services like
virtual memory, not to choose a GUI toolkit ==== */

/* OS(AIX) - AIX */
#ifdef _AIX
#define BUILD_OS_AIX 1
#endif

/* OS(DARWIN) - Any Darwin-based OS, including Mac OS X and iPhone OS */
#ifdef __APPLE__
#define BUILD_OS_DARWIN 1

#include <Availability.h>
#include <AvailabilityMacros.h>
#include <TargetConditionals.h>
#endif

/* OS(IOS) - iOS */
/* OS(MAC_OS_X) - Mac OS X (not including iOS) */
#if OS(DARWIN) && ((defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) \
    || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)                 \
    || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR))
#define BUILD_OS_IOS 1
#elif OS(DARWIN) && defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define BUILD_OS_MAC_OS_X 1
#endif

/* OS(FREEBSD) - FreeBSD */
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#define BUILD_OS_FREEBSD 1
#endif

/* OS(HURD) - GNU/Hurd */
#ifdef __GNU__
#define BUILD_OS_HURD 1
#endif

/* OS(LINUX) - Linux */
#ifdef __linux__
#define BUILD_OS_LINUX 1
#endif

/* OS(NETBSD) - NetBSD */
#if defined(__NetBSD__)
#define BUILD_OS_NETBSD 1
#endif

/* OS(OPENBSD) - OpenBSD */
#ifdef __OpenBSD__
#define BUILD_OS_OPENBSD 1
#endif

/* OS(SOLARIS) - Solaris */
#if defined(sun) || defined(__sun)
#define BUILD_OS_SOLARIS 1
#endif

/* OS(WINDOWS) - Any version of Windows */
#if defined WIN32 || defined _WIN32
#define BUILD_OS_WINDOWS 1
#endif

#define BUILD_OS_WIN ERROR "USE WINDOWS WITH OS NOT WIN"
#define BUILD_OS_MAC ERROR "USE MAC_OS_X WITH OS NOT MAC"

/* OS(UNIX) - Any Unix-like system */
#if    OS(AIX)              \
    || OS(DARWIN)           \
    || OS(FREEBSD)          \
    || OS(HURD)             \
    || OS(LINUX)            \
    || OS(NETBSD)           \
    || OS(OPENBSD)          \
    || OS(SOLARIS)          \
    || defined(unix)        \
    || defined(__unix)      \
    || defined(__unix__)
#define BUILD_OS_UNIX 1
#endif

/* Operating environments */
