#pragma once

namespace Struktur::Wren
{
struct BindingRegistry;

// The indoor cycling game's script modules ("ride" and "world"). Registered by the game.
void RegisterRideBindings(BindingRegistry& registry);
void RegisterWorldBindings(BindingRegistry& registry);
}  // namespace Struktur::Wren
