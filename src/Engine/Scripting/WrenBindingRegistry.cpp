#include "WrenBindingRegistry.h"

namespace Struktur::Wren
{
void RegisterAllBindings(BindingRegistry& registry)
{
	RegisterAnimationBindings(registry);
	RegisterGameObjectBindings(registry);
	RegisterGameObjectComponentBindings(registry);
	RegisterMathBindings(registry);
	RegisterPhysicsBindings(registry);
	RegisterCollisionLayersBindings(registry);
	RegisterResourceManagerBindings(registry);
	RegisterUIBindings(registry);
	RegisterDialogueBindings(registry);
	RegisterFunctionCallbackBindings(registry);
	RegisterFileSystemBindings(registry);
	RegisterFlagsBindings(registry);
	RegisterApplicationBindings(registry);
	RegisterInputBindings(registry);
	RegisterDebugBindings(registry);
	RegisterSerialisationBindings(registry);
	RegisterVariableSubstitutionBindings(registry);
	RegisterEventsBindings(registry);
}
}  // namespace Struktur::Wren
