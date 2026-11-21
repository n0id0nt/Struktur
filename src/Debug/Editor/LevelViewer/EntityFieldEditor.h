#pragma once

#include <string>

namespace Struktur
{
class GameContext;

namespace Debug
{
class LevelViewer;

// The Inspector's editable view of an LDtk entity's fields while the level viewer has its file open: one control per
// field, matched to its LDtk type and definition (Int/Float drags honouring min/max, Bool, String/FilePath/Multilines
// text with a file browser for FilePath, colour pickers, enum dropdowns) plus a Reset back to the definition's default.
// Changes go through LevelViewer::SetEntityField, so they're undoable, saved with the file and shown in the entity's
// preview straight away. Field types it can't edit yet (arrays, entity refs, points, tiles) are shown read-only.
namespace EntityFieldEditor
{
void Render(GameContext& context, LevelViewer& levelViewer, const std::string& entityIid);
}  // namespace EntityFieldEditor
}  // namespace Debug
}  // namespace Struktur
