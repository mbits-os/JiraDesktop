#include "stdafx.h"
#include "XHRConstructor.h"
#include "AppSettings.h"
#include "Logger.hpp"
#include <cctype>

static std::string censor(const std::string& header)
{
	const char* to_censor[] = { "authorization" };
	for (auto head : to_censor) {
		auto len = strlen(head);
		if (header.size() <= len)
			continue;
		if (header[len] != ':' && !std::isspace((uint8_t)header[len]))
			continue;

		auto sub = header.substr(0, len);
		if (_stricmp(sub.c_str(), head))
			continue;

		return sub + ": [REDACTED]";
	}
	return header;
}

using trace_kind = net::http::client::trace;

class XhrLoggingClient : public net::http::client::LoggingClient {
	class HexPrinter {
		enum { width = 48 };
		trace_kind m_current = trace_kind::unknown;
		size_t m_ptr = 0;
		char m_line[width * 4 + 1] = "";

		void print(Logger* logger)
		{
			static const std::string headers[] = { "", "DATA< ", "DATA> ", "SSL< ", "SLL> " };
			logger->write(headers[(int)m_current]);
			logger->write(m_line);
			logger->write("\n");
		}
	public:
		trace_kind kind() const { return m_current; }
		void flush(Logger* logger, trace_kind next = trace_kind::unknown)
		{
			if (m_current != trace_kind::unknown && m_ptr) {
				auto ptr = m_line + m_ptr * 3;
				while (m_ptr < width) {
					*ptr++ = ' ';
					*ptr++ = ' ';
					*ptr++ = ' ';
					++m_ptr;
				}
				print(logger);
			}
			m_current = next;
			m_ptr = 0;
		}

		void append(Logger* logger, const char *data, size_t size)
		{
			static const char hex[] = "0123456789ABCDEF";
			for (auto end = data + size; data != end; ++data) {
				auto ptr = m_line + m_ptr * 3;
				ptr[0] = hex[((uint8_t)*data >> 4) & 0xF];
				ptr[1] = hex[((uint8_t)*data) & 0xF];
				ptr[2] = ' ';
				ptr = m_line + width * 3 + m_ptr;
				ptr[0] = std::isprint((uint8_t)*data) ? *data : '.';
				ptr[1] = 0;
				++m_ptr;

				if (m_ptr == width) {
					print(logger);
					m_ptr = 0;
				}
			}
		}
	};
	std::shared_ptr<Logger> m_log;
	std::string m_outHeaders;
	HexPrinter m_hex;
public:
	XhrLoggingClient(const std::shared_ptr<Logger>& log) : m_log { log }
	{
	}

	~XhrLoggingClient()
	{
		m_hex.flush(m_log.get());
	}

	void onStart(const std::string& url) override
	{
		m_hex.flush(m_log.get());
		m_log->write(">>>>>>>>>>>>>>>>>>>>>>>>>> " + url + "\n");
	}

	void onFinalLocation(const std::string& location) override
	{
		m_hex.flush(m_log.get());
		m_log->write("LOCATION: " + location + "\n");
	}

	void onDebug(const char* msg) override
	{
		m_hex.flush(m_log.get());
		m_log->write("* ");
		m_log->write(msg);
	}

	void onResponse(const std::string& reason, int http_status, const std::map<std::string, std::string>& headers) override
	{
		m_hex.flush(m_log.get());
		m_log->write("< " + std::to_string(http_status) + " " + reason + "\n");
		for (auto& pair : headers)
		m_log->write("< " + pair.first + ": " + pair.second + "\n");
		m_log->write("<\n");
	}

	void onRequestHeaders(const char* data, size_t length) override
	{
		m_hex.flush(m_log.get());

		m_outHeaders.append(data, data + length);
		auto len = m_outHeaders.size();
		if (len > 4 && !strncmp(m_outHeaders.c_str() + len - 4, "\r\n\r\n", 4)) {

			auto ret = m_outHeaders.find('\n');
			decltype(ret) start = 0;
			while (ret != std::string::npos) {
				m_log->write("> " + censor(m_outHeaders.substr(start, ret - start)) + "\n");
				start = ret + 1;
				ret = m_outHeaders.find('\n', start);
			}
			if (start < m_outHeaders.size())
				m_log->write("> " + censor(m_outHeaders.substr(start)) + "\n");

			m_outHeaders.clear();
		}
	}

	void onTrace(trace_kind kind, const char *data, size_t size) override
	{
		if (m_hex.kind() != kind)
			m_hex.flush(m_log.get(), kind);

		m_hex.append(m_log.get(), data, size);
	}

	void onStop(bool) override
	{
		m_log->write("<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
		m_log->flush();
	}
};

net::http::client::XmlHttpRequestPtr XHRConstructor::create()
{
	CAppSettings settings;
	static net::http::client::LoggingClientPtr holder;
	auto xhr = net::http::client::create();
	auto log = settings.connectionLog();
	if (log.empty()) {
		holder.reset();
	} else {
		auto logger = open_log(log, !settings.fastLog());
		if (logger) {
			holder = std::make_shared<XhrLoggingClient>(logger);
			xhr->setLogging(holder);
		}
	}
	return xhr;
}
