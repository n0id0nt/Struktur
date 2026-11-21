#include "EntityFieldEditor.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <cfloat>
#include <climits>
#include <cstdio>
#include <filesystem>

#include "Debug/Editor/LevelViewer/LevelViewer.h"
#include "Engine/Core/FileSystem.h"
#include "nlohmann/json.hpp"

namespace Struktur::Debug
{
// Share of the Inspector's width the value controls take; the field's name sits to their right.
static constexpr float FIELD_WIDTH_FRACTION = 0.6f;
static constexpr float INT_DRAG_SPEED       = 0.2f;
static constexpr float FLOAT_DRAG_SPEED     = 0.01f;
static constexpr int MULTILINE_ROWS         = 4;
static constexpr size_t READ_ONLY_MAX_CHARS = 60;

// What a field's control produced this frame.
struct EntityFieldEditResult
{
	bool changed = false;
	nlohmann::ordered_json value;
	// Part of the same drag or typing session as the control's previous change - folded into that undo step.
	bool isContinuing = false;
};

// The control whose changes are being folded into one undo step (the one being dragged or typed into), or 0.
static ImGuiID s_editingItem = 0;

// Call right after a draggable/typeable control: whether its change continues the current edit session.
static bool ContinuesEditSession(bool changed)
{
	ImGuiID item = ImGui::GetItemID();
	if (ImGui::IsItemDeactivated() && s_editingItem == item)
	{
		s_editingItem = 0;
	}
	if (!changed)
	{
		return false;
	}
	bool isContinuing = ImGui::IsItemActive() && s_editingItem == item;
	s_editingItem     = ImGui::IsItemActive() ? item : 0;
	return isContinuing;
}

static bool IsEnumType(const std::string& typeName)
{
	return typeName.starts_with("LocalEnum.") || typeName.starts_with("ExternEnum.");
}

static bool HasNumber(const nlohmann::ordered_json* definition, const char* key)
{
	return definition && (*definition)[key].is_number();
}

static EntityFieldEditResult EditInt(const std::string& label, const nlohmann::ordered_json& value,
                                     const nlohmann::ordered_json* definition)
{
	EntityFieldEditResult result;
	int number = value.is_number() ? value.get<int>() : 0;
	// DragInt treats min == max as "no limits".
	int minimum            = 0;
	int maximum            = 0;
	ImGuiSliderFlags flags = ImGuiSliderFlags_None;
	if (HasNumber(definition, "min") || HasNumber(definition, "max"))
	{
		minimum = HasNumber(definition, "min") ? (int)(*definition)["min"].get<double>() : INT_MIN;
		maximum = HasNumber(definition, "max") ? (int)(*definition)["max"].get<double>() : INT_MAX;
		flags   = ImGuiSliderFlags_AlwaysClamp;
	}
	result.changed      = ImGui::DragInt(label.c_str(), &number, INT_DRAG_SPEED, minimum, maximum, "%d", flags);
	result.isContinuing = ContinuesEditSession(result.changed);
	result.value        = number;
	return result;
}

static EntityFieldEditResult EditFloat(const std::string& label, const nlohmann::ordered_json& value,
                                       const nlohmann::ordered_json* definition)
{
	EntityFieldEditResult result;
	float number           = value.is_number() ? value.get<float>() : 0.0f;
	float minimum          = 0.0f;
	float maximum          = 0.0f;
	ImGuiSliderFlags flags = ImGuiSliderFlags_None;
	if (HasNumber(definition, "min") || HasNumber(definition, "max"))
	{
		minimum = HasNumber(definition, "min") ? (*definition)["min"].get<float>() : -FLT_MAX;
		maximum = HasNumber(definition, "max") ? (*definition)["max"].get<float>() : FLT_MAX;
		flags   = ImGuiSliderFlags_AlwaysClamp;
	}
	result.changed = ImGui::DragFloat(label.c_str(), &number, FLOAT_DRAG_SPEED, minimum, maximum, "%.3f", flags);
	result.isContinuing = ContinuesEditSession(result.changed);
	result.value        = number;
	return result;
}

static EntityFieldEditResult EditBool(const std::string& label, const nlohmann::ordered_json& value)
{
	EntityFieldEditResult result;
	bool isChecked = value.is_boolean() && value.get<bool>();
	result.changed = ImGui::Checkbox(label.c_str(), &isChecked);
	result.value   = isChecked;
	return result;
}

static EntityFieldEditResult EditText(const std::string& label, const nlohmann::ordered_json& value, bool isMultiline)
{
	EntityFieldEditResult result;
	std::string text = value.is_string() ? value.get<std::string>() : "";
	if (isMultiline)
	{
		ImVec2 size(ImGui::CalcItemWidth(), ImGui::GetTextLineHeight() * MULTILINE_ROWS);
		result.changed = ImGui::InputTextMultiline(label.c_str(), &text, size);
	}
	else
	{
		result.changed = ImGui::InputText(label.c_str(), &text);
	}
	result.isContinuing = ContinuesEditSession(result.changed);
	result.value        = text;
	return result;
}

static EntityFieldEditResult EditEnum(const std::string& label, const nlohmann::ordered_json& value,
                                      const std::vector<std::string>& enumValues, bool canBeNull)
{
	EntityFieldEditResult result;
	std::string preview = value.is_string() ? value.get<std::string>() : "(none)";
	if (!ImGui::BeginCombo(label.c_str(), preview.c_str()))
	{
		return result;
	}
	if (canBeNull && ImGui::Selectable("(none)", value.is_null()))
	{
		result.changed = true;
		result.value   = nullptr;
	}
	for (const std::string& enumValue : enumValues)
	{
		if (ImGui::Selectable(enumValue.c_str(), value.is_string() && value.get<std::string>() == enumValue))
		{
			result.changed = true;
			result.value   = enumValue;
		}
	}
	ImGui::EndCombo();
	return result;
}

static EntityFieldEditResult EditColor(const std::string& label, const nlohmann::ordered_json& value)
{
	EntityFieldEditResult result;
	unsigned int red   = 0;
	unsigned int green = 0;
	unsigned int blue  = 0;
	if (value.is_string())
	{
		std::sscanf(value.get<std::string>().c_str(), "#%02x%02x%02x", &red, &green, &blue);
	}
	float color[3] = {red / 255.0f, green / 255.0f, blue / 255.0f};
	result.changed = ImGui::ColorEdit3(label.c_str(), color, ImGuiColorEditFlags_DisplayHex);
	result.isContinuing = ContinuesEditSession(result.changed);
	char hex[8];
	std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", (int)(color[0] * 255.0f + 0.5f), (int)(color[1] * 255.0f + 0.5f),
	              (int)(color[2] * 255.0f + 0.5f));
	result.value = std::string(hex);
	return result;
}

// A file picked with the OS file dialog, as LDtk stores it: relative to the .ldtk file, forward slashes. Empty if
// cancelled. The dialog starts in the .ldtk's folder and offers the definition's acceptFileTypes (e.g. ["png"]).
static std::string BrowseForFile(const std::string& worldFilePath, const nlohmann::ordered_json* definition)
{
	auto worldRealPath = FileSystem::GetRealPath(worldFilePath);
	if (!worldRealPath.success)
	{
		return "";
	}
	std::filesystem::path worldDirectory = std::filesystem::path(worldRealPath.value).parent_path();
	std::vector<std::string> filters;
	if (definition && (*definition)["acceptFileTypes"].is_array())
	{
		for (const auto& fileType : (*definition)["acceptFileTypes"])
		{
			filters.push_back("*." + fileType.get<std::string>());
		}
	}
	std::string picked =
	    FileSystem::OpenFileDialog("Choose a file", worldDirectory.generic_string() + "/", filters, "Accepted files");
	if (picked.empty())
	{
		return "";
	}
	return std::filesystem::path(picked).lexically_relative(worldDirectory).generic_string();
}

static void ShowReadOnly(const std::string& label, const nlohmann::ordered_json& value)
{
	std::string text = value.is_string() ? value.get<std::string>() : value.dump();
	if (text.size() > READ_ONLY_MAX_CHARS)
	{
		text = text.substr(0, READ_ONLY_MAX_CHARS) + "...";
	}
	ImGui::LabelText(label.c_str(), "%s", text.c_str());
	ImGui::SetItemTooltip("Not editable here yet - change it in LDtk");
}

static void RenderField(GameContext& context, LevelViewer& levelViewer, const std::string& entityIid,
                        const nlohmann::ordered_json& field, float controlWidth)
{
	LdtkDocument& document                   = levelViewer.GetDocument();
	const std::string identifier             = field["__identifier"].get<std::string>();
	const std::string typeName               = field["__type"].get<std::string>();
	const nlohmann::ordered_json& value      = field["__value"];
	const nlohmann::ordered_json* definition = document.FindFieldDefinition(entityIid, identifier);
	bool canBeNull                           = definition && (*definition)["canBeNull"].get<bool>();
	bool isDefault                           = field["realEditorValues"].empty();

	ImGui::PushID(identifier.c_str());
	ImGui::SetNextItemWidth(controlWidth);
	EntityFieldEditResult result;
	bool isEditable = true;
	if (typeName == "Int")
	{
		result = EditInt(identifier, value, definition);
	}
	else if (typeName == "Float")
	{
		result = EditFloat(identifier, value, definition);
	}
	else if (typeName == "Bool")
	{
		result = EditBool(identifier, value);
	}
	else if (typeName == "String")
	{
		result = EditText(identifier, value, false);
	}
	else if (typeName == "FilePath")
	{
		result = EditText(identifier, value, false);
		ImGui::SameLine();
		if (ImGui::SmallButton("..."))
		{
			std::string picked = BrowseForFile(levelViewer.GetFilePath(), definition);
			if (!picked.empty())
			{
				result.changed      = true;
				result.isContinuing = false;
				result.value        = picked;
			}
		}
		ImGui::SetItemTooltip("Choose a file (stored relative to the .ldtk, as LDtk does)");
	}
	else if (typeName == "Color")
	{
		result = EditColor(identifier, value);
	}
	else if (typeName == "Multilines")
	{
		result = EditText(identifier, value, true);
	}
	else if (IsEnumType(typeName))
	{
		std::string enumIdentifier = typeName.substr(typeName.find('.') + 1);
		result = EditEnum(identifier, value, document.GetEnumValues(enumIdentifier), canBeNull);
	}
	else
	{
		isEditable = false;
		ShowReadOnly(identifier, value);
	}
	if (isEditable && definition && (*definition)["doc"].is_string())
	{
		ImGui::SetItemTooltip("%s", (*definition)["doc"].get<std::string>().c_str());
	}

	if (result.changed)
	{
		levelViewer.SetEntityField(context, entityIid, identifier, result.value, result.isContinuing);
	}
	if (isEditable && value.is_null())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("null");
	}
	if (isEditable && !isDefault)
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("Reset"))
		{
			levelViewer.ResetEntityField(context, entityIid, identifier);
		}
		ImGui::SetItemTooltip("Back to the field's default");
	}
	ImGui::PopID();
}

void EntityFieldEditor::Render(GameContext& context, LevelViewer& levelViewer, const std::string& entityIid)
{
	const nlohmann::ordered_json* entity = levelViewer.GetDocument().FindEntity(entityIid);
	if (!entity)
	{
		return;
	}
	// A copy - an edit replaces field instances in the document while this loop is still drawing them.
	nlohmann::ordered_json fields = (*entity)["fieldInstances"];
	if (fields.empty())
	{
		ImGui::TextDisabled("No fields");
		return;
	}
	float controlWidth = ImGui::GetContentRegionAvail().x * FIELD_WIDTH_FRACTION;
	for (const auto& field : fields)
	{
		RenderField(context, levelViewer, entityIid, field, controlWidth);
	}
}
}  // namespace Struktur::Debug
