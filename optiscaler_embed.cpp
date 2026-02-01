#include "optiscaler_bypass.h"
#include "optiscaler_embed.h"
#include "logger.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/callback_sink.h"
#include "Config.h"

#include <typeinfo>
#include <string_view>

extern "C" BOOL WINAPI optisc_original_DllMain(HMODULE, DWORD, LPVOID);
HMODULE ModuleFromAddress(const void*);

static constexpr const char* kEOL = "\r\n";

static void set_sink_pattern_with_eol(const spdlog::sink_ptr& s, const std::string& pattern)
{
    s->set_formatter(std::make_unique<spdlog::pattern_formatter>(
        pattern, spdlog::pattern_time_type::local, std::string{ kEOL }));
}

// Helper to detect sink type by checking type name
static bool is_console_sink(const spdlog::sink_ptr& s)
{
    if (!s) return false;

    // Get the actual type name and check if it contains color/console indicators
    const char* name = typeid(*s).name();
    std::string_view sv(name);

    // MSVC mangles names, look for substrings
    return sv.find("color") != std::string_view::npos ||
        sv.find("stdout") != std::string_view::npos ||
        sv.find("stderr") != std::string_view::npos;
}

static bool is_file_sink(const spdlog::sink_ptr& s)
{
    if (!s) return false;

    const char* name = typeid(*s).name();
    std::string_view sv(name);

    return sv.find("basic_file_sink") != std::string_view::npos ||
        sv.find("file_sink") != std::string_view::npos;
}

static bool is_callback_sink(const spdlog::sink_ptr& s)
{
    if (!s) return false;

    const char* name = typeid(*s).name();
    std::string_view sv(name);

    return sv.find("callback_sink") != std::string_view::npos;
}

bool OptiScaler_Init(HMODULE hOpti, const OptiScalerConfig* cfg)
{
    dllModule = hOpti;
    const bool spoof = cfg && cfg->spoof_as_enabler;
    (void)spoof;

    PrepareLogger();

    Config::Instance()->LogToConsole.set_volatile_value(false);
    Config::Instance()->LogToFile.set_volatile_value(false);
    Config::Instance()->LogToNGX.set_volatile_value(true);

    auto logger = spdlog::default_logger();
    if (!logger)
        return true;

    std::vector<spdlog::sink_ptr> fixed_sinks;
    fixed_sinks.reserve(logger->sinks().size());

    for (auto& s : logger->sinks())
    {
        if (is_console_sink(s))
        {
            set_sink_pattern_with_eol(s, "[%a %b %d %H:%M:%S] [tid %t] [%^%l%$] [OPTI] %v");
            fixed_sinks.push_back(s);
            continue;
        }

        if (is_file_sink(s))
        {
            set_sink_pattern_with_eol(s, "[%H:%M:%S.%f] [%L] %v");
            fixed_sinks.push_back(s);
            continue;
        }

        if (is_callback_sink(s))
        {
            auto cb_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
                [](const spdlog::details::log_msg& msg)
                {

                });
            cb_sink->set_level(spdlog::level::trace);

            cb_sink->set_formatter(std::make_unique<spdlog::pattern_formatter>(
                "[%H:%M:%S.%f] [%L] %v", spdlog::pattern_time_type::local, std::string{ kEOL }));
            fixed_sinks.push_back(cb_sink);
            continue;
        }

        fixed_sinks.push_back(s);
    }

    logger->sinks() = fixed_sinks;

    logger->flush_on(spdlog::level::trace);
    logger->set_level(spdlog::level::trace);

    BOOL ok = optisc_original_DllMain(hOpti, DLL_PROCESS_ATTACH, nullptr);

    Config::Instance()->LogToFile.set_volatile_value(false);

    if (ok == FALSE)
        return false;

    return true;
}