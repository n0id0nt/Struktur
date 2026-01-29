// Condition.h
// Base interface for dialogue conditions
// Part of the Struktur dialogue system

#pragma once

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	// Base class for all dialogue conditions
	class Condition
	{
	public:
		virtual ~Condition() = default;

		virtual bool Evaluate(GameContext& context, const std::map<std::string, DialogueValue>& params) = 0;

		virtual void Dispose(GameContext& context) {}
	};
}