// DialogueEditorWindow.h
// Main dialogue editor window with node graph, editor panels, and playback testing
// Part of the Struktur editor

#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

#include "glm/glm.hpp"
#include "wren.hpp"

#include "Debug/Editor/Windows/EditorWindow.h"
#include "Engine/Dialogue/DialogueNode.h"
#include "Engine/Dialogue/DialogueResult.h"

namespace Struktur::Debug
{
	class PreviewWindow;

	// Validation error/warning structures
	struct ValidationError
	{
		enum class Type
		{
			DuplicateNodeId,
			MissingTargetNode,
			InvalidReference,
			CircularDependency
		};

		Type type;
		std::string nodeId;
		std::string message;
		std::string suggestion;
	};

	struct ValidationWarning
	{
		enum class Type
		{
			UnreachableNode,
			DeadEnd,
			MissingSpeaker,
			EmptyText,
			UnusedCondition
		};

		Type type;
		std::string nodeId;
		std::string message;
	};

	class DialogueEditorWindow : public EditorWindow
	{
	public:
		DialogueEditorWindow(PreviewWindow* previewWindow);

		void Render(GameContext& context) override;

		// File operations
		void LoadDialogueFile(GameContext& context, const std::string& filepath, const std::string& className);
		void SaveDialogueFile(const std::string& filepath);
		void CreateNewDialogue(const std::string& className);

		// Node operations
		void SelectNode(const std::string& nodeId);
		void AddNode(const std::string& nodeId);
		void DeleteNode(const std::string& nodeId);
		void DuplicateNode(const std::string& nodeId);

		// Playback
		void StartPlayback(GameContext& context, const std::string& entryNodeId);
		void StopPlayback(GameContext& context);
		void ContinuePlayback(GameContext& context);
		void MakePlaybackChoice(GameContext& context, int choiceIndex);

	private:
		enum class ViewMode
		{
			Edit,
			Playback
		};

		// Main render sections
		void RenderFilePanel(GameContext& context);
		void RenderGraphView(GameContext& context);
		void RenderNodeEditor(GameContext& context);
		void RenderPlaybackView(GameContext& context);
		void RenderValidationPanel(GameContext& context);

		// Graph view helpers
		void RenderNode(GameContext& context, const std::string& nodeId, ImVec2 position);
		void RenderNodeConnections(GameContext& context, ImVec2 canvasPos);
		void CalculateGraphLayout();

		// Node editor helpers
		void RenderNodeBasicInfo(GameContext& context, Dialogue::DialogueNode* node);
		void RenderNodeChoices(GameContext& context, Dialogue::DialogueNode* node);
		void RenderNodeCommands(GameContext& context, Dialogue::DialogueNode* node);
		void RenderNodeTargets(GameContext& context, Dialogue::DialogueNode* node);

		// Command/Condition parameter editors
		void RenderCommandParameters(GameContext& context, Dialogue::Command* command);
		void RenderConditionParameters(GameContext& context, Dialogue::Condition* condition);

		// Validation
		void ValidateDialogue();

		// Helper UI functions
		bool NodeSelector(const char* label, std::string& nodeId);
		bool SpeakerInput(char* buffer, size_t bufferSize);
		void InsertVariableAtCursor(char* buffer, size_t bufferSize, const char* variable);

		void SwitchNodeContinuationType(Dialogue::DialogueNode* node, int newType);
		void ParseDialogueDataFromWren(GameContext& context, WrenVM* vm, int slot);

		// View mode
		ViewMode m_viewMode;

		// Current file info
		std::string m_currentFile;
		std::string m_currentClassName;
		std::string m_currentModulePath;
		bool m_hasUnsavedChanges;

		unsigned int m_importCount; // used to ensure dialogue is 

		// Node data
		struct NodeData
		{
			std::unique_ptr<Dialogue::DialogueNode> node;
			glm::vec2 visualPosition;
		};
		std::unordered_map<std::string, NodeData> m_nodes;
		std::string m_entryNodeId;
		std::string m_selectedNodeId;

		// Graph view state
		glm::vec2 m_graphPanOffset;
		float m_graphZoom;
		bool m_isDraggingGraph;
		glm::vec2 m_graphDragStart;

		// Playback state
		bool m_isPlaybackActive;
		Dialogue::DialogueResult m_currentPlaybackResult;
		std::unordered_map<std::string, int> m_playbackIntFlags;
		std::unordered_map<std::string, bool> m_playbackBoolFlags;
		std::vector<std::string> m_playbackInventory;
		std::vector<std::string> m_playbackHistory;

		// Validation
		std::vector<ValidationError> m_errors;
		std::vector<ValidationWarning> m_warnings;

		// UI state
		char m_newNodeIdBuffer[128];
		char m_searchBuffer[128];

		PreviewWindow* m_previewWindow;
	};
}
