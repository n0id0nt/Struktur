#pragma once

#include <string>

#include "PreviewRenderer.h"

namespace Struktur::Debug
{
class UnknownFilePreviewRenderer : public IPreviewRenderer
{
   public:
	UnknownFilePreviewRenderer(const std::string& fileName)
	    : m_fileName(fileName)
	{
	}

	void Render(GameContext& context, const ImVec2& availableSize) override;
	std::string GetPreviewName() const override
	{
		return m_fileName;
	}

   private:
	std::string m_fileName;
};
}  // namespace Struktur::Debug
