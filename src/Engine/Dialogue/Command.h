// Command.h
// Base interface for dialogue commands
// Part of the Struktur dialogue system

#pragma once

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	// Base class for all dialogue commands
	class Command
	{
	public:
		virtual void Execute(GameContext& context) const = 0;
	};
}