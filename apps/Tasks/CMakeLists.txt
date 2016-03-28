find_package(CURL REQUIRED)

include_directories(
  ${PROJECT_SOURCE_DIR}/libs/libnet/inc
  ${PROJECT_SOURCE_DIR}/libs/libjson/inc
  ${PROJECT_SOURCE_DIR}/libs/libgui/inc
  ${PROJECT_SOURCE_DIR}/libs/libjira/inc
  inc
  pch
  src
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(tasks_SRCS
  src/AboutDlg.cpp
  src/AppModel.cpp
  src/AppSettings.cpp
  src/ConnectionDlg.cpp
  src/CredentialManager.cpp
  src/Logger.cpp
  src/LoginDlg.cpp
  src/stdafx.cpp
  src/Tasks.cpp
  src/TasksActions.cpp
  src/TasksFrame.cpp
  src/TasksView.cpp
  src/XHRConstructor.cpp
  src/Tasks.rc
)

set(tasks_INCS
  inc/AboutDlg.h
  inc/AppModel.h
  inc/AppSettings.h
  inc/ConnectionDlg.h
  inc/CredentialManager.h
  inc/Logger.hpp
  inc/LoginDlg.h
  inc/stdafx.h
  inc/TaskBarIcon.h
  inc/Tasks.h
  inc/TasksActions.h
  inc/TasksFrame.h
  inc/TasksView.h
  inc/XHRConstructor.h
  src/resource.h
  src/version.h
)

set(LANGS en pl)

foreach(LANG ${LANGS})
  add_custom_command(
    OUTPUT "${PROJECT_SOURCE_DIR}/strings/${LANG}/TasksStrings.mo"
	COMMAND msgfmt ARGS "${PROJECT_SOURCE_DIR}/strings/${LANG}/TasksStrings.po" -o "${PROJECT_SOURCE_DIR}/strings/${LANG}/TasksStrings.mo"
	MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/strings/${LANG}/TasksStrings.po"
  )

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/locale/Tasks.${LANG}"
	COMMAND lngs ARGS
	  make -vi "${CMAKE_CURRENT_SOURCE_DIR}/src/TasksStrings.idl"
	  -m "${PROJECT_SOURCE_DIR}/strings/${LANG}/TasksStrings.mo"
	  -o "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/locale/Tasks.${LANG}"
	  -l "${PROJECT_SOURCE_DIR}/strings/attr_language.txt"
	MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/strings/${LANG}/TasksStrings.mo"
	DEPENDS
	  src/TasksStrings.idl
	  ${PROJECT_SOURCE_DIR}/strings/attr_language.txt
  )
  list(APPEND tasks_INCS "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/locale/Tasks.${LANG}")
endforeach()

macro(make_lngs cmd output)
add_custom_command(
  OUTPUT "${output}"
  MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/src/TasksStrings.idl"
  COMMAND lngs ARGS ${cmd} -v -i "${CMAKE_CURRENT_SOURCE_DIR}/src/TasksStrings.idl" -o "${output}" ${ARGN}
)
list(APPEND tasks_SRCS "${output}")
endmacro()

set(LNGS_EMAIL "Marcin Zdun <mzdun@midnightbits.com>")

make_lngs(res   "${CMAKE_CURRENT_BINARY_DIR}/TasksStrings.cpp" -w --include "langs.h")
make_lngs(enums "${CMAKE_CURRENT_BINARY_DIR}/langs.h" -r)
make_lngs(pot   "${CMAKE_CURRENT_BINARY_DIR}/TasksStrings.pot" -c "${LNGS_EMAIL}" -a "${LNGS_EMAIL}")
make_lngs(py    "${CMAKE_CURRENT_BINARY_DIR}/langs.py")

find_path(LNGS_INCLUDE_DIR locale/locale.hpp
HINT
  $ENV{LNGS_ROOT}/include
  ${LNGS_ROOT}/include
)

find_library(LNGS_LIBRARY_R locale_r
HINT
  $ENV{LNGS_ROOT}/lib
  ${LNGS_ROOT}/lib
)

find_library(LNGS_LIBRARY_D locale_d
HINT
  $ENV{LNGS_ROOT}/lib
  ${LNGS_ROOT}/lib
)

mark_as_advanced(LNGS_INCLUDE_DIR LNGS_LIBRARY)

include_directories(${LNGS_INCLUDE_DIR})

set(tasks_LIBS json net gui jira ${CURL_LIBRARY} debug ${LNGS_LIBRARY_D} optimized ${LNGS_LIBRARY_R})
if (UNIX)
add_definitions(-pthread)
list(APPEND ut_LIBS
  pthread
  rt
)
endif (UNIX)

if (NOT CMAKE_HAVE_TS_FILESYSTEM)
list(APPEND tasks_LIBS
  boost_system
  boost_filesystem
)
endif (NOT CMAKE_HAVE_TS_FILESYSTEM)

add_executable(tasks WIN32 ${tasks_SRCS} ${tasks_INCS})
target_link_libraries(tasks ${tasks_LIBS})
set_target_properties(tasks PROPERTIES CXX_STANDARD 14)
