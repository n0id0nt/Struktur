#pragma once

#include <memory>

#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/TextSystems/VariableSubstitutionSystem.h"

namespace Struktur::Dialogue
{
// The dialogue library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<DialogueManager> dialogueManager;
	std::unique_ptr<DialogueRegistry> dialogueRegistry;
	std::unique_ptr<VariableSubstitutionSystem> variableSubstitutionSystem;
};
}  // namespace Struktur::Dialogue
