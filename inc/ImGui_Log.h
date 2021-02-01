#pragma once
#include <iostream>
#pragma warning(push, 0)
#include "imguial_term.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#pragma warning(pop)

template<typename Mutex>
class ImGui_Log : public spdlog::sinks::base_sink <Mutex>
{
public:
    ImGui_Log() : imguial_buffered_log_instance(nullptr) {}
    ImGui_Log(std::shared_ptr<ImGuiAl::BufferedLog<16384>> log) : imguial_buffered_log_instance(log) {}
private:
    std::shared_ptr<ImGuiAl::BufferedLog<16384>> imguial_buffered_log_instance;
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {

        // log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
        // msg.raw contains pre formatted log

        // If needed (very likely but not mandatory), the sink formats the message before sending it to its final destination:
        spdlog::memory_buf_t formatted;
        base_sink<Mutex>::formatter_->format(msg, formatted);
        //std::cout << fmt::to_string(formatted);

        switch (msg.level)
        {
        case(spdlog::level::trace):
            imguial_buffered_log_instance->trace(fmt::to_string(formatted).c_str());
            break;
        case(spdlog::level::debug):
            imguial_buffered_log_instance->debug(fmt::to_string(formatted).c_str());
            break;
        case(spdlog::level::info):
            imguial_buffered_log_instance->info(fmt::to_string(formatted).c_str());
            break;
        case(spdlog::level::warn):
            imguial_buffered_log_instance->warning(fmt::to_string(formatted).c_str());
            break;
        case(spdlog::level::err):
        case(spdlog::level::critical):
            imguial_buffered_log_instance->error(fmt::to_string(formatted).c_str());
            break;
        }
    }

    void flush_() override
    {
        std::cout << std::flush;
    }
};

#include "spdlog/details/null_mutex.h"
#include <mutex>
using imgui_log_sink_mt = ImGui_Log<std::mutex>;
using imgui_log_sink_st = ImGui_Log<spdlog::details::null_mutex>;