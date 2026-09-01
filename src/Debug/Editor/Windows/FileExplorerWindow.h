#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "EditorWindow.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"

namespace Struktur::Debug
{
class PreviewWindow;

enum class FileCategory
{
	Folder,
	Image,
	Text,
	Audio,
	Font,
	Data,
	Aseprite,
	Other
};

struct FileEntry
{
	std::string name;
	std::string path;
	bool isDirectory;
	size_t fileSize;
	std::string extension;  // lower-cased, includes the leading '.'
	FileCategory category;
};

class FileExplorerWindow : public EditorWindow
{
public:
	// assetsPath is a path in the mounted virtual filesystem (see Engine/Core/FileSystem.h - PhysFS-backed, NOT
	// an OS path), so "" (the mount root - see FileSystem::Mount) is the correct default, matching whatever the
	// user picked via OpenFolderDialog at startup (or the assets/ fallback) rather than an OS-relative "assets"
	// guess that may not exist relative to the process's actual working directory. No leading slash anywhere in
	// this class's paths - matches the convention every other path in this codebase already uses (e.g.
	// Texture.load("Tiles/Items/...")), so a file clicked here resolves to the exact same resource-pool cache
	// key a script loading the same asset would use.
	FileExplorerWindow(PreviewWindow* previewWindow, const std::string& assetsPath = "")
	    : EditorWindow("File Explorer"),
	      m_previewWindow(previewWindow),
	      m_assetsPath(assetsPath),
	      m_currentPath(assetsPath)
	{
		RefreshFileList();
	}

	void Render(GameContext& context) override;

	void SetAssetsPath(const std::string& path);

private:
	void RefreshFileList();
	void RenderToolbar();
	void RenderGrid(GameContext& context);
	void RenderStatusBar();
	void OnFileSelected(const FileEntry& file, GameContext& context);

	void NavigateTo(const std::string& path);
	void NavigateUp();

	bool PassesFilter(const FileEntry& file) const;

	// Bounded LRU cache of GPU-resident thumbnails keyed by virtual path. Holding the ResourcePtr is what keeps
	// the texture alive (see the splash-screen Pin comment in Game.cpp for the same eviction hazard); the cap
	// stops a big asset folder from pinning every image the user scrolls past into VRAM at once.
	Resource::ResourcePtr<Resource::TextureResource> GetThumbnail(GameContext& context, const std::string& path);

	std::string FormatFileSize(size_t bytes);

	PreviewWindow* m_previewWindow;
	std::string m_assetsPath;
	std::string m_currentPath;
	std::vector<FileEntry> m_files;
	std::string m_selectedFile;

	char m_searchBuffer[128] = {};
	float m_iconSize         = 64.0f;
	bool m_showThumbnails    = true;

	static constexpr size_t kMaxThumbnails = 96;
	std::unordered_map<std::string, Resource::ResourcePtr<Resource::TextureResource>> m_thumbnailCache;
	std::vector<std::string> m_thumbnailOrder;
};
}  // namespace Struktur::Debug
