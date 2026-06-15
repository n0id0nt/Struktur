#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "glm/glm.hpp"

namespace Struktur::UI
{
class UIElement;
}

namespace Struktur::Callback
{
// Forward declare to allow recursive variant
struct VariantList;
struct VariantMap;

// Variant type for cross-language argument passing
using Variant = std::variant<std::nullptr_t, bool, int, float, double, std::string, glm::vec2, glm::vec3, glm::vec4,
                             UI::UIElement*, VariantList, VariantMap>;

// List type - vector of variants (can contain nested lists/maps)
struct VariantList
{
	std::vector<Variant> items;
};

// Map type - string keyed map of variants (can contain nested lists/maps)
struct VariantMap
{
	std::unordered_map<std::string, Variant> items;
};
}  // namespace Struktur::Callback
