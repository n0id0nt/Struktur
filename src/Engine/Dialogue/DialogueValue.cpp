#include "DialogueValue.h"

std::string Struktur::Dialogue::DialogueValue::AsString() const
{
    return std::visit([](auto&& arg) -> std::string
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::string>)
        {
            return arg;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            return std::to_string(arg);
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return arg ? "true" : "false";
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return std::to_string(arg);
        }
    }, m_value);
}

int Struktur::Dialogue::DialogueValue::AsInt() const
{
    return std::visit([](auto&& arg) -> int
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int>)
        {
            return arg;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return arg ? 1 : 0;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return static_cast<int>(arg);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            try { return std::stoi(arg); }
            catch (...) { return 0; }
        }
    }, m_value);
}

bool Struktur::Dialogue::DialogueValue::AsBool() const
{
    return std::visit([](auto&& arg) -> bool
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, bool>)
        {
            return arg;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            return arg != 0;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return arg != 0.0;
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return arg == "true" || arg == "1";
        }
    }, m_value);
}

double Struktur::Dialogue::DialogueValue::AsDouble() const
{
    return std::visit([](auto&& arg) -> double
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, double>)
        {
            return arg;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            return static_cast<double>(arg);
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return arg ? 1.0 : 0.0;
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            try { return std::stod(arg); }
            catch (...) { return 0.0; }
        }
    }, m_value);
}
