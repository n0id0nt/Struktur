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

		// Evaluate the condition in the given context
		// Returns true if condition passes, false otherwise
		virtual bool Evaluate(GameContext& context) = 0;
	};
}