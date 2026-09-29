// Combat/Move.wren
// A single combat action: a name, a charge cost in time units (see Combat/Timeline.wren), flat
// damage, a base interrupt delay (Phase 3 - Combat/Disruption.wren), a stamina cost, a stamina
// restore, and an HP heal-self amount (Phase 4-5). A move fires the instant its bar fills; later
// phases add element and status effects to this same object.
class Move {
    // baseDelay: 2-6 time units (0 for non-offensive moves - they never interrupt).
    // staminaCost / staminaRestore / healAmount: paid / gained / healed when the move resolves.
    // curveId / particleId: keys into Combat/MoveCurves.wren and Combat/MoveParticles.wren for the
    // strike-animation shape and impact effect - null for non-offensive moves, which never pick an
    // enemy target and so never animate a strike (see CombatState.resolveMove).
    // No parry prompt - non-offensive moves are never a parry target (see `offensive`/`parryable`).
    construct new(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount, curveId,
                 particleId) {
        init_(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount, curveId,
             particleId, null)
    }

    // parryPromptTime: seconds the target has to answer States/CombatState.wren's "parryPrompt"
    // ("attempt a parry?") before it auto-answers "no" - only offensive moves need this; everything
    // else uses the 9-arg constructor above.
    construct new(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount, curveId,
                 particleId, parryPromptTime) {
        init_(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount, curveId,
             particleId, parryPromptTime)
    }

    init_(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount, curveId,
         particleId, parryPromptTime) {
        _name = name
        _timeCost = timeCost
        _damage = damage
        _baseDelay = baseDelay
        _staminaCost = staminaCost
        _staminaRestore = staminaRestore
        _healAmount = healAmount
        _curveId = curveId
        _particleId = particleId
        _parryPromptTime = parryPromptTime
    }

    name { _name }
    timeCost { _timeCost }
    damage { _damage }
    baseDelay { _baseDelay }
    staminaCost { _staminaCost }
    staminaRestore { _staminaRestore }
    healAmount { _healAmount }
    curveId { _curveId }
    particleId { _particleId }
    parryPromptTime { _parryPromptTime }

    // True for anything that picks an enemy target (vs. a self-buff / heal).
    offensive { _damage > 0 }

    // True for a move that offers the target a parry prompt (States/CombatState.wren) - always false
    // for non-offensive moves, which are built with the 9-arg constructor above.
    parryable { _parryPromptTime != null }

    // "Strike  4u  -26sp" / "Mend  4u  +42hp" / "Dodge  2u  +22sp" - for the move-menu buttons.
    menuLabel {
        if (_healAmount > 0) {
            return "%(_name)  %(_timeCost)u  +%(_healAmount)hp"
        }
        if (_staminaRestore > 0) {
            return "%(_name)  %(_timeCost)u  +%(_staminaRestore)sp"
        }
        return "%(_name)  %(_timeCost)u  -%(_staminaCost)sp"
    }
}
