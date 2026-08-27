#pragma once

#include "Engine/Core/FileSystem.h"
#include "wren.hpp"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrappers for FileResult
struct WrenFileResult
{
	Struktur::FileResult<void> fileResult;

	WrenFileResult(Struktur::FileResult<void>& fileResult)
	    : fileResult(fileResult)
	{
	}
};

struct WrenFileResultByte
{
	Struktur::FileResult<std::vector<uint8_t>> fileResult;

	WrenFileResultByte(Struktur::FileResult<std::vector<uint8_t>>& fileResult)
	    : fileResult(fileResult)
	{
	}
};

struct WrenFileResultString
{
	Struktur::FileResult<std::string> fileResult;

	WrenFileResultString(Struktur::FileResult<std::string>& fileResult)
	    : fileResult(fileResult)
	{
	}
};
