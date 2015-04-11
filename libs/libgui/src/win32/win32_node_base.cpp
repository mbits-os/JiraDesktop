#include "pch.h"

#ifdef max
#undef max
#endif

#include <gui/nodes/node_base.hpp>
#include <net/utf8.hpp>
#include "windows.h"
#include "shellapi.h"

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	void node_base::openLink(const std::string& url)
	{
		ShellExecute(nullptr, nullptr, utf::widen(url).c_str(), nullptr, nullptr, SW_SHOW);
	}
}
