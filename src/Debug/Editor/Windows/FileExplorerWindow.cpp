#include "FileExplorerWindow.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

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

std::string ToLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

std::string ToUpper(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	return s;
}

bool IsImageExtension(const std::string& e)
{
	return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" || e == ".tga" || e == ".gif" || e == ".psd" ||
	       e == ".pic" || e == ".hdr";
}

bool IsTextExtension(const std::string& e)
{
	return e == ".txt" || e == ".cpp" || e == ".h" || e == ".hpp" || e == ".c" || e == ".cs" || e == ".lua" ||
	       e == ".py" || e == ".wren" || e == ".fs" || e == ".vs" || e == ".json" || e == ".xml" || e == ".glsl" ||
	       e == ".frag" || e == ".vert" || e == ".sc" || e == ".hlsl" || e == ".md" || e == ".ldtk" || e == ".ini" ||
	       e == ".cfg" || e == ".yaml" || e == ".yml" || e == ".toml" || e == ".csv";
}

FileCategory Categorize(bool isDirectory, const std::string& e)
{
	if (isDirectory)
	{
		return FileCategory::Folder;
	}
	if (IsImageExtension(e))
	{
		return FileCategory::Image;
	}
	if (e == ".wav" || e == ".ogg" || e == ".mp3" || e == ".flac")
	{
		return FileCategory::Audio;
	}
	if (e == ".ttf" || e == ".otf")
	{
		return FileCategory::Font;
	}
	if (e == ".aseprite" || e == ".ase")
	{
		return FileCategory::Aseprite;
	}
	if (e == ".json" || e == ".xml" || e == ".ldtk" || e == ".csv" || e == ".toml" || e == ".yaml" || e == ".yml" ||
	    e == ".ini" || e == ".cfg")
	{
		return FileCategory::Data;
	}
	if (IsTextExtension(e))
	{
		return FileCategory::Text;
	}
	return FileCategory::Other;
}

ImU32 CategoryColor(FileCategory category)
{
	switch (category)
	{
		case FileCategory::Folder:
			return IM_COL32(220, 200, 90, 255);
		case FileCategory::Image:
			return IM_COL32(220, 110, 150, 255);
		case FileCategory::Text:
			return IM_COL32(150, 155, 165, 255);
		case FileCategory::Audio:
			return IM_COL32(110, 190, 130, 255);
		case FileCategory::Font:
			return IM_COL32(120, 160, 230, 255);
		case FileCategory::Data:
			return IM_COL32(200, 150, 90, 255);
		case FileCategory::Aseprite:
			return IM_COL32(190, 120, 210, 255);
		case FileCategory::Other:
		default:
			return IM_COL32(130, 130, 140, 255);
	}
}

const char* CategoryLabel(FileCategory category)
{
	switch (category)
	{
		case FileCategory::Folder:
			return "Folder";
		case FileCategory::Image:
			return "Image";
		case FileCategory::Text:
			return "Text";
		case FileCategory::Audio:
			return "Audio";
		case FileCategory::Font:
			return "Font";
		case FileCategory::Data:
			return "Data";
		case FileCategory::Aseprite:
			return "Aseprite";
		case FileCategory::Other:
		default:
			return "File";
	}
}

std::string GetExtensionLower(const std::string& filename)
{
	size_t dotPos = filename.find_last_of('.');
	if (dotPos != std::string::npos && dotPos > 0 && dotPos < filename.length() - 1)
	{
		return ToLower(filename.substr(dotPos));
	}
	return "";
}

ImU32 Recolor(ImU32 rgb, unsigned alpha)
{
	return (rgb & 0x00FFFFFFu) | (alpha << 24);
}
}  // namespace

void FileExplorerWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	RenderToolbar();
	ImGui::Separator();

	ImGui::BeginChild("##grid", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false);
	RenderGrid(context);
	ImGui::EndChild();

	ImGui::Separator();
	RenderStatusBar();

	// Backspace navigates up a level while the window (or its grid child) has focus - matches OS file browsers.
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Backspace))
	{
		NavigateUp();
	}

	ImGui::End();
}

void FileExplorerWindow::SetAssetsPath(const std::string& path)
{
	m_assetsPath  = path;
	m_currentPath = path;
	m_thumbnailCache.clear();
	m_thumbnailOrder.clear();
	RefreshFileList();
}

void FileExplorerWindow::NavigateTo(const std::string& path)
{
	m_currentPath = path;
	RefreshFileList();
}

void FileExplorerWindow::NavigateUp()
{
	if (!m_currentPath.empty() && m_currentPath != m_assetsPath)
	{
		NavigateTo(ParentPath(m_currentPath));
	}
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
		fileEntry.extension   = fileEntry.isDirectory ? "" : GetExtensionLower(name);
		fileEntry.category    = Categorize(fileEntry.isDirectory, fileEntry.extension);

		if (!fileEntry.isDirectory)
		{
			auto sizeResult    = FileSystem::GetFileSize(fileEntry.path);
			fileEntry.fileSize = sizeResult ? static_cast<size_t>(sizeResult.value) : 0;
		}
		else
		{
			fileEntry.fileSize = 0;
		}

		m_files.push_back(fileEntry);
	}

	// Directories first, then case-insensitive by name.
	std::sort(m_files.begin(), m_files.end(),
	          [](const FileEntry& a, const FileEntry& b)
	          {
		          if (a.isDirectory != b.isDirectory)
		          {
			          return a.isDirectory;
		          }
		          return ToLower(a.name) < ToLower(b.name);
	          });
}

bool FileExplorerWindow::PassesFilter(const FileEntry& file) const
{
	if (m_searchBuffer[0] == '\0')
	{
		return true;
	}
	return ToLower(file.name).find(ToLower(m_searchBuffer)) != std::string::npos;
}

void FileExplorerWindow::RenderToolbar()
{
	if (ImGui::Button("Up"))
	{
		NavigateUp();
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		m_thumbnailCache.clear();
		m_thumbnailOrder.clear();
		RefreshFileList();
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("|");
	ImGui::SameLine();

	// Clickable breadcrumb trail: a root button, then one button per path segment.
	if (ImGui::SmallButton(m_assetsPath.empty() ? "assets" : m_assetsPath.c_str()))
	{
		NavigateTo(m_assetsPath);
	}

	if (!m_currentPath.empty() && m_currentPath != m_assetsPath)
	{
		std::string relative = m_currentPath;
		if (!m_assetsPath.empty() && relative.rfind(m_assetsPath + "/", 0) == 0)
		{
			relative = relative.substr(m_assetsPath.size() + 1);
		}

		std::string accumulated = m_assetsPath;
		size_t start            = 0;
		int segmentIndex        = 0;
		while (start <= relative.size())
		{
			size_t slash        = relative.find('/', start);
			std::string segment = relative.substr(start, slash - start);
			if (!segment.empty())
			{
				accumulated = accumulated.empty() ? segment : accumulated + "/" + segment;

				ImGui::SameLine(0.0f, 2.0f);
				ImGui::TextUnformatted("/");
				ImGui::SameLine(0.0f, 2.0f);
				ImGui::PushID(segmentIndex++);
				if (ImGui::SmallButton(segment.c_str()))
				{
					NavigateTo(accumulated);
				}
				ImGui::PopID();
			}
			if (slash == std::string::npos)
			{
				break;
			}
			start = slash + 1;
		}
	}

	// Search + view controls.
	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputTextWithHint("##search", "Filter by name...", m_searchBuffer, sizeof(m_searchBuffer));
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		m_searchBuffer[0] = '\0';
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat("Size", &m_iconSize, 32.0f, 160.0f, "%.0f");
	ImGui::SameLine();
	ImGui::Checkbox("Thumbnails", &m_showThumbnails);
}

Resource::ResourcePtr<Resource::TextureResource> FileExplorerWindow::GetThumbnail(GameContext& context,
                                                                                 const std::string& path)
{
	auto it = m_thumbnailCache.find(path);
	if (it != m_thumbnailCache.end())
	{
		auto orderIt = std::find(m_thumbnailOrder.begin(), m_thumbnailOrder.end(), path);
		if (orderIt != m_thumbnailOrder.end())
		{
			m_thumbnailOrder.erase(orderIt);
		}
		m_thumbnailOrder.push_back(path);
		return it->second;
	}

	Resource::ResourcePtr<Resource::TextureResource> texture = context.GetResourceManager().GetTexture(context, path);
	if (texture.IsValid())
	{
		texture.EnsureReady(context);
	}

	m_thumbnailCache[path] = texture;
	m_thumbnailOrder.push_back(path);

	if (m_thumbnailOrder.size() > kMaxThumbnails)
	{
		std::string evict = m_thumbnailOrder.front();
		m_thumbnailOrder.erase(m_thumbnailOrder.begin());
		m_thumbnailCache.erase(evict);
	}

	return texture;
}

void FileExplorerWindow::RenderGrid(GameContext& context)
{
	const float pad    = 8.0f;
	const float cellW  = m_iconSize + pad * 2.0f;
	const float labelH = ImGui::GetFontSize() * 2.0f + 4.0f;
	const float cellH  = pad + m_iconSize + 4.0f + labelH + pad;

	float availWidth = ImGui::GetContentRegionAvail().x;
	int itemsPerRow  = std::max(1, static_cast<int>((availWidth + pad) / (cellW + pad)));

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	bool navigated = false;
	std::string navigateTarget;
	bool previewRequested = false;
	std::string previewPath;

	int column = 0;
	auto drawCell = [&](const std::string& idKey, const std::string& path, const std::string& name,
	                    FileCategory category, bool isDirectory, size_t fileSize, bool isParentEntry)
	{
		if (column > 0)
		{
			ImGui::SameLine();
		}
		column = (column + 1) % itemsPerRow;

		ImGui::PushID(idKey.c_str());
		ImVec2 cellMin = ImGui::GetCursorScreenPos();
		bool visible   = ImGui::IsRectVisible(ImVec2(cellW, cellH));
		bool selected  = !isParentEntry && (m_selectedFile == path);

		if (ImGui::Selectable("##cell", selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cellW, cellH)))
		{
			bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
			if (isParentEntry)
			{
				navigated      = true;
				navigateTarget = ParentPath(m_currentPath);
			}
			else if (isDirectory)
			{
				m_selectedFile = path;
				if (doubleClicked)
				{
					navigated      = true;
					navigateTarget = path;
				}
			}
			else
			{
				m_selectedFile   = path;
				previewRequested = true;
				previewPath      = path;
			}
		}

		if (!isParentEntry && ImGui::BeginPopupContextItem("##ctx"))
		{
			m_selectedFile = path;
			if (ImGui::MenuItem("Copy Path"))
			{
				ImGui::SetClipboardText(path.c_str());
			}
			if (ImGui::MenuItem("Copy Name"))
			{
				ImGui::SetClipboardText(name.c_str());
			}
			ImGui::Separator();
			if (isDirectory && ImGui::MenuItem("Open"))
			{
				navigated      = true;
				navigateTarget = path;
			}
			if (!isDirectory && ImGui::MenuItem("Preview"))
			{
				previewRequested = true;
				previewPath      = path;
			}
			if (ImGui::MenuItem("Refresh Folder"))
			{
				m_thumbnailCache.clear();
				m_thumbnailOrder.clear();
				RefreshFileList();
			}
			ImGui::EndPopup();
		}

		if (ImGui::IsItemHovered() && !isParentEntry)
		{
			std::string tip = name + "\n" + CategoryLabel(category);
			if (!isDirectory)
			{
				tip += "  -  " + FormatFileSize(fileSize);
			}
			ImGui::SetTooltip("%s", tip.c_str());
		}

		// --- overlay: draw-list only, so nothing can steal the Selectable's clicks ---
		ImVec2 iconMin(cellMin.x + (cellW - m_iconSize) * 0.5f, cellMin.y + pad);
		ImVec2 iconMax(iconMin.x + m_iconSize, iconMin.y + m_iconSize);
		ImU32 catColor = isParentEntry ? IM_COL32(180, 180, 180, 255) : CategoryColor(category);

		bool drewThumb = false;
		if (visible && m_showThumbnails && category == FileCategory::Image)
		{
			auto texture = GetThumbnail(context, path);
			if (texture.IsValid() && texture.IsReady() && texture->GetWidth() > 0 && texture->GetHeight() > 0)
			{
				float tw    = static_cast<float>(texture->GetWidth());
				float th    = static_cast<float>(texture->GetHeight());
				float scale = std::min(m_iconSize / tw, m_iconSize / th);
				ImVec2 drawSize(tw * scale, th * scale);
				ImVec2 tMin(iconMin.x + (m_iconSize - drawSize.x) * 0.5f,
				            iconMin.y + (m_iconSize - drawSize.y) * 0.5f);
				drawList->AddRectFilled(iconMin, iconMax, IM_COL32(0, 0, 0, 90), 4.0f);
				drawList->AddImage((ImTextureID)(intptr_t)texture->GetHandle().id, tMin,
				                   ImVec2(tMin.x + drawSize.x, tMin.y + drawSize.y));
				drewThumb = true;
			}
		}

		if (!drewThumb)
		{
			drawList->AddRectFilled(iconMin, iconMax, Recolor(catColor, 60), 4.0f);
			drawList->AddRect(iconMin, iconMax, Recolor(catColor, 200), 4.0f);

			std::string glyph;
			if (isParentEntry)
			{
				glyph = "..";
			}
			else if (isDirectory)
			{
				glyph = "DIR";
			}
			else
			{
				size_t dot = name.find_last_of('.');
				glyph      = (dot != std::string::npos && dot + 1 < name.size()) ? ToUpper(name.substr(dot + 1)) : "?";
				if (glyph.size() > 4)
				{
					glyph = glyph.substr(0, 4);
				}
			}
			ImVec2 gs = ImGui::CalcTextSize(glyph.c_str());
			drawList->AddText(ImVec2(iconMin.x + (m_iconSize - gs.x) * 0.5f, iconMin.y + (m_iconSize - gs.y) * 0.5f),
			                  Recolor(catColor, 255), glyph.c_str());
		}

		// filename, clipped to the cell so long names never bleed into the next row
		ImVec2 labelMin(cellMin.x + 3.0f, iconMax.y + 3.0f);
		ImVec2 labelMax(cellMin.x + cellW - 3.0f, cellMin.y + cellH - 2.0f);
		drawList->PushClipRect(labelMin, labelMax, true);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), labelMin, ImGui::GetColorU32(ImGuiCol_Text),
		                  name.c_str(), nullptr, cellW - 6.0f);
		drawList->PopClipRect();

		ImGui::PopID();
	};

	if (!m_currentPath.empty() && m_currentPath != m_assetsPath)
	{
		drawCell("##parent", "##parent", "..", FileCategory::Folder, true, 0, true);
	}

	for (const auto& file : m_files)
	{
		if (!PassesFilter(file))
		{
			continue;
		}
		drawCell(file.path, file.path, file.name, file.category, file.isDirectory, file.fileSize, false);
	}

	if (navigated)
	{
		NavigateTo(navigateTarget);
	}
	else if (previewRequested)
	{
		for (const auto& file : m_files)
		{
			if (file.path == previewPath)
			{
				OnFileSelected(file, context);
				break;
			}
		}
	}
}

void FileExplorerWindow::RenderStatusBar()
{
	int shown = 0;
	for (const auto& file : m_files)
	{
		if (PassesFilter(file))
		{
			++shown;
		}
	}

	if (m_searchBuffer[0] != '\0')
	{
		ImGui::Text("%d of %zu items", shown, m_files.size());
	}
	else
	{
		ImGui::Text("%d items", shown);
	}

	if (!m_selectedFile.empty())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();
		ImGui::TextUnformatted(m_selectedFile.c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Copy"))
		{
			ImGui::SetClipboardText(m_selectedFile.c_str());
		}
	}
}

void FileExplorerWindow::OnFileSelected(const FileEntry& file, GameContext& context)
{
	if (!m_previewWindow)
	{
		return;
	}

	const std::string& ext = file.extension;  // already lower-cased in RefreshFileList

	if (IsImageExtension(ext))
	{
		auto texture = context.GetResourceManager().GetTexture(context, file.path);
		PreviewTexture(m_previewWindow, texture, file.name);
	}
	else if (IsTextExtension(ext))
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

std::string FileExplorerWindow::FormatFileSize(size_t bytes)
{
	const char* units[] = {"B", "KB", "MB", "GB"};
	int unitIndex       = 0;
	double size         = static_cast<double>(bytes);

	while (size >= 1024.0 && unitIndex < 3)
	{
		size /= 1024.0;
		unitIndex++;
	}

	char buffer[32];
	std::snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
	return std::string(buffer);
}
}  // namespace Struktur::Debug
