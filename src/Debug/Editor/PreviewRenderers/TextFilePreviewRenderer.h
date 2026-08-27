#pragma once

#include <string>

#include "PreviewRenderer.h"

namespace Struktur::Debug
{
class TextFilePreviewRenderer : public IPreviewRenderer
{
public:
	TextFilePreviewRenderer(const std::string& filePath, const std::string& name)
	    : m_filePath(filePath),
	      m_name(name)
	{
		LoadFile();
	}

	void Render(GameContext& context, const ImVec2& availableSize) override;
	std::string GetPreviewName() const override
	{
		return m_name;
	}

private:
	void LoadFile();

private:
	std::string m_filePath;
	std::string m_name;
	std::string m_fileContent;
};
}  // namespace Struktur::Debug
