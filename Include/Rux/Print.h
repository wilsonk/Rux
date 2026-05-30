/*
    Rux Compiler
    Copyright © 2026 Rux Contributors
    Licensed under the MIT License
*/

#pragma once

#include <iostream>
#include <format>
#include <string>

#ifdef _WIN32
// Windows: std::print has compatibility issues, use std::format + fputs
#include <cstdio>
namespace Rux {
    template<typename... Args>
    inline void print(std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...);
        fputs(result.c_str(), stdout);
        fflush(stdout);
    }

    template<typename... Args>
    inline void print(FILE* file, std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...);
        fputs(result.c_str(), file);
        fflush(file);
    }

    template<typename... Args>
    inline void print(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...);
        os << result;
        os.flush();
    }

    template<typename... Args>
    inline void println(std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...) + '\n';
        fputs(result.c_str(), stdout);
        fflush(stdout);
    }

    template<typename... Args>
    inline void println(FILE* file, std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...) + '\n';
        fputs(result.c_str(), file);
        fflush(file);
    }

    template<typename... Args>
    inline void println(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...) + '\n';
        os << result;
        os.flush();
    }
}
#else
// Unix-like: use std::print/std::println
namespace Rux {
    using std::print;
    using std::println;
}
#endif
