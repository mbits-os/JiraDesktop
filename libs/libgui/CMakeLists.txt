include_directories(inc)
include_directories(pch)
include_directories(src)
include_directories(../libnet/inc)
include_directories(../libjson/inc)

set(gui_SRCS
  pch/pch.cpp

  src/animation.cpp
  src/cairo_painter.cpp
  src/font_awesome.cc
  src/hotkey.cpp
  src/json_settings.cpp
  src/nodes/block_node.cpp
  src/nodes/caption_row_node.cpp
  src/nodes/document_impl.cpp
  src/nodes/doc_element.cpp
  src/nodes/image_nodes.cpp
  src/nodes/link_node.cpp
  src/nodes/node_base.cpp
  src/nodes/row_node.cpp
  src/nodes/span_node.cpp
  src/nodes/table_node.cpp
  src/nodes/text_node.cpp
  src/painter_base.cpp
  src/settings.cpp
  src/styles.cpp
)

set(gui_INCS
  inc/gui/action.hpp
  inc/gui/animation.hpp
  inc/gui/cairo_painter.hpp
  inc/gui/document.hpp
  inc/gui/font_awesome.hh
  inc/gui/gdi_painter.hpp
  inc/gui/hotkey.hpp
  inc/gui/image.hpp
  inc/gui/listeners.hpp
  inc/gui/menu.hpp
  inc/gui/node.hpp
  inc/gui/nodes/block_node.hpp
  inc/gui/nodes/caption_row_node.hpp
  inc/gui/nodes/document_impl.hpp
  inc/gui/nodes/doc_element.hpp
  inc/gui/nodes/image_nodes.hpp
  inc/gui/nodes/link_node.hpp
  inc/gui/nodes/node_base.hpp
  inc/gui/nodes/row_node.hpp
  inc/gui/nodes/span_node.hpp
  inc/gui/nodes/table_node.hpp
  inc/gui/nodes/text_node.hpp
  inc/gui/painter.hpp
  inc/gui/painter_base.hpp
  inc/gui/settings.hpp
  inc/gui/styles.hpp
  inc/gui/types.hpp

  pch/pch.h

  src/json_settings.hpp
  src/settings_impl.hpp
)

if (WIN32)
list(APPEND gui_SRCS
  src/win32/win32_action.cpp
  src/win32/win32_animation.cpp
  src/win32/win32_gdi_painter.cpp
  src/win32/win32_hotkey.cpp
  src/win32/win32_menu.cpp
  src/win32/win32_node_base.cpp
  src/win32/win32_settings.cpp
)

list(APPEND gui_INCS
  inc/gui/win32_animation.hpp
  src/win32/win32_settings.hpp
)
endif(WIN32)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(gui STATIC ${gui_SRCS} ${gui_INCS})
set_target_properties(gui PROPERTIES CXX_STANDARD 14)
