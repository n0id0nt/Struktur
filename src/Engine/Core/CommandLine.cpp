#include "CommandLine.h"

#include <algorithm>
#include <cctype>

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
    // clang-format off - windows.h must come before shellapi.h
	#include <windows.h>
	#include <shellapi.h>
    // clang-format on
	#pragma comment(lib, "shell32.lib")
#endif

namespace Struktur::Core::CommandLine
{
static std::vector<std::string> s_arguments;

#if defined(_WIN32)
// argv is in the ANSI code page on Windows; read the wide command line instead so paths stay exact (UTF-8).
static bool SetFromWideCommandLine()
{
	int count        = 0;
	LPWSTR* wideArgs = ::CommandLineToArgvW(::GetCommandLineW(), &count);
	if (wideArgs == nullptr)
	{
		return false;
	}
	for (int i = 1; i < count; ++i)
	{
		const int bytes = ::WideCharToMultiByte(CP_UTF8, 0, wideArgs[i], -1, nullptr, 0, nullptr, nullptr);
		if (bytes <= 1)
		{
			continue;
		}
		std::string utf8((size_t)bytes - 1, '?');
		::WideCharToMultiByte(CP_UTF8, 0, wideArgs[i], -1, utf8.data(), bytes, nullptr, nullptr);
		s_arguments.push_back(std::move(utf8));
	}
	::LocalFree(wideArgs);
	return true;
}
#endif

void Set(int argc, char* argv[])
{
	s_arguments.clear();
#if defined(_WIN32)
	if (SetFromWideCommandLine())
	{
		return;
	}
#endif
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] != nullptr)
		{
			s_arguments.emplace_back(argv[i]);
		}
	}
}

const std::vector<std::string>& GetArguments()
{
	return s_arguments;
}

bool HasFlag(const std::string& flag)
{
	return std::find(s_arguments.begin(), s_arguments.end(), flag) != s_arguments.end();
}

static std::string ToLower(std::string text)
{
	for (char& character : text)
	{
		character = (char)std::tolower((unsigned char)character);
	}
	return text;
}

std::optional<std::string> FindFileArgument(const std::vector<std::string>& extensions)
{
	for (const std::string& argument : s_arguments)
	{
		if (argument.rfind("--", 0) == 0)
		{
			continue;
		}
		const std::string lower = ToLower(argument);
		for (const std::string& extension : extensions)
		{
			if (lower.size() > extension.size() &&
			    lower.compare(lower.size() - extension.size(), extension.size(), extension) == 0)
			{
				return argument;
			}
		}
	}
	return std::nullopt;
}
}  // namespace Struktur::Core::CommandLine
