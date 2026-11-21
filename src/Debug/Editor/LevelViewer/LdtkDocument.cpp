#include "LdtkDocument.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>

#include "Engine/Core/FileSystem.h"

namespace Struktur::Debug
{
// One span of the saved text to replace when saving.
struct LdtkTextReplacement
{
	size_t start;
	size_t end;
	std::string text;
};

static bool StartsLater(const LdtkTextReplacement& left, const LdtkTextReplacement& right)
{
	return left.start > right.start;
}

// "#RRGGBB" (or "RRGGBB", either case) as 0xRRGGBB - LDtk's own form for a colour (realEditorValues, defaults).
static bool ParseHexColor(const std::string& text, int& outColor)
{
	std::string digits = !text.empty() && text[0] == '#' ? text.substr(1) : text;
	if (digits.size() != 6)
	{
		return false;
	}
	for (char character : digits)
	{
		if (!std::isxdigit(static_cast<unsigned char>(character)))
		{
			return false;
		}
	}
	outColor = std::stoi(digits, nullptr, 16);
	return true;
}

// 0xRRGGBB as LDtk exports it in __value: "#RRGGBB".
static std::string FormatHexColor(int color)
{
	char text[8];
	std::snprintf(text, sizeof(text), "#%06X", color & 0xFFFFFF);
	return text;
}

static bool IsEnumType(const std::string& typeName)
{
	return typeName.starts_with("LocalEnum.") || typeName.starts_with("ExternEnum.");
}

static bool IsStringType(const std::string& typeName)
{
	return typeName == "String" || typeName == "Multilines" || typeName == "FilePath" || IsEnumType(typeName);
}

// Whether SetFieldValue can write `value` into a field of LDtk type `typeName` (null clears it to its default).
static bool IsEditableValue(const std::string& typeName, const nlohmann::ordered_json& value)
{
	if (value.is_null())
	{
		return typeName == "Int" || typeName == "Float" || typeName == "Bool" || typeName == "Color" ||
		       IsStringType(typeName);
	}
	if (typeName == "Color")
	{
		int color = 0;
		return value.is_string() && ParseHexColor(value.get<std::string>(), color);
	}
	if (typeName == "Int")
	{
		return value.is_number_integer();
	}
	if (typeName == "Float")
	{
		return value.is_number();
	}
	if (typeName == "Bool")
	{
		return value.is_boolean();
	}
	return IsStringType(typeName) && value.is_string();
}

// LDtk loads a field's value from realEditorValues (__value is only the exported copy): one {"id", "params"} entry,
// or none for "use the field's default".
static nlohmann::ordered_json BuildRealEditorValues(const std::string& typeName, const nlohmann::ordered_json& value)
{
	nlohmann::ordered_json values = nlohmann::ordered_json::array();
	if (value.is_null())
	{
		return values;
	}
	nlohmann::ordered_json entry = nlohmann::ordered_json::object();
	if (typeName == "Int")
	{
		entry["id"] = "V_Int";
	}
	else if (typeName == "Float")
	{
		entry["id"] = "V_Float";
	}
	else if (typeName == "Bool")
	{
		entry["id"] = "V_Bool";
	}
	else if (typeName == "Color")
	{
		// LDtk keeps a colour as an int (0xRRGGBB) here, and only exports the hex string in __value.
		int color = 0;
		ParseHexColor(value.get<std::string>(), color);
		entry["id"]     = "V_Int";
		entry["params"] = nlohmann::ordered_json::array({color});
		values.push_back(entry);
		return values;
	}
	else
	{
		entry["id"] = "V_String";
	}
	entry["params"] = nlohmann::ordered_json::array({value});
	values.push_back(entry);
	return values;
}

// The value a field takes when it has no realEditorValues: the definition's defaultOverride, else null for a nullable
// field, else the type's zero value.
static nlohmann::ordered_json DefaultFieldValue(const nlohmann::ordered_json& definition, const std::string& typeName)
{
	const nlohmann::ordered_json& defaultOverride = definition["defaultOverride"];
	if (defaultOverride.is_object() && defaultOverride.contains("params") && !defaultOverride["params"].empty())
	{
		const nlohmann::ordered_json& defaultValue = defaultOverride["params"][0];
		// A colour default is stored as an int, like the colour's realEditorValues.
		if (typeName == "Color" && defaultValue.is_number_integer())
		{
			return FormatHexColor(defaultValue.get<int>());
		}
		return defaultValue;
	}
	if (definition["canBeNull"].get<bool>())
	{
		return nullptr;
	}
	if (typeName == "Color")
	{
		return FormatHexColor(0);
	}
	if (typeName == "Int")
	{
		return 0;
	}
	if (typeName == "Float")
	{
		return 0.0;
	}
	if (typeName == "Bool")
	{
		return false;
	}
	if (typeName == "String" || typeName == "Multilines" || typeName == "FilePath")
	{
		return "";
	}
	return nullptr;
}

// Index just past the JSON string whose opening quote is at text[start], or npos.
static size_t SkipString(const std::string& text, size_t start)
{
	size_t index = start + 1;
	while (index < text.size())
	{
		char character = text[index];
		if (character == '\\')
		{
			index += 2;
			continue;
		}
		if (character == '"')
		{
			return index + 1;
		}
		index++;
	}
	return std::string::npos;
}

// Index of the bracket closing the '{' or '[' at text[openIndex], or npos.
static size_t FindMatchingClose(const std::string& text, size_t openIndex)
{
	int depth    = 0;
	size_t index = openIndex;
	while (index < text.size())
	{
		char character = text[index];
		if (character == '"')
		{
			index = SkipString(text, index);
			if (index == std::string::npos)
			{
				return std::string::npos;
			}
			continue;
		}
		if (character == '{' || character == '[')
		{
			depth++;
		}
		else if (character == '}' || character == ']')
		{
			depth--;
			if (depth == 0)
			{
				return index;
			}
		}
		index++;
	}
	return std::string::npos;
}

// Index of the '{' of the innermost object containing text[position], or npos. Scans from the start of the text so
// that brackets inside strings are skipped correctly.
static size_t FindEnclosingObjectStart(const std::string& text, size_t position)
{
	std::vector<size_t> openBrackets;
	size_t index = 0;
	while (index < position)
	{
		char character = text[index];
		if (character == '"')
		{
			index = SkipString(text, index);
			if (index == std::string::npos)
			{
				return std::string::npos;
			}
			continue;
		}
		if (character == '{' || character == '[')
		{
			openBrackets.push_back(index);
		}
		else if ((character == '}' || character == ']') && !openBrackets.empty())
		{
			openBrackets.pop_back();
		}
		index++;
	}
	for (auto bracket = openBrackets.rbegin(); bracket != openBrackets.rend(); ++bracket)
	{
		if (text[*bracket] == '{')
		{
			return *bracket;
		}
	}
	return std::string::npos;
}

// The span [outStart, outEnd) of the JSON value that starts at or after valueIndex (leading whitespace skipped).
static bool FindValueSpan(const std::string& text, size_t valueIndex, size_t& outStart, size_t& outEnd)
{
	size_t index = text.find_first_not_of(" \t\r\n", valueIndex);
	if (index == std::string::npos)
	{
		return false;
	}
	outStart       = index;
	char character = text[index];
	if (character == '{' || character == '[')
	{
		size_t close = FindMatchingClose(text, index);
		outEnd       = close == std::string::npos ? close : close + 1;
	}
	else if (character == '"')
	{
		outEnd = SkipString(text, index);
	}
	else
	{
		outEnd = text.find_first_of(",}] \t\r\n", index);
	}
	return outEnd != std::string::npos;
}

// The span of the value of `key` among the direct members of the object whose '{' is at objectStart.
static bool FindMemberValueSpan(const std::string& text, size_t objectStart, const std::string& key, size_t& outStart,
                                size_t& outEnd)
{
	size_t objectEnd = FindMatchingClose(text, objectStart);
	if (objectEnd == std::string::npos)
	{
		return false;
	}
	std::string quotedKey = nlohmann::ordered_json(key).dump();
	size_t index          = objectStart + 1;
	while (index < objectEnd)
	{
		index = text.find_first_not_of(" \t\r\n,", index);
		if (index == std::string::npos || index >= objectEnd || text[index] != '"')
		{
			return false;
		}
		size_t keyEnd = SkipString(text, index);
		if (keyEnd == std::string::npos)
		{
			return false;
		}
		bool isKey   = text.compare(index, keyEnd - index, quotedKey) == 0;
		size_t colon = text.find(':', keyEnd);
		size_t valueStart = 0;
		size_t valueEnd   = 0;
		if (colon == std::string::npos || !FindValueSpan(text, colon + 1, valueStart, valueEnd))
		{
			return false;
		}
		if (isKey)
		{
			outStart = valueStart;
			outEnd   = valueEnd;
			return true;
		}
		index = valueEnd;
	}
	return false;
}

// The span of element `elementIndex` of the array whose '[' is at arrayStart.
static bool FindArrayElementSpan(const std::string& text, size_t arrayStart, size_t elementIndex, size_t& outStart,
                                 size_t& outEnd)
{
	size_t arrayEnd = FindMatchingClose(text, arrayStart);
	if (arrayEnd == std::string::npos)
	{
		return false;
	}
	size_t index        = arrayStart + 1;
	size_t currentIndex = 0;
	while (index < arrayEnd)
	{
		index = text.find_first_not_of(" \t\r\n,", index);
		if (index == std::string::npos || index >= arrayEnd)
		{
			return false;
		}
		size_t valueStart = 0;
		size_t valueEnd   = 0;
		if (!FindValueSpan(text, index, valueStart, valueEnd))
		{
			return false;
		}
		if (currentIndex == elementIndex)
		{
			outStart = valueStart;
			outEnd   = valueEnd;
			return true;
		}
		currentIndex++;
		index = valueEnd;
	}
	return false;
}

// A value the way LDtk writes one inside an entity: the outermost object on one line ({ "key": value, ... }), arrays
// inline ([1,2]), and any object nested deeper broken over lines - its members one tab deeper than `indent` (the
// indentation of the line the value starts on), its closing brace back at `indent`. E.g. a field instance:
//     { "__identifier": "Name", ..., "realEditorValues": [{
//         "id": "V_String",
//         "params": ["Finn"]
//     }] }
static std::string FormatLikeLdtk(const nlohmann::ordered_json& value, const std::string& indent, bool isOutermost)
{
	if (value.is_object())
	{
		if (value.empty())
		{
			return "{}";
		}
		std::string memberSeparator = isOutermost ? ", " : ",\n" + indent + "\t";
		std::string text            = isOutermost ? "{ " : "{\n" + indent + "\t";
		bool isFirst                = true;
		for (const auto& member : value.items())
		{
			if (!isFirst)
			{
				text += memberSeparator;
			}
			isFirst = false;
			text += nlohmann::ordered_json(member.key()).dump() + ": " + FormatLikeLdtk(member.value(), indent, false);
		}
		return text + (isOutermost ? " }" : "\n" + indent + "}");
	}
	if (value.is_array())
	{
		std::string text = "[";
		for (size_t i = 0; i < value.size(); i++)
		{
			if (i > 0)
			{
				text += ",";
			}
			text += FormatLikeLdtk(value[i], indent, false);
		}
		return text + "]";
	}
	return value.dump();
}

// The run of tabs/spaces at the start of the line containing text[position].
static std::string LineIndentation(const std::string& text, size_t position)
{
	size_t lineStart = text.rfind('\n', position);
	lineStart        = lineStart == std::string::npos ? 0 : lineStart + 1;
	size_t indentEnd = text.find_first_not_of(" \t", lineStart);
	return text.substr(lineStart, std::min(indentEnd, position) - lineStart);
}

bool LdtkDocument::Load(const std::string& filePath, std::string& outError)
{
	Clear();
	auto contents = FileSystem::ReadString(filePath);
	if (!contents.success)
	{
		outError = "Can't read " + filePath + ": " + contents.errorMessage;
		return false;
	}
	nlohmann::ordered_json parsed = nlohmann::ordered_json::parse(contents.value, nullptr, false);
	if (parsed.is_discarded() || !parsed.contains("levels"))
	{
		outError = filePath + " isn't a readable LDtk project";
		return false;
	}

	m_filePath  = filePath;
	m_savedText = std::move(contents.value);
	m_savedJson = std::move(parsed);
	m_json      = m_savedJson;

	const auto& levels = m_json["levels"];
	for (size_t levelIndex = 0; levelIndex < levels.size(); levelIndex++)
	{
		const auto& layers = levels[levelIndex]["layerInstances"];
		if (!layers.is_array())
		{
			continue;
		}
		for (size_t layerIndex = 0; layerIndex < layers.size(); layerIndex++)
		{
			const auto& entities = layers[layerIndex]["entityInstances"];
			for (size_t entityIndex = 0; entityIndex < entities.size(); entityIndex++)
			{
				m_entityLocations[entities[entityIndex]["iid"].get<std::string>()] = {levelIndex, layerIndex,
				                                                                      entityIndex};
			}
		}
	}
	return true;
}

void LdtkDocument::Clear()
{
	m_filePath.clear();
	m_savedText.clear();
	m_savedJson = nullptr;
	m_json      = nullptr;
	m_entityLocations.clear();
	m_history.clear();
	m_historyIndex      = 0;
	m_savedHistoryIndex = 0;
	m_editedEntities.clear();
}

bool LdtkDocument::HasEntity(const std::string& entityIid) const
{
	return m_entityLocations.contains(entityIid);
}

nlohmann::ordered_json* LdtkDocument::FindMutableEntity(const std::string& entityIid)
{
	auto location = m_entityLocations.find(entityIid);
	if (location == m_entityLocations.end())
	{
		return nullptr;
	}
	return &m_json["levels"][location->second.levelIndex]["layerInstances"][location->second.layerIndex]
	              ["entityInstances"][location->second.entityIndex];
}

const nlohmann::ordered_json* LdtkDocument::FindEntity(const std::string& entityIid) const
{
	auto location = m_entityLocations.find(entityIid);
	if (location == m_entityLocations.end())
	{
		return nullptr;
	}
	return &m_json["levels"][location->second.levelIndex]["layerInstances"][location->second.layerIndex]
	              ["entityInstances"][location->second.entityIndex];
}

glm::ivec2 LdtkDocument::GetEntityPosition(const std::string& entityIid) const
{
	const nlohmann::ordered_json* entity = FindEntity(entityIid);
	if (!entity)
	{
		return glm::ivec2(0);
	}
	return glm::ivec2((*entity)["px"][0].get<int>(), (*entity)["px"][1].get<int>());
}

int LdtkDocument::GetEntityGridSize(const std::string& entityIid) const
{
	auto location = m_entityLocations.find(entityIid);
	if (location == m_entityLocations.end())
	{
		return 1;
	}
	return m_json["levels"][location->second.levelIndex]["layerInstances"][location->second.layerIndex]["__gridSize"]
	    .get<int>();
}

glm::ivec2 LdtkDocument::GetEntityLevelSize(const std::string& entityIid) const
{
	auto location = m_entityLocations.find(entityIid);
	if (location == m_entityLocations.end())
	{
		return glm::ivec2(0);
	}
	const auto& level = m_json["levels"][location->second.levelIndex];
	return glm::ivec2(level["pxWid"].get<int>(), level["pxHei"].get<int>());
}

const nlohmann::ordered_json* LdtkDocument::FindFieldDefinition(const std::string& entityIid,
                                                                const std::string& fieldIdentifier) const
{
	const nlohmann::ordered_json* entity = FindEntity(entityIid);
	const nlohmann::ordered_json* field  = FindFieldInstance(entityIid, fieldIdentifier);
	if (!entity || !field)
	{
		return nullptr;
	}
	for (const auto& entityDefinition : m_json["defs"]["entities"])
	{
		if (entityDefinition["uid"] != (*entity)["defUid"])
		{
			continue;
		}
		for (const auto& fieldDefinition : entityDefinition["fieldDefs"])
		{
			if (fieldDefinition["uid"] == (*field)["defUid"])
			{
				return &fieldDefinition;
			}
		}
	}
	return nullptr;
}

std::vector<std::string> LdtkDocument::GetEnumValues(const std::string& enumIdentifier) const
{
	std::vector<std::string> values;
	for (const char* enumList : {"enums", "externalEnums"})
	{
		if (!m_json["defs"].contains(enumList))
		{
			continue;
		}
		for (const auto& enumDefinition : m_json["defs"][enumList])
		{
			if (enumDefinition["identifier"] != enumIdentifier)
			{
				continue;
			}
			for (const auto& value : enumDefinition["values"])
			{
				values.push_back(value["id"].get<std::string>());
			}
			return values;
		}
	}
	return values;
}

const nlohmann::ordered_json* LdtkDocument::FindFieldInstance(const std::string& entityIid,
                                                              const std::string& fieldIdentifier) const
{
	const nlohmann::ordered_json* entity = FindEntity(entityIid);
	if (!entity)
	{
		return nullptr;
	}
	for (const auto& field : (*entity)["fieldInstances"])
	{
		if (field["__identifier"] == fieldIdentifier)
		{
			return &field;
		}
	}
	return nullptr;
}

bool LdtkDocument::MoveEntity(const std::string& entityIid, const glm::ivec2& position)
{
	const nlohmann::ordered_json* entity = FindEntity(entityIid);
	if (!entity)
	{
		return false;
	}
	nlohmann::ordered_json after = nlohmann::ordered_json::array({position.x, position.y});
	if ((*entity)["px"] == after)
	{
		return false;
	}
	Edit edit{entityIid, "", (*entity)["px"], after};
	ApplyValue(entityIid, "", after);
	RecordEdit(std::move(edit));
	return true;
}

bool LdtkDocument::SetFieldValue(const std::string& entityIid, const std::string& fieldIdentifier,
                                 const nlohmann::ordered_json& value, bool mergeWithLastEdit)
{
	const nlohmann::ordered_json* field = FindFieldInstance(entityIid, fieldIdentifier);
	if (!field)
	{
		return false;
	}
	const std::string typeName = (*field)["__type"].get<std::string>();
	if (!IsEditableValue(typeName, value))
	{
		return false;
	}

	// The whole field instance is the edit's before/after, so undo restores realEditorValues exactly (an untouched
	// field has none - "use the default" - which writing its value back would not reproduce).
	nlohmann::ordered_json after = *field;
	after["__value"]             = value;
	after["realEditorValues"]    = BuildRealEditorValues(typeName, value);
	return CommitFieldEdit(entityIid, fieldIdentifier, after, mergeWithLastEdit);
}

bool LdtkDocument::ResetFieldToDefault(const std::string& entityIid, const std::string& fieldIdentifier)
{
	const nlohmann::ordered_json* field      = FindFieldInstance(entityIid, fieldIdentifier);
	const nlohmann::ordered_json* definition = FindFieldDefinition(entityIid, fieldIdentifier);
	if (!field || !definition)
	{
		return false;
	}
	nlohmann::ordered_json after = *field;
	after["__value"]             = DefaultFieldValue(*definition, (*field)["__type"].get<std::string>());
	after["realEditorValues"]    = nlohmann::ordered_json::array();
	return CommitFieldEdit(entityIid, fieldIdentifier, after, false);
}

bool LdtkDocument::CommitFieldEdit(const std::string& entityIid, const std::string& fieldIdentifier,
                                   const nlohmann::ordered_json& after, bool mergeWithLastEdit)
{
	const nlohmann::ordered_json* field = FindFieldInstance(entityIid, fieldIdentifier);
	if (!field || after == *field)
	{
		return false;
	}

	bool canMerge = mergeWithLastEdit && m_historyIndex > 0 && m_historyIndex == m_history.size() &&
	                m_history.back().entityIid == entityIid && m_history.back().fieldIdentifier == fieldIdentifier;
	if (canMerge)
	{
		m_history.back().after = after;
		ApplyValue(entityIid, fieldIdentifier, after);
		// The merged edit was the saved state - it isn't any more.
		if (m_savedHistoryIndex == m_historyIndex)
		{
			m_savedHistoryIndex = SIZE_MAX;
		}
		return true;
	}

	Edit edit{entityIid, fieldIdentifier, *field, after};
	ApplyValue(entityIid, fieldIdentifier, after);
	RecordEdit(std::move(edit));
	return true;
}

void LdtkDocument::ApplyValue(const std::string& entityIid, const std::string& fieldIdentifier,
                              const nlohmann::ordered_json& value)
{
	nlohmann::ordered_json* entity = FindMutableEntity(entityIid);
	if (!entity)
	{
		return;
	}
	m_editedEntities.insert(entityIid);

	if (!fieldIdentifier.empty())
	{
		for (auto& field : (*entity)["fieldInstances"])
		{
			if (field["__identifier"] == fieldIdentifier)
			{
				field = value;
				return;
			}
		}
		return;
	}

	// A move: px plus the values LDtk derives from it.
	const EntityLocation& location = m_entityLocations.at(entityIid);
	const auto& level              = m_json["levels"][location.levelIndex];
	const auto& layer              = level["layerInstances"][location.layerIndex];
	int x                          = value[0].get<int>();
	int y                          = value[1].get<int>();
	int gridSize                   = layer["__gridSize"].get<int>();
	(*entity)["px"]                = value;
	(*entity)["__grid"]            = nlohmann::ordered_json::array(
        {(int)std::floor((float)x / gridSize), (int)std::floor((float)y / gridSize)});
	(*entity)["__worldX"] = level["worldX"].get<int>() + x + layer["__pxTotalOffsetX"].get<int>();
	(*entity)["__worldY"] = level["worldY"].get<int>() + y + layer["__pxTotalOffsetY"].get<int>();
}

void LdtkDocument::RecordEdit(Edit edit)
{
	m_history.resize(m_historyIndex);
	if (m_savedHistoryIndex > m_historyIndex)
	{
		m_savedHistoryIndex = SIZE_MAX;
	}
	m_history.push_back(std::move(edit));
	m_historyIndex++;
}

const LdtkDocument::Edit* LdtkDocument::Undo()
{
	if (!CanUndo())
	{
		return nullptr;
	}
	m_historyIndex--;
	const Edit& edit = m_history[m_historyIndex];
	ApplyValue(edit.entityIid, edit.fieldIdentifier, edit.before);
	return &edit;
}

const LdtkDocument::Edit* LdtkDocument::Redo()
{
	if (!CanRedo())
	{
		return nullptr;
	}
	const Edit& edit = m_history[m_historyIndex];
	m_historyIndex++;
	ApplyValue(edit.entityIid, edit.fieldIdentifier, edit.after);
	return &edit;
}

bool LdtkDocument::BuildPatchedText(std::string& outText, std::string& outError) const
{
	std::vector<LdtkTextReplacement> replacements;
	for (const std::string& entityIid : m_editedEntities)
	{
		const EntityLocation& location     = m_entityLocations.at(entityIid);
		const nlohmann::ordered_json& entity = *FindEntity(entityIid);
		const nlohmann::ordered_json& savedEntity =
		    m_savedJson["levels"][location.levelIndex]["layerInstances"][location.layerIndex]["entityInstances"]
		               [location.entityIndex];
		if (entity == savedEntity)
		{
			continue;
		}

		// Entity iids are unique and appear as an "iid" member only on their own entity (references use
		// "entityIid"), so this finds the entity's object in the text.
		std::string iidMember = "\"iid\": " + nlohmann::ordered_json(entityIid).dump();
		size_t iidPosition    = m_savedText.find(iidMember);
		if (iidPosition == std::string::npos || m_savedText.find(iidMember, iidPosition + 1) != std::string::npos)
		{
			outError = "Couldn't find entity " + entityIid + " in the file text";
			return false;
		}
		size_t objectStart = FindEnclosingObjectStart(m_savedText, iidPosition);
		if (objectStart == std::string::npos)
		{
			outError = "Couldn't find the object of entity " + entityIid;
			return false;
		}

		for (const auto& member : entity.items())
		{
			const nlohmann::ordered_json& savedValue = savedEntity[member.key()];
			if (member.value() == savedValue)
			{
				continue;
			}
			size_t valueStart = 0;
			size_t valueEnd   = 0;
			if (!FindMemberValueSpan(m_savedText, objectStart, member.key(), valueStart, valueEnd))
			{
				outError = "Couldn't find \"" + member.key() + "\" of entity " + entityIid + " in the file text";
				return false;
			}

			// Replace just the changed field instances, so the rest of the array keeps LDtk's formatting.
			bool isFieldList = member.key() == "fieldInstances" && member.value().is_array() &&
			                   savedValue.is_array() && member.value().size() == savedValue.size();
			if (!isFieldList)
			{
				replacements.push_back(
				    {valueStart, valueEnd,
				     FormatLikeLdtk(member.value(), LineIndentation(m_savedText, valueStart), true)});
				continue;
			}
			for (size_t i = 0; i < member.value().size(); i++)
			{
				if (member.value()[i] == savedValue[i])
				{
					continue;
				}
				size_t fieldStart = 0;
				size_t fieldEnd   = 0;
				if (!FindArrayElementSpan(m_savedText, valueStart, i, fieldStart, fieldEnd))
				{
					outError = "Couldn't find field " + std::to_string(i) + " of entity " + entityIid;
					return false;
				}
				replacements.push_back(
				    {fieldStart, fieldEnd,
				     FormatLikeLdtk(member.value()[i], LineIndentation(m_savedText, fieldStart), true)});
			}
		}
	}

	std::sort(replacements.begin(), replacements.end(), StartsLater);
	outText = m_savedText;
	for (const LdtkTextReplacement& replacement : replacements)
	{
		outText.replace(replacement.start, replacement.end - replacement.start, replacement.text);
	}
	return true;
}

bool LdtkDocument::Save(std::string& outError)
{
	if (!IsLoaded())
	{
		outError = "No file open";
		return false;
	}

	// Patching against a stale baseline would silently throw away whatever changed the file (e.g. a save from LDtk).
	auto diskContents = FileSystem::ReadString(m_filePath);
	if (!diskContents.success || diskContents.value != m_savedText)
	{
		outError = m_filePath + " changed on disk since it was opened - reload it (your unsaved edits will be lost) "
		                        "or redo them after reloading";
		return false;
	}

	std::string patchedText;
	if (!BuildPatchedText(patchedText, outError))
	{
		return false;
	}
	nlohmann::ordered_json reparsed = nlohmann::ordered_json::parse(patchedText, nullptr, false);
	if (reparsed.is_discarded() || reparsed != m_json)
	{
		outError = "Save check failed - the patched file doesn't match the edits, so nothing was written";
		return false;
	}

	auto realPath = FileSystem::GetRealPath(m_filePath);
	if (!realPath.success)
	{
		outError = realPath.errorMessage;
		return false;
	}
	std::ofstream file(realPath.value, std::ios::binary | std::ios::trunc);
	file << patchedText;
	file.close();
	if (!file)
	{
		outError = "Couldn't write " + realPath.value;
		return false;
	}

	m_savedText         = std::move(patchedText);
	m_savedJson         = m_json;
	m_savedHistoryIndex = m_historyIndex;
	m_editedEntities.clear();
	return true;
}
}  // namespace Struktur::Debug
