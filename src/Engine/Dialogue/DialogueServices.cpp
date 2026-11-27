#include "DialogueServices.h"

namespace Struktur::Dialogue
{
Services::Services()
{
	dialogueManager            = std::make_unique<DialogueManager>();
	dialogueRegistry           = std::make_unique<DialogueRegistry>();
	variableSubstitutionSystem = std::make_unique<VariableSubstitutionSystem>();
}

void Services::Shutdown()
{
	dialogueManager.reset();
	dialogueRegistry.reset();
	variableSubstitutionSystem.reset();
}
}  // namespace Struktur::Dialogue
