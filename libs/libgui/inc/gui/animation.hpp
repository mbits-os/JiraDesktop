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

#include <chrono>
#include <memory>
#include <list>
#include <mutex>

using namespace std::chrono_literals;

namespace ani {
	struct animation {
		virtual ~animation() {}
		virtual void init() = 0;
		virtual void step(uint32_t step) = 0;
		virtual void shutdown() = 0;
	};

	struct actor;
	struct scene {
		void animate(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration, uint32_t counts);
		void animate(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration);
		void remove(const std::shared_ptr<animation>& target);
		bool step();
	private:
		std::mutex m_mtx;
		std::list<std::shared_ptr<actor>> m_actors;
	};
};