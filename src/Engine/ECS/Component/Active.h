#pragma once

namespace Struktur
{
	namespace Component
	{
        struct Active
        {
            enum class ActiveState
            {
                Active,
                InactiveSelf,
                InactiveParent,
            } activeState = ActiveState::Active;
        };
    }
}