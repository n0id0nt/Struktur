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
		virtual ~Command() = default;

		// Execute the command in the given context
		virtual void Execute(GameContext& context) = 0;
	};
}