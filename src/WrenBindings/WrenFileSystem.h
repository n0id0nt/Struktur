#pragma once

#include "wren.hpp"

#include "Engine/Core/FileSystem.h"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrappers for FileResult
struct WrenFileResult
{
	Struktur::FileResult<void> fileResult;

	WrenFileResult(Struktur::FileResult<void>& fileResult) : fileResult(fileResult) {}
};

struct WrenFileResultByte
{
	Struktur::FileResult<std::vector<uint8_t>> fileResult;

	WrenFileResultByte(Struktur::FileResult<std::vector<uint8_t>>& fileResult) : fileResult(fileResult) {}
};

struct WrenFileResultString
{
	Struktur::FileResult<std::string> fileResult;

	WrenFileResultString(Struktur::FileResult<std::string>& fileResult) : fileResult(fileResult) {}
};
