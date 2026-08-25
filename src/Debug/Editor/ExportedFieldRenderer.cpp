#include "ExportedFieldRenderer.h"

#include <imgui.h>

#include <any>
#include <array>
#include <cstring>
#include <memory>
#include <vector>

namespace Struktur::Debug
{
namespace
{
class NumFieldRenderer : public IExportedFieldRenderer
{
public:
	WrenType GetHandledType() const override
	{
		return WREN_TYPE_NUM;
	}

	bool Render(const std::string& label, const Wren::WrenItem& value, bool editable,
	           Wren::WrenItem& out_newValue) const override
	{
		float numValue = static_cast<float>(std::any_cast<double>(value.value));
		ImGui::BeginDisabled(!editable);
		bool changed = ImGui::DragFloat(label.c_str(), &numValue, 0.1f);
		ImGui::EndDisabled();
		if (changed)
		{
			out_newValue.type  = WREN_TYPE_NUM;
			out_newValue.value = static_cast<double>(numValue);
		}
		return changed;
	}
};

class BoolFieldRenderer : public IExportedFieldRenderer
{
public:
	WrenType GetHandledType() const override
	{
		return WREN_TYPE_BOOL;
	}

	bool Render(const std::string& label, const Wren::WrenItem& value, bool editable,
	           Wren::WrenItem& out_newValue) const override
	{
		bool boolValue = std::any_cast<bool>(value.value);
		ImGui::BeginDisabled(!editable);
		bool changed = ImGui::Checkbox(label.c_str(), &boolValue);
		ImGui::EndDisabled();
		if (changed)
		{
			out_newValue.type  = WREN_TYPE_BOOL;
			out_newValue.value = boolValue;
		}
		return changed;
	}
};

class StringFieldRenderer : public IExportedFieldRenderer
{
public:
	WrenType GetHandledType() const override
	{
		return WREN_TYPE_STRING;
	}

	bool Render(const std::string& label, const Wren::WrenItem& value, bool editable,
	           Wren::WrenItem& out_newValue) const override
	{
		std::string strValue = std::any_cast<std::string>(value.value);
		char buffer[256];
		strncpy(buffer, strValue.c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		ImGui::BeginDisabled(!editable);
		bool changed = ImGui::InputText(label.c_str(), buffer, sizeof(buffer));
		ImGui::EndDisabled();
		if (changed)
		{
			out_newValue.type  = WREN_TYPE_STRING;
			out_newValue.value = std::string(buffer);
		}
		return changed;
	}
};

// One instance of each registered renderer, for the lifetime of the process - these are stateless (each Render
// call is self-contained), so there's no reason to construct/destroy them per call or per window.
const std::vector<std::unique_ptr<IExportedFieldRenderer>>& GetRenderers()
{
	static const std::vector<std::unique_ptr<IExportedFieldRenderer>> renderers = []
	{
		std::vector<std::unique_ptr<IExportedFieldRenderer>> v;
		v.push_back(std::make_unique<NumFieldRenderer>());
		v.push_back(std::make_unique<BoolFieldRenderer>());
		v.push_back(std::make_unique<StringFieldRenderer>());
		return v;
	}();
	return renderers;
}
}  // namespace

bool RenderExportedField(const std::string& label, const Wren::WrenItem& value, bool editable,
                         Wren::WrenItem& out_newValue)
{
	for (const std::unique_ptr<IExportedFieldRenderer>& renderer : GetRenderers())
	{
		if (renderer->GetHandledType() == value.type)
		{
			return renderer->Render(label, value, editable, out_newValue);
		}
	}

	ImGui::Text("%s: <unsupported type>", label.c_str());
	return false;
}
}  // namespace Struktur::Debug
