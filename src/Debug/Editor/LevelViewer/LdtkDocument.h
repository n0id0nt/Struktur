#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "glm/glm.hpp"
#include "nlohmann/json.hpp"

namespace Struktur
{
namespace Debug
{
// An LDtk project file open for editing in the level viewer: the file's text as last loaded/saved plus a parsed copy
// that edits change (ordered, so keys keep LDtk's order). Edits are undoable. Save writes back only what changed -
// each changed value patched into the original text, LDtk's own formatting everywhere else - and only after checking
// the patched text parses to exactly the edited copy, and that nothing else (LDtk) changed the file since it was read.
class LdtkDocument
{
public:
	struct Edit
	{
		std::string entityIid;
		// Empty for a move; otherwise the identifier of the field instance that was set.
		std::string fieldIdentifier;
		// A move's [x, y] (px within the level, LDtk's "px"), or a field's __value - before and after the edit.
		nlohmann::ordered_json before;
		nlohmann::ordered_json after;
	};

	// Reads and parses the file (a virtual path, e.g. "Levels/SevenGods.ldtk"). Clears the undo history.
	bool Load(const std::string& filePath, std::string& outError);
	void Clear();
	bool IsLoaded() const
	{
		return !m_filePath.empty();
	}

	bool HasEntity(const std::string& entityIid) const;
	// LDtk's "px" (top-left of the entity's cell relative to its level, before the pivot) and its layer's grid size.
	glm::ivec2 GetEntityPosition(const std::string& entityIid) const;
	int GetEntityGridSize(const std::string& entityIid) const;
	// The size (px) of the level the entity is in.
	glm::ivec2 GetEntityLevelSize(const std::string& entityIid) const;
	// The entity's JSON object, or null.
	const nlohmann::ordered_json* FindEntity(const std::string& entityIid) const;
	// The entity's field instance with this identifier, or null.
	const nlohmann::ordered_json* FindFieldInstance(const std::string& entityIid,
	                                                const std::string& fieldIdentifier) const;
	// The definition (defs.entities[].fieldDefs[]) of that field instance - type, canBeNull, min/max,
	// defaultOverride - or null.
	const nlohmann::ordered_json* FindFieldDefinition(const std::string& entityIid,
	                                                  const std::string& fieldIdentifier) const;
	// The value ids of the enum with this identifier (defs.enums / defs.externalEnums), in LDtk's order.
	std::vector<std::string> GetEnumValues(const std::string& enumIdentifier) const;

	// Undoable edits. False (and nothing recorded) if the entity or field doesn't exist, the value is unchanged, or -
	// for SetFieldValue - the field's type can't be edited yet (only Int, Float, Bool, String, Multilines, FilePath and
	// enums can; null clears a field back to its default). A Color value is a "#RRGGBB" string.
	bool MoveEntity(const std::string& entityIid, const glm::ivec2& position);
	// mergeWithLastEdit folds the change into the previous edit if that set the same field - one undo step for a
	// whole drag or typing session.
	bool SetFieldValue(const std::string& entityIid, const std::string& fieldIdentifier,
	                   const nlohmann::ordered_json& value, bool mergeWithLastEdit = false);
	// Back to "use the definition's default" (no realEditorValues), __value set to that default.
	bool ResetFieldToDefault(const std::string& entityIid, const std::string& fieldIdentifier);

	// The edit just undone/redone (the caller refreshes that entity), or null when there's nothing to undo/redo.
	const Edit* Undo();
	const Edit* Redo();
	bool CanUndo() const
	{
		return m_historyIndex > 0;
	}
	bool CanRedo() const
	{
		return m_historyIndex < m_history.size();
	}
	bool HasUnsavedChanges() const
	{
		return m_historyIndex != m_savedHistoryIndex;
	}

	// Writes the edits to the file on disk. False, with nothing written, if the file changed on disk since it was
	// read, the patched text doesn't parse back to the edited document, or the file can't be written.
	bool Save(std::string& outError);

private:
	struct EntityLocation
	{
		size_t levelIndex;
		size_t layerIndex;
		size_t entityIndex;
	};

	nlohmann::ordered_json* FindMutableEntity(const std::string& entityIid);
	// Records the field instance `after` as an edit (or folds it into the last one - see SetFieldValue).
	bool CommitFieldEdit(const std::string& entityIid, const std::string& fieldIdentifier,
	                     const nlohmann::ordered_json& after, bool mergeWithLastEdit);
	// Sets a move's position or a field's value on the parsed copy - no history.
	void ApplyValue(const std::string& entityIid, const std::string& fieldIdentifier,
	                const nlohmann::ordered_json& value);
	void RecordEdit(Edit edit);
	// The saved text with every edited entity's changed values patched in, or false with outError set.
	bool BuildPatchedText(std::string& outText, std::string& outError) const;

	std::string m_filePath;
	// The file as last loaded or saved, and that text parsed - the baseline Save diffs and patches against.
	std::string m_savedText;
	nlohmann::ordered_json m_savedJson;
	// m_savedJson with the current edits applied.
	nlohmann::ordered_json m_json;
	std::unordered_map<std::string, EntityLocation> m_entityLocations;

	std::vector<Edit> m_history;
	// Edits before this index are applied; the rest are redoable.
	size_t m_historyIndex = 0;
	// m_historyIndex as of the last load/save - no longer reachable (SIZE_MAX) once that point was undone past and
	// overwritten by a new edit.
	size_t m_savedHistoryIndex = 0;
	// Entities touched since the last save - the only ones Save diffs.
	std::unordered_set<std::string> m_editedEntities;
};
}  // namespace Debug
}  // namespace Struktur
