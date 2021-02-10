#pragma once
#include <memory>
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/dist_sink.h>
#pragma warning(pop)

class Logger
{
public:
	static void Init();

	inline static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }
private:
	static std::shared_ptr<spdlog::logger> s_Logger;
};

#define LOG_TRACE(...)	      ::Logger::GetLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)	      ::Logger::GetLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...)	      ::Logger::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)	      ::Logger::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)	      ::Logger::GetLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...)	      ::Logger::GetLogger()->fatal(__VA_ARGS__)

