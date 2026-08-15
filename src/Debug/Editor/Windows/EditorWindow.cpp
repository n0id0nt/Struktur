#include "EditorWindow.h"

namespace Struktur::Debug
{
EditorWindow::EditorWindow(const std::string& name, const std::string& dockName)
    : m_name(name),
      m_dockName(dockName.empty() ? name : dockName),
      m_isOpen(true),
      m_isVisible(true)
{
}

EditorWindow::~EditorWindow() = default;

void EditorWindow::Initialise(GameContext& context) {}

void EditorWindow::Shutdown(GameContext& context) {}

const std::string& EditorWindow::GetName() const
{
	return m_name;
}

const std::string& EditorWindow::GetDockName() const
{
	return m_dockName;
}

bool EditorWindow::IsOpen() const
{
	return m_isOpen;
}

void EditorWindow::SetOpen(bool open)
{
	m_isOpen = open;
}

bool EditorWindow::IsVisible() const
{
	return m_isVisible;
}

void EditorWindow::SetVisible(bool visible)
{
	m_isVisible = visible;
}

void EditorWindow::ToggleVisible()
{
	m_isVisible = !m_isVisible;
}
}  // namespace Struktur::Debug