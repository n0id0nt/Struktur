#include "StateWindow.h"

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "Debug/Editor/ExportedFieldRenderer.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenExportedFields.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"

namespace Struktur::Debug
{
namespace
{
// "key1=value1, key2=value2" from a JSON object - values are dumped as JSON (so a string still reads as
// "value" with quotes, matching how StateManager.wren's summariseParams already renders non-primitive params
// as a quoted "<Type>" placeholder string). Empty params render as "(none)" rather than an empty string, so a
// leaf state's row doesn't look broken/blank.
std::string FormatParams(const nlohmann::json& params)
{
	if (!params.is_object() || params.empty())
	{
		return "(none)";
	}

	std::string result;
	bool first = true;
	for (auto it = params.begin(); it != params.end(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		first = false;
		result += it.key() + "=" + it.value().dump();
	}
	return result;
}
}  // namespace

StateWindow::StateWindow()
    : EditorWindow("State")
{
}

void StateWindow::RenderToolbar(GameContext& context)
{
	std::vector<std::string> stateNames = context.GetWrenStateManager().GetRegisteredStateNames(context);

	if (stateNames.empty())
	{
		ImGui::TextDisabled("No registered states yet");
		return;
	}

	if (m_selectedStateIndex >= (int)stateNames.size())
	{
		m_selectedStateIndex = 0;
	}

	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::BeginCombo("##ForceState", stateNames[m_selectedStateIndex].c_str()))
	{
		for (int i = 0; i < (int)stateNames.size(); i++)
		{
			bool isSelected = i == m_selectedStateIndex;
			if (ImGui::Selectable(stateNames[i].c_str(), isSelected))
			{
				m_selectedStateIndex = i;
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();
	if (ImGui::Button("Change State"))
	{
		context.GetWrenStateManager().TriggerStateChange(context, stateNames[m_selectedStateIndex]);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(
		    "Forces the root StateManager to this state with empty params. States whose enter() needs specific "
		    "params (e.g. InteractState's interactingEntity) will likely break if force-triggered this way.");
	}

	ImGui::Separator();
}

void StateWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	RenderToolbar(context);

	Wren::WrenStateManager& stateManager = context.GetWrenStateManager();

	std::string stackJson = stateManager.GetActiveStateStackJson(context);
	nlohmann::json stack  = nlohmann::json::parse(stackJson, nullptr, false);
	if (stack.is_discarded() || !stack.is_array() || stack.empty())
	{
		ImGui::TextDisabled("No active state");
		ImGui::End();
		return;
	}

	std::vector<WrenHandle*> instances = stateManager.GetActiveStateInstances(context);
	WrenVM* vm                         = context.GetWrenScriptEngine().GetVM();

	for (size_t depth = 0; depth < stack.size(); depth++)
	{
		const nlohmann::json& level = stack[depth];
		std::string name            = level.value("name", "Unknown");
		std::string params          = FormatParams(level.value("params", nlohmann::json::object()));

		ImGui::PushID((int)depth);

		bool open = ImGui::TreeNode("##level", "[%zu] %s  -  %s", depth, name.c_str(), params.c_str());

		if (open)
		{
			ImGui::Indent();

			if (depth < instances.size() && instances[depth])
			{
				const std::vector<Wren::WrenExportedField>& fields =
				    stateManager.GetStateExportedFields(context, instances[depth]);

				if (fields.empty())
				{
					ImGui::TextDisabled("None (tag a getter with #!export to expose it here)");
				}
				else
				{
					for (const Wren::WrenExportedField& field : fields)
					{
						ImGui::PushID(field.name.c_str());

						Wren::WrenItem value;
						if (!stateManager.GetStateFieldValue(context, instances[depth], field.name, value))
						{
							ImGui::TextDisabled("%s: <unavailable>", field.name.c_str());
							ImGui::PopID();
							continue;
						}

						Wren::WrenItem newValue;
						if (RenderExportedField(field.name, value, field.hasSetter, newValue))
						{
							stateManager.SetStateFieldValue(context, instances[depth], field.name, newValue);
						}

						if (!field.hasSetter)
						{
							ImGui::SameLine();
							ImGui::TextDisabled("(read-only)");
						}

						ImGui::PopID();
					}
				}
			}
			else
			{
				ImGui::TextDisabled("<instance unavailable>");
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	// GetActiveStateInstances hands back a fresh WrenHandle* per call - release once done with them this frame,
	// same handle-hygiene convention used everywhere else in this codebase (e.g. every wrenMakeCallHandle above).
	if (vm)
	{
		for (WrenHandle* handle : instances)
		{
			if (handle)
			{
				wrenReleaseHandle(vm, handle);
			}
		}
	}

	ImGui::End();
}
}  // namespace Struktur::Debug
