#pragma once

#include "EditorWindow.h"

namespace Struktur::Debug
{
class GameViewportWindow;

class DebugInfoWindow : public EditorWindow
{
   public:
	DebugInfoWindow(GameViewportWindow* viewportWindow)
	    : EditorWindow("Debug Info"),
	      m_viewportWindow(viewportWindow)
	{
	}

	void Render(GameContext& context) override;

   private:
	GameViewportWindow* m_viewportWindow;
};
}  // namespace Struktur::Debug
