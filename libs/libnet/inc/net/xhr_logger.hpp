/*
 * Copyright (C) 2014 midnightBITS
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
#include <map>
#include <string>


namespace net { namespace http { namespace client {
	enum class trace {
		unknown,
		data_in,
		data_out,
		ssl_in,
		ssl_out
	};
	struct LoggingClient {
		virtual ~LoggingClient() {}
		virtual void onStart(const std::string& url) = 0;
		virtual void onFinalLocation(const std::string& location) = 0;
		virtual void onDebug(const char *data) = 0;
		virtual void onRequestHeaders(const char* data, size_t length) = 0;
		virtual void onResponse(const std::string& reason, int http_status, const std::map<std::string, std::string>& headers) = 0;
		virtual void onTrace(trace mode, const char *data, size_t size) = 0;
		virtual void onStop(bool success) = 0;
	};

	using LoggingClientPtr = std::shared_ptr<LoggingClient>;
}}}
