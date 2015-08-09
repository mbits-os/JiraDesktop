#include "pch.h"
#include <stdint.h>

CUSTOM_ACTION(CloseTasksApp)
{
	log(LOGMSG_STANDARD, "Initialized.");

	static constexpr WCHAR kClass[] = L"TasksWindow[mBITS]";
	static constexpr WCHAR kMessage[] = L"Tasks-uninstall-2E10A6B5-33CF-422C-810C-8B18DFC03C6E";

	uintmax_t counter = 0;
	HWND handle = FindWindowW(kClass, nullptr);
	while (handle) {
		++counter;
		log(LOGMSG_STANDARD, "Found window at %p, sending message....", handle);
		static UINT WM_UNINSTALL = RegisterWindowMessageW(kMessage);
		SendMessageW(handle, WM_UNINSTALL, 0, 0);
		log(LOGMSG_STANDARD, "    message sent and delivered", handle);
		handle = FindWindowW(kClass, nullptr);
	}
	if (!counter)
		log(LOGMSG_STANDARD, "Closed no apps");

	return S_OK;
}
