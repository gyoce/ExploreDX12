#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <cstring>
#include <chrono>
#include <iomanip>

#include <windows.h>

namespace Logs
{
    static void AddConsole()
    {
        AllocConsole();

        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        std::ios::sync_with_stdio();

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE)
        {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode))
            {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
    }

    namespace Internal
    {
        template<typename CharT>
        struct LogContext
        {
            const CharT* Format = nullptr;
            int FormatSize = 0;
            int Index = 0;
            std::basic_stringstream<CharT> Stream{};
        };

        inline std::string GetLocalTimeStr()
        {
            const auto now = std::chrono::system_clock::now();
            time_t time = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &time);
            std::stringstream ss{};
            ss << std::put_time(&tm, "%H:%M:%S");
            return ss.str();
        }

        inline std::string ToUtf8(const std::wstring& wstr)
        {
            if (wstr.empty())
                return {};
            int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
            std::string result(sizeNeeded, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), sizeNeeded, nullptr, nullptr);
            return result;
        }

        template<typename CharT, typename T, class... Args>
        void FormatLog(LogContext<CharT>& context, T&& current, Args&&... args)
        {
            while (context.Index < context.FormatSize)
            {
                if (context.Format[context.Index] == '{' && context.FormatSize > context.Index + 1 && context.Format[context.Index + 1] == '}')
                {
                    context.Index = context.Index + 2;
                    context.Stream << std::forward<T>(current);
                    FormatLog(context, std::forward<Args>(args)...);
                } 
                else
                {
                    context.Stream << context.Format[context.Index];
                }
                context.Index++;
            }
        }

        template<typename CharT>
        inline void FormatLog(LogContext<CharT>& context)
        {
            while (context.Index < context.FormatSize)
            {
                context.Stream << context.Format[context.Index];
                context.Index++;
            }
        }

        template<typename CharT>
        inline void Output(const std::basic_string<CharT>& msg, bool error = false)
        {
            std::string utf8msg;
            if constexpr (std::is_same_v<CharT, wchar_t>)
                utf8msg = ToUtf8(msg);
            else
                utf8msg = msg;

            if (error)
                std::cerr << utf8msg << "\n";
            else
                std::cout << utf8msg << "\n";
        }

        inline LogContext<char> GetNewLogContext(const std::string& format)
        {
            return
            {
                .Format = format.c_str(),
                .FormatSize = static_cast<int>(format.size())
            };
        }

        inline LogContext<wchar_t> GetNewLogContext(const std::wstring& format)
        {
            return
            {
                .Format = format.c_str(),
                .FormatSize = static_cast<int>(format.size())
            };
        }

        inline std::string Green(const std::string& text)
        {
            return "\033[32m" + text + "\033[0m";
        }

        inline std::string BoldAndRed(const std::string& text)
        {
            return "\033[1;31m" + text + "\033[0m";
        }
    } // namespace Internal

    template <typename CharT, class... Args>
    void Message(const std::basic_string<CharT>& format, Args&&... args)
    {
#ifndef NDEBUG
        Internal::LogContext<CharT> context = Internal::GetNewLogContext(format);
        std::string localTimeStr = Internal::GetLocalTimeStr();

        if constexpr (std::is_same_v<CharT, wchar_t>)
            context.Stream << L"LOG:   [" << std::wstring(localTimeStr.begin(), localTimeStr.end()) << L"]: ";
        else
            context.Stream << "LOG:   [" << localTimeStr << "]: ";

        if constexpr (sizeof...(args) > 0)
            Internal::FormatLog(context, std::forward<Args>(args)...);
        else
            Internal::FormatLog(context);

        if constexpr (std::is_same_v<CharT, wchar_t>)
            Internal::Output(Internal::Green(Internal::ToUtf8(context.Stream.str())));
        else
            Internal::Output(Internal::Green(context.Stream.str()));
#endif
    }

    template <typename CharT, class... Args>
    void Message(const CharT* format, Args&&... args)
    {
        Message(std::basic_string<CharT>(format), std::forward<Args>(args)...);
    }

    template <typename CharT, class... Args>
    void Error(const std::basic_string<CharT>& format, Args&&... args)
    {
#ifndef NDEBUG
        Internal::LogContext<CharT> context = Internal::GetNewLogContext(format);
        std::string localTimeStr = Internal::GetLocalTimeStr();

        if constexpr (std::is_same_v<CharT, wchar_t>)
            context.Stream << L"ERROR: [" << std::wstring(localTimeStr.begin(), localTimeStr.end()) << L"]: ";
        else
            context.Stream << "ERROR: [" << localTimeStr << "]: ";

        if constexpr (sizeof...(args) > 0)
            Internal::FormatLog(context, std::forward<Args>(args)...);
        else
            Internal::FormatLog(context);

        if constexpr (std::is_same_v<CharT, wchar_t>)
            Internal::Output(Internal::BoldAndRed(Internal::ToUtf8(context.Stream.str())), true);
        else
            Internal::Output(Internal::BoldAndRed(context.Stream.str()), true);
#endif
    }

    template <typename CharT, class... Args>
    void Error(const CharT* format, Args&&... args)
    {
        Error(std::basic_string<CharT>(format), std::forward<Args>(args)...);
    }

} // namespace Logs