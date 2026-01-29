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

		virtual void Execute(GameContext& context, const std::map<std::string, DialogueValue>& params) = 0;

		virtual void Dispose(GameContext& context) {}
	};
}