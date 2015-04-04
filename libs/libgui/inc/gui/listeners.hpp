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

#include <memory>
#include <vector>
#include <functional>
#include <mutex>

template <typename T, typename F>
inline void synchronize(T& mtx, F fn)
{
	std::lock_guard<T> guard(mtx);
	fn();
}

template <typename T, typename Final>
class listeners {
	std::mutex m_guard;
	std::vector<std::weak_ptr<T>> m_listeners;

protected:
	template <typename Pred>
	void emit(Pred cb)
	{
		std::lock_guard<std::mutex> lock{ m_guard };
		for (auto& item : m_listeners) {
			auto locked = item.lock();
			if (!locked)
				continue;

			cb(locked.get());
		}
	}

public:
	void registerListener(const std::shared_ptr<T>& listener)
	{
		unregisterListener(listener);

		{
			std::lock_guard<std::mutex> lock{ m_guard };
			m_listeners.push_back(listener);
		}

		static_cast<Final*>(this)->onListenerAdded(listener);
	}

	void unregisterListener(const std::shared_ptr<T>& listener)
	{
		static_cast<Final*>(this)->onListenerRemoved(listener);

		std::lock_guard<std::mutex> lock{ m_guard };

		auto it = m_listeners.begin();
		auto end = m_listeners.end();
		for (; it != end; ++it) {
			auto locked = it->lock();
			if (locked == listener) {
				m_listeners.erase(it);
				break;
			}
		}
	}

	void onListenerAdded(const std::shared_ptr<T>& /*listener*/) {}
	void onListenerRemoved(const std::shared_ptr<T>& /*listener*/) {}
};
