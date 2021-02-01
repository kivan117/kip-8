#include "Logger.h"
#pragma warning(push, 0)
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#pragma warning(pop)

//std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;

void Logger::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");
	//s_CoreLogger = spdlog::stdout_color_mt("Replicant");
	//s_CoreLogger->set_level(spdlog::level::trace);

	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/kip8_log", 1048576 * 5, 3, false);
	s_ClientLogger = std::make_shared<spdlog::logger>("KIP-8 Core", file_sink);
	s_ClientLogger->sinks().push_back(console_sink);
	s_ClientLogger->set_level(spdlog::level::info);
}
