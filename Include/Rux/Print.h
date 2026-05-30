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
    // Plain string overloads (no formatting)
    inline void print(const char* str) {
        fputs(str, stdout);
        fflush(stdout);
    }

    inline void print(FILE* file, const char* str) {
        fputs(str, file);
        fflush(file);
    }

    inline void print(std::ostream& os, const char* str) {
        os << str;
        os.flush();
    }

    inline void println(const char* str) {
        fputs(str, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }

    inline void println(FILE* file, const char* str) {
        fputs(str, file);
        fputc('\n', file);
        fflush(file);
    }

    inline void println(std::ostream& os, const char* str) {
        os << str << '\n';
        os.flush();
    }

    // Formatting overloads
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
// Unix-like: provide our own implementations for consistency across platforms
#include <cstdio>
namespace Rux {
    // Plain string overloads (no formatting)
    inline void print(const char* str) {
        fputs(str, stdout);
        fflush(stdout);
    }

    inline void print(FILE* file, const char* str) {
        fputs(str, file);
        fflush(file);
    }

    inline void print(std::ostream& os, const char* str) {
        os << str;
        os.flush();
    }

    inline void println(const char* str) {
        fputs(str, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }

    inline void println(FILE* file, const char* str) {
        fputs(str, file);
        fputc('\n', file);
        fflush(file);
    }

    inline void println(std::ostream& os, const char* str) {
        os << str << '\n';
        os.flush();
    }

    // Formatting overloads
    template<typename... Args>
    inline void print(std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...);
        fputs(result.c_str(), stdout);
        fflush(stdout);
    }

    template<typename... Args>
    inline void print(FILE* file, std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...);
        fprintf(file, "%s", result.c_str());
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
        fprintf(file, "%s", result.c_str());
        fflush(file);
    }

    template<typename... Args>
    inline void println(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
        std::string result = std::format(fmt, std::forward<Args>(args)...) + '\n';
        os << result;
        os.flush();
    }
}
#endif
