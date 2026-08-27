
#include "FileExplorerWindow.h"

#include <algorithm>

#include "Debug/Assertions.h"
#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Resource/ResourceManager.h"
#include "PreviewWindow.h"

namespace Struktur::Debug
{
namespace
{
// Pure string manipulation (no disk access) over a PhysFS-style, no-leading-slash path (see FileExplorerWindow's
// own comment on the convention) - deliberately not std::filesystem::path, which reasons about OS separators/
// roots that don't apply to the mounted virtual filesystem's own "" root (see FileSystem::Mount).
std::string ParentPath(const std::string& path)
{
	size_t slash = path.find_last_of('/');
	return slash == std::string::npos ? "" : path.substr(0, slash);
}
}  // namespace

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
		if (!m_currentPath.empty() && m_currentPath != m_assetsPath)
		{
			m_currentPath = ParentPath(m_currentPath);
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

	// PhysFS-backed listing (see FileSystem::ListDirectory's own comment for why this isn't
	// std::filesystem::directory_iterator) - names only, so each child's full path is built by joining onto
	// m_currentPath here.
	for (const std::string& name : FileSystem::ListDirectory(m_currentPath))
	{
		FileEntry fileEntry;
		fileEntry.name        = name;
		fileEntry.path        = m_currentPath.empty() ? name : m_currentPath + "/" + name;
		fileEntry.isDirectory = FileSystem::IsDirectory(fileEntry.path);

		if (!fileEntry.isDirectory)
		{
			auto sizeResult    = FileSystem::GetFileSize(fileEntry.path);
			fileEntry.fileSize = sizeResult ? (size_t)sizeResult.value : 0;
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
	//
	// Images: decodable by stb_image (see TextureResource::LoadFromDisk) - fetched through the resource manager
	// (not read directly) so this reuses whatever's already cached and the preview stays alive via the
	// ResourcePtr TexturePreviewRenderer holds, same pattern as ResourceManagerWindow's texture-row preview.
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".gif" ||
	    ext == ".psd" || ext == ".pic" || ext == ".hdr")
	{
		auto texture = context.GetResourceManager().GetTexture(context, file.path);
		PreviewTexture(m_previewWindow, texture, file.name);
	}
	else if (ext == ".txt" || ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".cs" ||
	         ext == ".lua" || ext == ".py" || ext == ".wren" || ext == ".fs" || ext == ".vs" || ext == ".json" ||
	         ext == ".xml" || ext == ".glsl" || ext == ".frag" || ext == ".vert" || ext == ".sc" || ext == ".hlsl" ||
	         ext == ".md" || ext == ".ldtk" || ext == ".ini" || ext == ".cfg" || ext == ".yaml" || ext == ".yml" ||
	         ext == ".toml" || ext == ".csv")
	{
		PreviewTextFile(m_previewWindow, file.path, file.name);
	}
	else
	{
		// Genuinely unsupported today - no renderer decodes these (see UnknownFilePreviewRenderer): audio
		// (.wav/.ogg - no audio-preview renderer exists), Aseprite's proprietary binary sprite format
		// (.aseprite - not a stb_image-decodable image despite being image-adjacent), and font binaries (.ttf -
		// no glyph-rendering preview exists). Not a fixable case in OnFileSelected itself; would need a new
		// PreviewRenderer subclass per format.
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
