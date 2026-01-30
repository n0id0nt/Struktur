// Condition.h
// Base interface for dialogue conditions
// Part of the Struktur dialogue system

#pragma once

#include "DialogueValue.h"

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	// Base class for all dialogue conditions
	class Operator
	{
	public:
		virtual ~Operator() = default;

		virtual bool Evaluate(GameContext& context) const = 0;
	};
}