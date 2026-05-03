#include "wren.hpp"

#include "Debug/Assertions.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueHelperFunctions.h"

// ============================================================================
// VARIABLE SUBSTITUTION REGISTRATION
// ============================================================================

// VariableSubstitution.applyModifiers(text, modifierChain)
void wren_VariableSubstitutionApplyModifiers(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::VariableSubstitutionSystem& variableSubstitutionSystem = context->GetVariableSubstitutionSystem();

    Struktur::Dialogue::DialogueValue value = Struktur::Dialogue::HelperFunctions::DialogueParseWrenDialogueValue(vm, 1);

    const char* modifierChain = wrenGetSlotString(vm, 2);

    std::string processedString = variableSubstitutionSystem.ApplyModifiers(*context, value, modifierChain);

	wrenSetSlotString(vm, 0, processedString.c_str());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// FileSystem static methods
WREN_CLASS_STATIC("dialogue", "VariableSubstitution", "applyModifiers(_,_)", wren_VariableSubstitutionApplyModifiers, "Process the value through modifiers.");
