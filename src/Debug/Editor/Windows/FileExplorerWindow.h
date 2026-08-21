#pragma once

#include <string>
#include <vector>

#include "EditorWindow.h"

namespace Struktur::Debug
{
class PreviewWindow;

struct FileEntry
{
	std::string name;
	std::string path;
	bool isDirectory;
	size_t fileSize;
	std::string extension;
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
	void RenderFileList(GameContext& context);
	void RenderFileGrid(GameContext& context);
	void OnFileSelected(const FileEntry& file, GameContext& context);
	std::string GetFileExtension(const std::string& filename);
	std::string FormatFileSize(size_t bytes);
	ImVec4 GetFileTypeColor(const std::string& extension);

	PreviewWindow* m_previewWindow;
	std::string m_assetsPath;
	std::string m_currentPath;
	std::vector<FileEntry> m_files;
	std::string m_selectedFile;
};
}  // namespace Struktur::Debug
