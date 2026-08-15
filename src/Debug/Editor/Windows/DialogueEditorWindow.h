// DialogueEditorWindow.h
// Main dialogue editor window with node graph, editor panels, and playback testing
// Part of the Struktur editor

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Debug/Editor/Exporters/DialogueExporter.h"
#include "Debug/Editor/Windows/EditorWindow.h"
#include "Engine/Dialogue/DialogueNode.h"
#include "Engine/Dialogue/DialogueResult.h"
#include "glm/glm.hpp"
#include "wren.hpp"

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
	void LoadDialogueFile(GameContext& context, const std::string& filepath, const std::string& className,
	                      const std::string& entryNode);
	void SaveDialogueFile(const std::string& filepath, Dialogue::DialogueExporter::DialogueSaveFormat format);
	void CreateNewDialogue(const std::string& className);

	// Node operations
	void SelectNode(const std::string& nodeId);
	void AddNode(const std::string& nodeId);
	void DeleteNode(const std::string& nodeId);
	void DuplicateNode(const std::string& nodeId);

	// Playback (sandboxed - see ProcessPlaybackNode)
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

	// File operations
	bool LoadDialogueFileWren(GameContext& context, const std::string& filepath, const std::string& className);
	bool LoadDialogueFileJson(GameContext& context, const std::string& filepath);
	void ParseDialogueDataFromJson(GameContext& context, const nlohmann::json& root);
	std::unique_ptr<Dialogue::Command> ParseCommandFromJson(const nlohmann::json& j);
	std::unique_ptr<Dialogue::Choice> ParseChoiceFromJson(const nlohmann::json& j);
	std::unique_ptr<Dialogue::ConditionalTarget> ParseTargetFromJson(const nlohmann::json& j);
	static Dialogue::DialogueValue ParseDialogueValueFromJson(const nlohmann::json& value);
	void RenderOpenDialogueOptionsPopup(GameContext& context);

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

	// Sandboxed playback evaluation - never touches real game state, see the comment block above
	// ProcessPlaybackNode's definition for details
	Dialogue::DialogueResult ProcessPlaybackNode(const std::string& nodeId);
	bool EvaluatePlaybackConditions(const Dialogue::ConditionList& conditions);
	bool EvaluatePlaybackCondition(const Dialogue::Condition& condition);
	void ExecutePlaybackCommands(const Dialogue::CommandList& commands);
	void ExecutePlaybackCommand(const Dialogue::Command& command);

	// Helper UI functions
	bool NodeSelector(const char* label, std::string& nodeId);
	bool SpeakerInput(char* buffer, size_t bufferSize);
	void InsertVariableAtCursor(char* buffer, size_t bufferSize, const char* variable);

	void ParseDialogueDataFromWren(GameContext& context, WrenVM* vm, int slot);

	void SaveFileWithPicker();
	void OpenFileWithPicker(GameContext& context);
	static Dialogue::DialogueExporter::DialogueSaveFormat FormatFromExtension(const std::string& filepath);

	// View mode
	ViewMode m_viewMode;

	// Current file info
	std::string m_currentFile;
	std::string m_currentClassName;
	std::string m_currentModulePath;
	bool m_hasUnsavedChanges;

	unsigned int m_importCount;  // used to ensure dialogue is

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

	Dialogue::DialogueExporter::DialogueSaveFormat m_currentSaveFormat =
	    Dialogue::DialogueExporter::DialogueSaveFormat::Wren;

	// Pending load state for the open options popup
	std::string m_pendingLoadFilepath;
	std::string m_pendingLoadClassName;
	std::string m_pendingLoadEntryNode;
};
}  // namespace Struktur::Debug
