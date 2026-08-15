#pragma once

#include <imgui.h>

#include <string>

namespace Struktur
{
class GameContext;

namespace Debug
{
// Base class for all editor windows
class EditorWindow
{
   public:
	EditorWindow(const std::string& name, const std::string& dockName = "");

	virtual ~EditorWindow();

	// Render the window (must be implemented by derived classes)
	virtual void Render(GameContext& context) = 0;

	// Optional: Called once when the editor initialises
	virtual void Initialise(GameContext& context);

	// Optional: Called once when the editor shuts down
	virtual void Shutdown(GameContext& context);

	// Window properties
	const std::string& GetName() const;
	const std::string& GetDockName() const;

	bool IsOpen() const;
	void SetOpen(bool open);

	bool IsVisible() const;
	void SetVisible(bool visible);

	// Toggle visibility
	void ToggleVisible();

   protected:
	std::string m_name;      // Display name
	std::string m_dockName;  // Name used for docking
	bool m_isOpen;           // Whether window is open (has close button)
	bool m_isVisible;        // Whether window should be rendered
};
}  // namespace Debug
}  // namespace Struktur
