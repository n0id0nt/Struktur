#pragma once

#include "EditorWindow.h"
#include <string>
#include <vector>
#include <filesystem>

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
		FileExplorerWindow(PreviewWindow* previewWindow, const std::string& assetsPath = "assets")
			: EditorWindow("File Explorer")
			, m_previewWindow(previewWindow)
			, m_assetsPath(assetsPath)
			, m_currentPath(assetsPath)
		{
			RefreshFileList();
		}

		void Render(GameContext& context) override;

		void SetAssetsPath(const std::string& path);

	private:
		void RefreshFileList();
		void RenderFileList(GameContext& context);
		void RenderFileGrid(GameContext& context);
		void RenderDirectoryTree(GameContext& context, const std::filesystem::path& path, int depth = 0);
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
}
