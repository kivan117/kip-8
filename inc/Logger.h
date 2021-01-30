#pragma once
#include <memory>
#pragma warning(push, 0)
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/dist_sink.h"
#pragma warning(pop)

class Logger
{
public:
	static void Init();

	//inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
	inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
private:
	//static std::shared_ptr<spdlog::logger> s_CoreLogger;
	static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

//// Core log macros
//#define RP_CORE_TRACE(...)    ::Replicant::Log::GetCoreLogger()->trace(__VA_ARGS__)
//#define RP_CORE_INFO(...)     ::Replicant::Log::GetCoreLogger()->info(__VA_ARGS__)
//#define RP_CORE_WARN(...)     ::Replicant::Log::GetCoreLogger()->warn(__VA_ARGS__)
//#define RP_CORE_ERROR(...)    ::Replicant::Log::GetCoreLogger()->error(__VA_ARGS__)
//#define RP_CORE_FATAL(...)    ::Replicant::Log::GetCoreLogger()->fatal(__VA_ARGS__)
//
//// Client log macros
#define LOG_TRACE(...)	      ::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)	      ::Logger::GetClientLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)	      ::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)	      ::Logger::GetClientLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...)	      ::Logger::GetClientLogger()->fatal(__VA_ARGS__)

