#pragma once

#include <string>
#include <vector>
#include <cstdint>

class Log{
public:
    enum class Level : uint8_t {
        Info,
        Warn,
        Error
    };

    struct Entry{
        Level level;
        std::string message;
        uint32_t frame;
    };

    static void init(uint32_t maxEntries = 500);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);

    static const std::vector<Entry>& entries();
    static void clear();
    static void setFrame(uint32_t frame);
    static void drawPanel();

private:
    static void push(Level level, const std::string& msg);
};
