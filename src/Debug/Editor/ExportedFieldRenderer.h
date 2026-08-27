#pragma once

#include <string>

#include "Engine/Scripting/WrenValueWrapper.h"

namespace Struktur::Debug
{
// One ImGui widget per Wren value type, shared by every editor surface that shows #!export-tagged fields
// (InspectorWindow's script components, StateWindow's states, and anything else that reads a
// Wren::WrenItem via Wren::ExportedFields::GetValue - see WrenExportedFields.h). Adding support for a new
// Wren value type (List/Map/a foreign class, say) means writing one new renderer and registering it in
// ExportedFieldRenderer.cpp - every window that already calls RenderExportedField picks it up automatically,
// no per-window switch statement to update.
class IExportedFieldRenderer
{
public:
	virtual ~IExportedFieldRenderer() = default;

	// The Wren value type this renderer handles - used by RenderExportedField to pick the right one.
	virtual WrenType GetHandledType() const = 0;

	// Renders one field's current value under ImGui id/label `label` (caller should already have pushed a
	// unique ImGui ID if `label` isn't unique on its own - matches how every other list-of-fields render in
	// this codebase works). `editable` gates whether the widget accepts input at all (e.g. no matching Wren
	// setter). Returns true on any frame the user changed the value, with `out_newValue` set to the edited
	// value ready to hand to Wren::ExportedFields::SetValue - matches the existing commit-on-change behaviour
	// (no deactivation-edge tracking), so a drag/text edit calls SetValue every frame it's actively changing.
	virtual bool Render(const std::string& label, const Wren::WrenItem& value, bool editable,
	                    Wren::WrenItem& out_newValue) const = 0;
};

// The single entry point every window should call - looks up the renderer registered for value.type and
// delegates to it; if no renderer is registered for that type, renders a plain "<unsupported type>" line
// instead (never crashes/asserts on an unhandled type, matching this codebase's general "safe default over
// hard failure" convention for anything content/script-driven).
bool RenderExportedField(const std::string& label, const Wren::WrenItem& value, bool editable,
                         Wren::WrenItem& out_newValue);
}  // namespace Struktur::Debug
