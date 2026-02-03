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
		virtual bool Evaluate(GameContext& context) const = 0;
	};
}