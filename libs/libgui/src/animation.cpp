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
#include <gui/animation.hpp>

namespace ani {
	struct actor {
		actor(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration, uint32_t counts);
		~actor();

		const std::shared_ptr<animation>& target() const;
		void start();
		bool step();
	private:
		using clock = std::chrono::high_resolution_clock;
		std::shared_ptr<animation> m_target;
		clock::duration m_duration;
		uint32_t m_counts;
		clock::time_point m_startTime;
	};
	actor::actor(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration, uint32_t counts)
		: m_target(target)
		, m_duration(duration)
		, m_counts(counts)
	{
	}

	actor::~actor()
	{
		m_target->shutdown();
	}

	const std::shared_ptr<animation>& actor::target() const
	{
		return m_target;
	}

	void actor::start()
	{
		m_target->init();
		m_startTime = clock::now();
	}

	bool actor::step()
	{
		auto now = clock::now();
		auto running = now - m_startTime;

		if (m_counts && (running / m_duration > m_counts))
			return false;

		auto timeframe = running % m_duration;
		auto frame = 1000 * timeframe / m_duration;
		m_target->step((uint32_t)frame);
	}

	void scene::animate(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration, uint32_t counts)
	{
		if (!target)
			return;

		remove(target);

		std::lock_guard<std::mutex> guard(m_mtx);
		auto animator = std::make_shared<actor>(target, duration, counts);
		animator->start();
		m_actors.push_back(std::move(animator));
	}

	void scene::animate(const std::shared_ptr<animation>& target, std::chrono::milliseconds duration)
	{
		return animate(target, duration, 0);
	}

	void scene::remove(const std::shared_ptr<animation>& target)
	{
		if (!target)
			return;

		std::lock_guard<std::mutex> guard(m_mtx);

		auto it = std::find_if(std::begin(m_actors), std::end(m_actors), [&](const std::shared_ptr<actor>& ptr) {
			return ptr->target() == target;
		});

		if (it == std::end(m_actors))
			return;

		m_actors.erase(it);
		return;
	}

	bool scene::step()
	{
		std::lock_guard<std::mutex> guard(m_mtx);
		auto it = std::begin(m_actors), end = std::end(m_actors);

		while (it != end) {
			auto& actor = *it;

			if (!actor->step()) {
				it = m_actors.erase(it);
				end = std::end(m_actors);
			} else {
				++it;
			}
		}

		return !m_actors.empty();
	}
}
