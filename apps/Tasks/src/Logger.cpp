#include "stdafx.h"
#include "Logger.hpp"

#include <map>
#include <mutex>
#include <experimental/filesystem>
#include <net/utf8.hpp>
#include <thread>
#include <sstream>
#include <vector>

namespace ex_fs = std::experimental::filesystem;

class FileWriter {
	std::mutex m;
	std::unique_ptr<FILE, decltype(&fclose)> m_file { nullptr, fclose };

	static std::string build_thread_id()
	{
		std::ostringstream o;
		o << std::this_thread::get_id();
		return o.str();
	}
	static const char* thread_id()
	{
		thread_local std::string id = build_thread_id();
		return id.c_str();
	}
public:
	FileWriter(const std::string& path)
	{
		std::error_code ec;
		auto p = ex_fs::canonical(utf::widen(path), ec);
		m_file.reset(_wfopen(p.native().c_str(), L"w"));
	}

	bool opened() const { return !!m_file; }

	void print(const std::string& s)
	{
		if (!m_file)
			return;

		//std::lock_guard<std::mutex> guard { m };
		fprintf(m_file.get(), "%s ", thread_id());

		auto ret = s.find('\r');
		decltype(ret) start = 0;
		while (ret != std::string::npos) {
			fwrite(s.c_str() + start, 1, ret - start, m_file.get());
			start = ret + 1;
			ret = s.find('\r', start);
		}
		if (start < s.size())
			fwrite(s.c_str() + start, 1, s.size() - start, m_file.get());
		fputc('\n', m_file.get());
	}

	void flush()
	{
		if (m_file)
			fflush(m_file.get());
	}

	void lock()
	{
		m.lock();
	}

	void unlock()
	{
		m.unlock();
	}
};

class LoggerManager {
	std::mutex m;
	std::map<std::string, std::weak_ptr<FileWriter>> m_writers;
public:
	std::shared_ptr<Logger> open(const std::string& path, bool caching);
};

class ThreadLogger : public Logger {
	std::shared_ptr<FileWriter> m_out;
	std::string m_buffer;

	void print(const std::string& in)
	{
		m_out->print(in);
	}
public:
	explicit ThreadLogger(const std::shared_ptr<FileWriter>& writer)
		: m_out { writer }
	{
	}

	~ThreadLogger()
	{
		if (!m_buffer.empty())
			print(m_buffer);
	}

	void write(const std::string& in) override
	{
		auto enter = in.find('\n');
		if (enter == std::string::npos) {
			m_buffer += in;
			return;
		}

		std::lock_guard<FileWriter> guard { *m_out };
		m_buffer += in.substr(0, enter);
		print(m_buffer);
		m_buffer.clear();

		++enter;
		auto next = in.find('\n', enter);
		while (next != std::string::npos) {
			print(in.substr(enter, next - enter));
			enter = next + 1;
			next = in.find('\n', enter);
		}
		m_out->flush();

		m_buffer = in.substr(enter);
	}

	void flush() override
	{
	}
};

class CachingThreadLogger : public Logger {
	std::shared_ptr<FileWriter> m_out;
	std::string m_buffer;
	std::vector<std::string> m_lines;

	void print(const std::string& in)
	{
		m_lines.push_back(in);
	}
public:
	explicit CachingThreadLogger(const std::shared_ptr<FileWriter>& writer)
		: m_out { writer }
	{
	}

	~CachingThreadLogger()
	{
		if (!m_buffer.empty())
			print(m_buffer);
		CachingThreadLogger::flush();
	}

	void write(const std::string& in) override
	{
		auto enter = in.find('\n');
		if (enter == std::string::npos) {
			m_buffer += in;
			return;
		}

		m_buffer += in.substr(0, enter);
		print(m_buffer);
		m_buffer.clear();

		++enter;
		auto next = in.find('\n', enter);
		while (next != std::string::npos) {
			print(in.substr(enter, next - enter));
			enter = next + 1;
			next = in.find('\n', enter);
		}

		m_buffer = in.substr(enter);
	}

	void flush() override
	{
		if (m_lines.empty())
			return;
		std::lock_guard<FileWriter> guard { *m_out };
		for (auto& ln : m_lines)
			m_out->print(ln);
		m_lines.clear();
		flush();
	}
};

std::shared_ptr<Logger> LoggerManager::open(const std::string& path, bool caching)
{
	std::error_code ec;
	ex_fs::path p = ex_fs::canonical(utf::widen(path), ec);
	if (ec)
		return { };

	std::lock_guard<std::mutex> guard { m };

	auto it = m_writers.find(p.string());
	std::shared_ptr<FileWriter> writer;
	if (it != m_writers.end())
		writer = it->second.lock();

	if (!writer) {
		writer = std::make_shared<FileWriter>(path);
		if (!writer->opened())
			return { };
		m_writers[p.string()] = writer;
	}

	if (caching)
		return std::make_shared<CachingThreadLogger>(writer);
	return std::make_shared<ThreadLogger>(writer);
}

std::shared_ptr<Logger> open_log(const std::string& path, bool caching)
{
	static LoggerManager loggers;
	return loggers.open(path, caching);
}
