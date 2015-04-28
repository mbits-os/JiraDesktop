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

#include "pch.h"
#include <gui/win32_animation.hpp>

namespace ani { namespace win32 {

	scene::scene(UINT_PTR timer)
		: m_timer(timer)
	{
	}

	void scene::setWindow(HWND hWnd)
	{
		m_hWnd = hWnd;
	}

	void scene::animate(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration, uint32_t counts)
	{
		m_scene.animate(target, duration, counts);
		start();
	}

	void scene::animate(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration)
	{
		m_scene.animate(target, duration);
		start();
	}

	void scene::remove(const std::shared_ptr<animation>& target)
	{
		m_scene.remove(target);
	}

	bool scene::step()
	{
		auto ret = m_scene.step();
		if (!ret)
			stop();

		return ret;
	}

	LRESULT scene::handle_message(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, bool& handled)
	{
		if (uMsg == WM_TIMER && wParam == m_timer) {
			step();
			handled = true;
		}

		return 0;
	}

	void scene::start()
	{
		if (m_running)
			return;

		SetTimer(m_hWnd, m_timer, 100, nullptr);
		m_running = true;
	}

	void scene::stop()
	{
		if (!m_running)
			return;

		KillTimer(m_hWnd, m_timer);
		m_running = false;
	}
}}
