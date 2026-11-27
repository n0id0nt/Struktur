#pragma once

#include <optional>
#include <string>
#include <vector>

// The process command line, captured in main() before the engine starts so any system can ask
// for a flag (e.g. --demo) or a file to open.
namespace Struktur::Core::CommandLine
{
void Set(int argc, char* argv[]);
const std::vector<std::string>& GetArguments();
bool HasFlag(const std::string& flag);
// The first argument that isn't a flag and ends in one of `extensions` (lower-case, with the dot).
std::optional<std::string> FindFileArgument(const std::vector<std::string>& extensions);
}  // namespace Struktur::Core::CommandLine
