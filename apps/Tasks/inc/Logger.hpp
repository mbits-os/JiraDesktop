#pragma once

#include <memory>
#include <string>

struct Logger {
	virtual ~Logger() {};
	virtual void print(const std::string&) = 0;
};


std::shared_ptr<Logger> open_log(const std::string& path);