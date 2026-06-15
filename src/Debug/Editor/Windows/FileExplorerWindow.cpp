
#include "FileExplorerWindow.h"

#include <algorithm>

#include "Debug/Assertions.h"
#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include "Engine/GameContext.h"
#include "PreviewWindow.h"

namespace Struktur::Debug
{
void FileExplorerWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	// Toolbar
	if (ImGui::Button("< Back"))
	{
		std::filesystem::path current(m_currentPath);
		if (current.has_parent_path() && current != m_assetsPath)
		{
			m_currentPath = current.parent_path().string();
			RefreshFileList();
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Refresh"))
	{
		RefreshFileList();
	}
	ImGui::SameLine();

	ImGui::Text("Path: %s", m_currentPath.c_str());

	ImGui::Separator();

	// File list
	// RenderFileList(context);
	RenderFileGrid(context);

	ImGui::End();
}

void FileExplorerWindow::SetAssetsPath(const std::string& path)
{
	m_assetsPath  = path;
	m_currentPath = path;
	RefreshFileList();
}

void FileExplorerWindow::RefreshFileList()
{
	m_files.clear();

	try
	{
		for (const auto& entry : std::filesystem::directory_iterator(m_currentPath))
		{
			FileEntry fileEntry;
			fileEntry.name        = entry.path().filename().string();
			fileEntry.path        = entry.path().string();
			fileEntry.isDirectory = entry.is_directory();

			if (!fileEntry.isDirectory)
			{
				fileEntry.fileSize  = std::filesystem::file_size(entry.path());
				fileEntry.extension = GetFileExtension(fileEntry.name);
			}
			else
			{
				fileEntry.fileSize  = 0;
				fileEntry.extension = "";
			}

			m_files.push_back(fileEntry);
		}

		// Sort: directories first, then files alphabetically
		std::sort(m_files.begin(), m_files.end(),
		          [](const FileEntry& a, const FileEntry& b)
		          {
			          if (a.isDirectory != b.isDirectory)
			          {
				          return a.isDirectory;
			          }
			          return a.name < b.name;
		          });
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		// Handle error silently or log
	}
}

void FileExplorerWindow::RenderFileList(GameContext& context)
{
	// Create table for file list
	if (ImGui::BeginTable("FileTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableHeadersRow();

		bool newFolderSelected        = false;
		bool newFileSelected          = false;
		const FileEntry* selectedFile = nullptr;

		for (const auto& file : m_files)
		{
			ImGui::TableNextRow();

			// Name column
			ImGui::TableSetColumnIndex(0);

			const char* icon = file.isDirectory ? "[DIR]" : "[FILE]";
			bool isSelected  = (m_selectedFile == file.path);

			ImGui::PushID(file.path.c_str());
			if (ImGui::Selectable(icon, isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				if (file.isDirectory)
				{
					// Navigate into directory
					m_currentPath     = file.path;
					newFolderSelected = true;
				}
				else
				{
					// Select file and preview
					m_selectedFile  = file.path;
					selectedFile    = &file;
					newFileSelected = true;
				}
			}
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::Text("%s", file.name.c_str());

			// Type column
			ImGui::TableSetColumnIndex(1);
			if (file.isDirectory)
			{
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Folder");
			}
			else
			{
				ImGui::Text("%s", file.extension.c_str());
			}

			// Size column
			ImGui::TableSetColumnIndex(2);
			if (!file.isDirectory)
			{
				ImGui::Text("%s", FormatFileSize(file.fileSize).c_str());
			}
		}

		ImGui::EndTable();

		if (newFolderSelected)
		{
			RefreshFileList();
		}
		else if (newFileSelected)
		{
			OnFileSelected(*selectedFile, context);
		}
	}
}

void FileExplorerWindow::RenderFileGrid(GameContext& context)
{
	// Configuration
	const float itemWidth  = 100.0f;
	const float itemHeight = 120.0f;
	const float iconSize   = 64.0f;
	const float padding    = 10.0f;

	// Calculate how many items fit per row
	float availWidth = ImGui::GetContentRegionAvail().x;
	int itemsPerRow  = std::max(1, (int)((availWidth + padding) / (itemWidth + padding)));

	bool newFolderSelected        = false;
	bool newFileSelected          = false;
	const FileEntry* selectedFile = nullptr;

	// Render grid
	int itemIndex = 0;
	for (const auto& file : m_files)
	{
		// Start new row if needed
		if (itemIndex > 0 && itemIndex % itemsPerRow != 0)
		{
			ImGui::SameLine();
		}

		ImGui::PushID(file.path.c_str());

		// Begin item group
		ImVec2 cursorPos = ImGui::GetCursorPos();
		bool isSelected  = (m_selectedFile == file.path);

		// Draw selection background
		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.5f, 0.8f, 0.3f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		}

		// Create clickable area
		ImGui::BeginChild(("item_" + std::to_string(itemIndex)).c_str(), ImVec2(itemWidth, itemHeight), false,
		                  ImGuiWindowFlags_NoScrollbar);

		// Make entire area selectable
		bool clicked = ImGui::InvisibleButton("##select", ImVec2(itemWidth - padding, itemHeight - padding));

		// Draw icon/type indicator
		ImGui::SetCursorPos(ImVec2((itemWidth - iconSize) * 0.5f, padding));

		if (file.isDirectory)
		{
			// Directory icon (colored box)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.2f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.3f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.1f, 0.8f));
			ImGui::Button("[DIR]", ImVec2(iconSize, iconSize));
			ImGui::PopStyleColor(3);
		}
		else
		{
			// File icon (colored box based on extension)
			ImVec4 color = GetFileTypeColor(file.extension);
			ImGui::PushStyleColor(ImGuiCol_Button, color);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
			                      ImVec4(color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, color.w));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,
			                      ImVec4(color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, color.w));

			// Show extension on icon
			ImGui::Button(file.extension.c_str(), ImVec2(iconSize, iconSize));
			ImGui::PopStyleColor(3);
		}

		// Draw filename (wrapped text)
		ImGui::SetCursorPosX(padding * 0.5f);
		ImGui::PushTextWrapPos(itemWidth - padding);
		ImGui::TextWrapped("%s", file.name.c_str());
		ImGui::PopTextWrapPos();

		// Draw file size for files
		if (!file.isDirectory)
		{
			ImGui::SetCursorPosX(padding * 0.5f);
			ImGui::TextDisabled("%s", FormatFileSize(file.fileSize).c_str());
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();  // ChildBg

		// Handle click
		if (clicked)
		{
			if (file.isDirectory)
			{
				m_currentPath     = file.path;
				newFolderSelected = true;
			}
			else
			{
				m_selectedFile  = file.path;
				selectedFile    = &file;
				newFileSelected = true;
			}
		}

		ImGui::PopID();
		itemIndex++;
	}

	// Handle selection events after rendering
	if (newFolderSelected)
	{
		RefreshFileList();
	}
	else if (newFileSelected)
	{
		OnFileSelected(*selectedFile, context);
	}
}

void FileExplorerWindow::OnFileSelected(const FileEntry& file, GameContext& context)
{
	if (!m_previewWindow)
	{
		return;
	}

	std::string ext = file.extension;
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// Determine file type and preview accordingly
	// Improve way this is handled. load this from somewhere
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
	{
		PreviewUnknownFile(m_previewWindow, file.name);
	}
	else if (ext == ".txt" || ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".cs" ||
	         ext == ".lua" || ext == ".py" || ext == ".wren" || ext == ".fs" || ext == ".vs" || ext == ".json" ||
	         ext == ".xml" || ext == ".glsl" || ext == ".frag" || ext == ".vert")
	{
		PreviewTextFile(m_previewWindow, file.path, file.name);
	}
	else
	{
		PreviewUnknownFile(m_previewWindow, file.name);
	}
}

std::string FileExplorerWindow::GetFileExtension(const std::string& filename)
{
	size_t dotPos = filename.find_last_of('.');
	if (dotPos != std::string::npos && dotPos < filename.length() - 1)
	{
		return filename.substr(dotPos);
	}
	return "";
}

std::string FileExplorerWindow::FormatFileSize(size_t bytes)
{
	const char* units[] = {"B", "KB", "MB", "GB"};
	int unitIndex       = 0;
	double size         = (double)bytes;

	while (size >= 1024.0 && unitIndex < 3)
	{
		size /= 1024.0;
		unitIndex++;
	}

	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
	return std::string(buffer);
}
ImVec4 FileExplorerWindow::GetFileTypeColor(const std::string& extension)
{
	if (extension == ".txt" || extension == ".md")
	{
		return ImVec4(0.6f, 0.6f, 0.6f, 0.6f);  // Gray for text
	}
	else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
	{
		return ImVec4(0.9f, 0.4f, 0.6f, 0.6f);  // Pink for images
	}
	else if (extension == ".wren")
	{
		return ImVec4(0.8f, 0.3f, 0.8f, 0.6f);  // Purple for Wren scripts
	}
	else
	{
		return ImVec4(0.5f, 0.5f, 0.5f, 0.6f);  // Default gray
	}
}
}  // namespace Struktur::Debug
