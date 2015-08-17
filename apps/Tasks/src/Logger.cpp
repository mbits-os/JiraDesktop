#include "stdafx.h"
#include "Logger.hpp"

class LoggerManager {
public:
	std::shared_ptr<Logger> open(const std::string& path);
};

std::shared_ptr<Logger> open_log(const std::string& path)
{
	static LoggerManager loggers;
	return loggers.open(path);
}

std::shared_ptr<Logger> LoggerManager::open(const std::string& path)
{
	return { };
}
