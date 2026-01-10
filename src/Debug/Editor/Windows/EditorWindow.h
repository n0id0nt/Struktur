#pragma once

#include <string>
#include <imgui.h>

namespace Struktur
{
	class GameContext;

	namespace Debug
	{
		// Base class for all editor windows
		class EditorWindow
		{
		public:
			EditorWindow(const std::string& name, const std::string& dockName = "")
				: m_name(name)
				, m_dockName(dockName.empty() ? name : dockName)
				, m_isOpen(true)
				, m_isVisible(true)
			{
			}

			virtual ~EditorWindow() = default;

			// Render the window (must be implemented by derived classes)
			virtual void Render(GameContext& context) = 0;

			// Optional: Called once when the editor initialises
			virtual void Initialise(GameContext& context) {}

			// Optional: Called once when the editor shuts down
			virtual void Shutdown(GameContext& context) {}

			// Window properties
			const std::string& GetName() const { return m_name; }
			const std::string& GetDockName() const { return m_dockName; }

			bool IsOpen() const { return m_isOpen; }
			void SetOpen(bool open) { m_isOpen = open; }

			bool IsVisible() const { return m_isVisible; }
			void SetVisible(bool visible) { m_isVisible = visible; }

			// Toggle visibility
			void ToggleVisible() { m_isVisible = !m_isVisible; }

		protected:
			std::string m_name;      // Display name
			std::string m_dockName;  // Name used for docking
			bool m_isOpen;           // Whether window is open (has close button)
			bool m_isVisible;        // Whether window should be rendered
		};
	}
}