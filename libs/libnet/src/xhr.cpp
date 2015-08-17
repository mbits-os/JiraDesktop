/*
 * Copyright (C) 2013 midnightBITS
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
#include <net/xhr.hpp>
#include <string.h>

#include <string>
#include "curl_http.hpp"
#include <cctype>

namespace std
{
	static std::string tolower(std::string in) {
		for (auto&& c : in)
			c = (char)std::tolower((unsigned char)c);
		return in;
	}
}

namespace net { namespace http {
	namespace impl
	{
		struct ContentData
		{
			char* content;
			size_t content_length;
			size_t capacity;
			ContentData()
				: content(nullptr)
				, content_length(0)
				, capacity(0)
			{
			}
			~ContentData()
			{
				delete [] content;
			}

			bool grow(size_t newsize)
			{
				if (capacity >= newsize)
					return true;

				auto copy_capacity = capacity;
				if (!copy_capacity)
					copy_capacity = 16;

				while (copy_capacity < newsize)
					copy_capacity <<= 1;

				auto tmp = new (std::nothrow) char[copy_capacity];
				if (!tmp)
					return false;

				if (content)
					memcpy(tmp, content, content_length);

				delete [] content;
				content = tmp;
				capacity = copy_capacity;
				return true;
			}

			bool append(const void* data, size_t len)
			{
				if (data && len)
				{
					if (!grow(content_length + len))
						return false;

					memcpy(content + content_length, data, len);
					content_length += len;
				}
				return true;
			}

			void clear()
			{
				delete [] content;
				content = nullptr;
				content_length = 0;
				capacity = 0;
			}
		};

		class XmlHttpRequest
			: public client::XmlHttpRequest
			, public http::HttpCallback
			, public std::enable_shared_from_this<XmlHttpRequest>
		{
			ONREADYSTATECHANGE handler;
			ONPROGRESS progress;

			client::HTTP_METHOD http_method;
			std::string url;
			std::string userAgent;
			bool async;
			Headers request_headers;
			READY_STATE ready_state;
			ContentData body;

			int http_status;
			std::string reason;
			Headers response_headers;
			ContentData response;

			bool send_flag, done_flag;
			HttpEndpointPtr m_endpoint;
			client::LoggingClientPtr logger;

			bool m_followRedirects;
			size_t m_redirects;

			bool m_wasRedirected;
			std::string m_finalLocation;

			bool m_lengthCalculable;
			uint64_t m_contentLength;
			uint64_t m_loadedLength;

			std::string m_error;

			struct Credentials : http::HttpCredentials
			{
				client::CredentialProviderPtr m_provider;
				std::string username() override 
				{
					return m_provider->getUsername();
				}

				std::string password() override
				{
					return m_provider->getPassword();
				}
			} m_impl;
			http::HttpCredentials* m_credentials;

			void onReadyStateChange()
			{
				if (handler)
					handler(this);
			}

			void onProgress(uint64_t justLoaded) {
				m_loadedLength += justLoaded;
				if (progress)
					progress(m_lengthCalculable, m_contentLength, m_loadedLength);
			}

			void clear_response()
			{
				http_status = 0;
				reason.clear();
				response_headers.clear();
				response.clear();
				m_wasRedirected = false;
				m_finalLocation.clear();
				m_error.clear();
			}
		public:

			XmlHttpRequest(const std::string& userAgent)
				: http_method(client::HTTP_GET)
				, userAgent(userAgent)
				, async(true)
				, ready_state(UNSENT)
				, http_status(0)
				, send_flag(false)
				, done_flag(false)
				, m_followRedirects(true)
				, m_redirects(10)
				, m_wasRedirected(false)
				, m_credentials(nullptr)
			{
			}

			~XmlHttpRequest()
			{
				if (m_endpoint)
					m_endpoint->releaseEndpoint();
			}

			void onreadystatechange(ONREADYSTATECHANGE) override;
			void onprogress(ONPROGRESS) override;
			READY_STATE getReadyState() const override;

			void open(client::HTTP_METHOD, const std::string&, bool = true) override;
			void setRequestHeader(const std::string&, const std::string&) override;

			void setBody(const void*, size_t) override;
			void send() override;
			void abort() override;

			int getStatus() const override;
			std::string getStatusText() const override;
			std::string getResponseHeader(const std::string&) const override;
			std::map<std::string, std::string> getResponseHeaders() const override;
			size_t getResponseTextLength() const override;
			const char* getResponseText() const override;

			bool wasRedirected() const override;
			const std::string getFinalLocation() const override;

			void setLogging(const client::LoggingClientPtr& logger) override;
			void setMaxRedirects(size_t) override;
			void setShouldFollowLocation(bool) override;
			void setCredentials(const client::CredentialProviderPtr& provider) override;
			const std::string& getError() override;

			void onStart() override;
			void onError(const std::string& error) override;
			void onFinish() override;
			size_t onData(const void*, size_t) override;
			void onFinalLocation(const std::string&) override;
			void onHeaders(const std::string&, int, const Headers&) override;

			void appendHeaders() override;
			std::string getUrl() override;
			std::string getUserAgent() override;
			void* getContent(size_t&) override;
			const client::LoggingClientPtr& getLogger() const override;
			bool shouldFollowLocation() override;
			long getMaxRedirs() override;
			HttpCredentials* getCredentials() override;
		};

		void XmlHttpRequest::onreadystatechange(ONREADYSTATECHANGE fn)
		{
			//Synchronize on(*this);
			handler = fn;
			if (ready_state != UNSENT)
				onReadyStateChange();
		}

		void XmlHttpRequest::onprogress(ONPROGRESS fn)
		{
			progress = fn;
		}

		http::client::XmlHttpRequest::READY_STATE XmlHttpRequest::getReadyState() const
		{
			return ready_state;
		}

		void XmlHttpRequest::open(client::HTTP_METHOD method, const std::string& url_, bool async_)
		{
			abort();

			//Synchronize on(*this);

			send_flag = false;
			clear_response();
			request_headers.clear();

			http_method = method;
			url = url_;
			async = async_;

			m_lengthCalculable = false;
			m_contentLength = 0;
			m_loadedLength = 0;

			ready_state = OPENED;
			onReadyStateChange();
		}

		void XmlHttpRequest::setRequestHeader(const std::string& header, const std::string& value)
		{
			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			auto _h = std::tolower(header);
			Headers::iterator _it = request_headers.find(_h);
			if (_it == request_headers.end())
			{
				if (_h == "accept-charset") return;
				if (_h == "accept-encoding") return;
				if (_h == "connection") return;
				if (_h == "content-length") return;
				if (_h == "cookie") return;
				if (_h == "cookie2") return;
				if (_h == "content-transfer-encoding") return;
				if (_h == "date") return;
				if (_h == "expect") return;
				if (_h == "host") return;
				if (_h == "keep-alive") return;
				if (_h == "referer") return;
				if (_h == "te") return;
				if (_h == "trailer") return;
				if (_h == "transfer-encoding") return;
				if (_h == "upgrade") return;
				if (_h == "user-agent") return;
				if (_h == "via") return;
				if (_h.substr(0, 6) == "proxy-") return;
				if (_h.substr(0, 4) == "sec-") return;
				request_headers[_h] = header +": " + value;
			}
			else
			{
				_it->second += ", " + value;
			}
		}

		void XmlHttpRequest::setBody(const void* body_, size_t length)
		{
			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			body.clear();

			if (http_method != client::HTTP_POST) return;

			body.append(body_, length);
		}

		void XmlHttpRequest::send()
		{
			abort();

			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			send_flag = true;
			done_flag = false;
			if (!m_endpoint)
				m_endpoint = http::GetEndpoint(shared_from_this());
			if (m_endpoint)
				m_endpoint->send(async);
		}

		void XmlHttpRequest::abort()
		{
			if (m_endpoint)
				m_endpoint->abort();
		}

		int XmlHttpRequest::getStatus() const
		{
			return http_status;
		}

		std::string XmlHttpRequest::getStatusText() const
		{
			return reason;
		}

		std::string XmlHttpRequest::getResponseHeader(const std::string& name) const
		{
			Headers::const_iterator _it = response_headers.find(std::tolower(name));
			if (_it == response_headers.end()) return std::string();
			return _it->second;
		}

		std::map<std::string, std::string> XmlHttpRequest::getResponseHeaders() const
		{
			return response_headers;
		}

		size_t XmlHttpRequest::getResponseTextLength() const
		{
			return response.content_length;
		}

		const char* XmlHttpRequest::getResponseText() const
		{
			return (const char*)response.content;
		}

		bool XmlHttpRequest::wasRedirected() const { return m_wasRedirected; }
		const std::string XmlHttpRequest::getFinalLocation() const { return m_finalLocation; }

		void XmlHttpRequest::setLogging(const client::LoggingClientPtr& logger_)
		{
			logger = logger_;
		}

		void XmlHttpRequest::setMaxRedirects(size_t redirects)
		{
			m_redirects = redirects;
		}

		void XmlHttpRequest::setShouldFollowLocation(bool follow)
		{
			m_followRedirects = follow;
		}

		void XmlHttpRequest::setCredentials(const client::CredentialProviderPtr& provider)
		{
			m_impl.m_provider = provider;
			m_credentials = provider ? &m_impl : nullptr;
		}

		const std::string& XmlHttpRequest::getError()
		{
			return m_error;
		}

		void XmlHttpRequest::onStart()
		{
			if (!body.content || !body.content_length)
				http_method = client::HTTP_GET;

			//onReadyStateChange();
		}

		void XmlHttpRequest::onError(const std::string& error)
		{
			m_error = error;
			ready_state = DONE;
			done_flag = true;
			onReadyStateChange();
		}

		void XmlHttpRequest::onFinish()
		{
			ready_state = DONE;
			onReadyStateChange();
		}

		size_t XmlHttpRequest::onData(const void* data, size_t count)
		{
			//Synchronize on(*this);

			size_t ret = response.append(data, count) ? count : 0;
			if (ret)
			{
				onProgress(ret);

				if (ready_state == HEADERS_RECEIVED) ready_state = LOADING;
				if (ready_state == LOADING)          onReadyStateChange();
			}

			return ret;
		}

		void XmlHttpRequest::onFinalLocation(const std::string& location)
		{
			m_wasRedirected = true;
			m_finalLocation = location;
		}

		void XmlHttpRequest::onHeaders(const std::string& reason_, int http_status_, const Headers& headers)
		{
			//Synchronize on(*this);
			http_status = http_status_;
			reason = reason_;

			for (auto&& item : headers)
				response_headers[std::tolower(item.first)] = item.second;

			m_contentLength = 0;
			auto length = getResponseHeader("content-length");
			if (!length.empty())
				m_contentLength = std::stoull(length);

			m_lengthCalculable = m_contentLength > 0;

			ready_state = HEADERS_RECEIVED;
			onProgress(0);
			onReadyStateChange();
		}

		void XmlHttpRequest::appendHeaders()
		{
			if (m_endpoint)
			{
				for (auto&& pair : request_headers)
					m_endpoint->appendHeader(pair.second);
			}
		}

		std::string XmlHttpRequest::getUrl() { return url; }
		std::string XmlHttpRequest::getUserAgent() { return userAgent; }
		void* XmlHttpRequest::getContent(size_t& length) { if (http_method == client::HTTP_POST) { length = body.content_length; return body.content; } return nullptr; }
		const client::LoggingClientPtr& XmlHttpRequest::getLogger() const { return logger; }
		bool XmlHttpRequest::shouldFollowLocation() { return m_followRedirects; }
		long XmlHttpRequest::getMaxRedirs() { return m_redirects; }
		HttpCredentials* XmlHttpRequest::getCredentials() { return m_credentials; }
	} // http::impl


	namespace client {
		std::string s_program_version;

		void set_program_client_info(const char* ver)
		{
			if (!ver || !*ver) {
				s_program_version.clear();
				return;
			}
			s_program_version = " ";
			s_program_version += ver;
		}

		std::string os_client_info();
		std::string http_client_info();

		static std::string getUserAgent()
		{
			static std::string userAgent = "Mozilla/5.0 (" + os_client_info() + ")" + s_program_version + " XHRLoader/1.0 " + http_client_info();
			return userAgent;
		}

		XmlHttpRequestPtr create()
		{
			try {
				return std::make_shared<impl::XmlHttpRequest>(getUserAgent());
			} catch (std::bad_alloc) {
				return nullptr;
			}
		}
	}
}}
