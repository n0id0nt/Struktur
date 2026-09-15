#pragma once

#include <string>
#include <vector>

namespace Struktur
{
namespace Component
{
struct Level
{
	int index;
	std::string Iid;
	std::string identifier;
	int width, height;
	std::vector<std::string> tags;
};
}  // namespace Component
}  // namespace Struktur
