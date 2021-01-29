#include "Logger.h"
#pragma warning(push, 0)
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#pragma warning(pop)

//std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;

void Logger::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");
	//s_CoreLogger = spdlog::stdout_color_mt("Replicant");
	//s_CoreLogger->set_level(spdlog::level::trace);

	//s_ClientLogger = spdlog::stdout_color_mt("Log");
	s_ClientLogger = spdlog::rotating_logger_mt("KIP-8 Core", "logs/kip8_log", 1048576 * 5, 3);
	s_ClientLogger->set_level(spdlog::level::warn);
}
