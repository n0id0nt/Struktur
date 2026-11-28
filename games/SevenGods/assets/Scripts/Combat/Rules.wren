// Combat/Rules.wren
// Everything gear does to a fight, in one place: the damage / healing / stagger formulas of the battle system doc,
// the seven elements (weapon effect on a damaging hit, armour effect on a trigger, matched sets, the named
// weapon x armour synergies and paradoxes, Echo for every other pairing, party reactions between statuses,
// resistances) and the sixteen custom effects (weapon and armour versions, ranks I-III, Resonance capstones), with
// the stacking caps: bonuses in a category add together, are capped, then apply once.
//
// Hard rule: nothing here ever changes a move's impact, completion or move time. It changes damage, defence, the
// size of a set-back or a skip, stamina, statuses and position.
//
// Combat/BattleEngine.wren owns one Rules and calls it at fixed points: moveCost / onMoveStart when a move starts,
// damage / staggerOf / onHit when a hit lands, onBlock / onDodge / onDamaged for the defender's side, tick every
// step for statuses. Rules calls back into the engine for the board (who stands where, pushing, set-backs).
import "Combat/Gear" for Elements

var CAP_DAMAGE = 1.0          // damage bonus %
var CAP_DEFENCE = 0.6         // defence bonus %
var CAP_SETBACK = 0.75        // set-back reduction % (everyone stays interruptible)
var CAP_LIFESTEAL = 0.15      // healing from damage dealt, share of the hit
var CAP_REDUCTION = 0.7       // damage reduction vs specific hits

var BURN_TIME = 3
var CHILL_TIME = 4
var BLEED_TIME = 3
var STAGGER_TO_STAMINA = 20   // Absorb: stamina gained per second of stagger soaked

class Rules {
    construct new(engine) {
        _e = engine
    }

    // values: [-, rank I, rank II, rank III]; rank 0 -> 0.
    static r(values, rank) { rank <= 0 ? 0 : values[rank] }

    // The element / strength a combatant attacks with: its weapon's (or a monster's own).
    attackElement(fighter) { fighter.has("element") ? fighter.trait("element") : fighter.loadout.weaponElement }
    bodyElement(fighter) { fighter.has("bodyElement") ? fighter.trait("bodyElement") : fighter.loadout.armourElement }
    late(fighter) { fighter.loadout.weaponLate || fighter.has("late") }

    // --- stamina -------------------------------------------------------------------------

    // What `move` costs `f` right now: base + weapon tax + armour tax (+ the armour's movement surcharge), then the
    // effects that bend it. consume: true when the move is really starting (one-shot discounts are used up).
    moveCost(fighter, move, consume) {
        if (move.isHold || fighter.has("noStamina")) return 0
        var load = fighter.loadout
        var tax = load.tax
        var weaponTax = tax[0]
        var armourTax = tax[1]
        var mobile = move.isMovement || move.isDodge
        // Wild armour: the next Move (late: or Dodge) after a dodge pays no armour tax.
        if (fighter.counter("wildFree") > 0 && (move.isMovement || (load.armourLate && move.isDodge))) {
            armourTax = 0
            if (consume) fighter.setCounter("wildFree", 0)
        }
        var cost = move.cost + weaponTax + armourTax + (fighter.has("tax") ? fighter.trait("tax") : 0)
        if (mobile) {
            cost = cost + load.surcharge - Rules.r([0, 1, 2, 3], load.armourRank("Skirmisher"))
            // Skirmisher capstone: the first movement move of each chain costs nothing.
            if (load.capstone("Skirmisher") && move.isMovement && fighter.counter("skirmishFree") == 0) {
                cost = 0
                if (consume) fighter.setCounter("skirmishFree", 1)
            }
        }
        // Gold armour hit this combatant: its next move costs more.
        if (fighter.counter("goldTax") > 0) {
            cost = cost * (1 + fighter.counter("goldTax"))
            if (consume) fighter.setCounter("goldTax", 0)
        }
        cost = cost.round
        return cost < 0 ? 0 : cost
    }

    // Stamina per second in Hold: Chill slows it, Last Stand's capstone doubles it below 20% HP.
    holdRecovery(fighter) {
        var rate = fighter.stats.recovery
        if (fighter.chill > 0) rate = rate * (1 - fighter.chillStrength)
        if (fighter.loadout.capstone("Last Stand") && fighter.stats.fraction < 0.2) rate = rate * 2
        return rate
    }

    onChainStart(fighter) {
        fighter.setCounter("skirmishFree", 0)
    }

    onMoveStart(fighter, move) {
        var load = fighter.loadout
        // Riposte (weapon): an attack that starts within 0.7 s of your block peak.
        var blockedAt = fighter.counter("blockPeakAt")
        fighter.setCounter("riposte", (move.isAttack && blockedAt > 0 && _e.clock - blockedAt <= 0.7) ? 1 : 0)
        // Wildfire: when you move, Burn spreads from any burning enemy beside you to its neighbours.
        if (move.isMovement && load.pairing == "Wildfire") {
            for (enemy in _e.adjacentEnemies(fighter)) {
                if (enemy.burn > 0) {
                    for (other in _e.neighbours(enemy)) {
                        if (other.side == enemy.side && other.burn == 0) addBurn(other, fighter, 1, false)
                    }
                }
            }
        }
    }

    // --- damage ----------------------------------------------------------------------------

    // The weapon numbers `move` uses for `f`: [power, stagger multiplier, element or null, item or null].
    weaponFor(fighter, move) {
        if (fighter.has("natural")) {
            var natural = fighter.trait("natural")
            return [natural[0], natural[1], attackElement(fighter), null]
        }
        var item = fighter.loadout.itemFor(move)[1]
        if (item == null) return [0, 1, null, null]
        return [item.power, item.type.staggerMul, item.element, item]
    }

    scalingOf(fighter, move) {
        var mana = move.scaling == "mana" || (fighter.has("scaling") && fighter.trait("scaling") == "mana")
        return mana ? fighter.stats.mana + fighter.loadout.manaBonus : fighter.stats.attack
    }

    // The summed, capped damage bonus % of every custom effect (and Magnetize) for this hit.
    damageBonus(attacker, target, move, isAim) {
        var load = attacker.loadout
        var bonus = 0
        if (attacker.counter("riposte") > 0) bonus = bonus + Rules.r([0, 0.2, 0.3, 0.4], load.weaponRank("Riposte"))
        var momentum = load.weaponRank("Momentum")
        if (momentum > 0) bonus = bonus + attacker.counter("momentumHits") * Rules.r([0, 0.06, 0.08, 0.1], momentum)
        if (load.weaponRank("Opportunist") > 0 && (target.inRecovery || target.counter("recoverUntil") > _e.clock)) {
            bonus = bonus + Rules.r([0, 0.25, 0.4, 0.6], load.weaponRank("Opportunist"))
        }
        if (load.weaponRank("Vanguard") > 0 && (_e.inFrontRow(attacker) || (load.capstone("Vanguard") && _e.inEnemyHalf(attacker)))) {
            bonus = bonus + Rules.r([0, 0.15, 0.2, 0.3], load.weaponRank("Vanguard"))
        }
        if (move.isMovement) bonus = bonus + Rules.r([0, 0.15, 0.25, 0.35], load.weaponRank("Skirmisher"))
        if (target.guardNow > 0) bonus = bonus + Rules.r([0, 0.5, 0.75, 1], load.weaponRank("Breaker"))
        var lastStand = load.weaponRank("Last Stand")
        if (lastStand > 0) bonus = bonus + ((1 - attacker.stats.fraction) * 10).floor * Rules.r([0, 0.01, 0.015, 0.02], lastStand)
        if (attacker.counter("reprisal") > 0) bonus = bonus + Rules.r([0, 0.15, 0.25, 0.35], load.weaponRank("Reprisal"))
        if (attacker.startedAfterHold && move.scaling == "mana") bonus = bonus + Rules.r([0, 0.1, 0.15, 0.2], load.weaponRank("Focus"))
        if (load.pairing == "Magnetize") bonus = bonus + attacker.counter("charges") * 0.1
        return bonus > CAP_DAMAGE ? CAP_DAMAGE : bonus
    }

    // The summed, capped defence bonus % - applies to passive defence and to the block alike.
    defenceBonus(target, guard) {
        var load = target.loadout
        var bonus = 0
        var momentum = load.armourRank("Momentum")
        if (momentum > 0) bonus = bonus + target.counter("momentumBlocks") * Rules.r([0, 0.06, 0.08, 0.1], momentum)
        if (load.armourRank("Vanguard") > 0 && (_e.inFrontRow(target) || (load.capstone("Vanguard") && _e.inEnemyHalf(target)))) {
            bonus = bonus + Rules.r([0, 0.15, 0.2, 0.3], load.armourRank("Vanguard"))
        }
        var lastStand = load.armourRank("Last Stand")
        if (lastStand > 0) bonus = bonus + ((1 - target.stats.fraction) * 10).floor * Rules.r([0, 0.01, 0.015, 0.02], lastStand)
        // Timed buffs: Guardian (from the ally ahead) and Bastion (Radiance overheal).
        if (target.counter("defBuffUntil") > _e.clock) bonus = bonus + target.counter("defBuff")
        // Iron armour: stronger at the peak of a Defend move.
        if (bodyElement(target) == "Iron" && guard >= 0.9) bonus = bonus + (target.loadout.armourLate ? 0.25 : 0.15)
        return bonus > CAP_DEFENCE ? CAP_DEFENCE : bonus
    }

    // The passive defence of `t` (its stat - or a boss's Granite Skin - plus armour), before bonuses.
    passiveDefence(target) {
        var base = target.stats.defence
        if (target.has("granite")) {
            var granite = target.trait("granite")
            base = target.exposedNow ? granite[1] : granite[0]
        }
        return base + target.loadout.armourDefence
    }

    // The value of a Defend move at guard level g (0..1) for its user: (base + the shield's guard) x g.
    blockValue(defender, guard) {
        var move = defender.move
        if (move == null || !move.isDefend || guard <= 0) return 0
        var value = move.defence
        var shield = defender.loadout.shield
        if (move.addsShield && shield != null) value = value + shield.guard
        if (defender.loadout.pairing == "Grounded") value = value * 0.85
        return value * guard
    }

    // Final damage of one hit. share: the stencil share of the cell. isAim: the aim cell (vs a side / depth cell).
    // block: the active block value protecting the target (own or an ally's cover), already scaled by its curve.
    // Returns [final, raw].
    damage(attacker, target, move, share, isAim, depthCell, block, guard) {
        var weapon = weaponFor(attacker, move)
        var power = weapon[0]
        var item = weapon[3]
        var raw = (move.damage + power) * (1 + scalingOf(attacker, move) / 100) * share

        // Type quirks from the weapon table.
        if (item != null) {
            var id = item.type.id
            if (id == "claymore" && !isAim && !depthCell) raw = raw * 1.15
            if (id == "spear" && depthCell) raw = raw * 1.25
            if (id == "pike" && move.aim == "reach" && move.reach == 1) raw = raw * 0.5
        }
        // Backstab: the target faces away from the attacker.
        if (move.backstab > 1 && _e.isBehind(attacker, target)) {
            var mult = move.backstab
            if (item != null && item.type.id == "stiletto" && mult < 2.5) mult = 2.5
            raw = raw * mult
        }
        if (!isAim) raw = raw * (1 + Rules.r([0, 0.25, 0.5, 0.75], attacker.loadout.weaponRank("Precision")))
        raw = raw * (1 + damageBonus(attacker, target, move, isAim))
        // Shatterpoint: Quake hits on a Frozen target deal +50% and consume Frozen.
        if (attacker.loadout.pairing == "Shatterpoint" && target.frozen > 0) {
            raw = raw * 1.5
            target.frozen = 0
            _e.note(target, "Shatterpoint")
        }
        // A boss part caught in an Exposed window.
        if (target.exposedNow) raw = raw * 1.5
        raw = raw * Elements.resistance(bodyElement(target), weapon[2])

        var bonus = 1 + defenceBonus(target, guard)
        var final = raw * 100 / (100 + passiveDefence(target) * bonus) - block * bonus
        // Granite Skin: outside an Exposed window a hit only scratches.
        if (target.has("granite") && !target.exposedNow) final = final * target.trait("granite")[2]
        // Gold matched set: stamina stolen above the maximum, banked as flat bonus damage (outside the cap).
        if (attacker.counter("goldBonus") > 0) {
            final = final + attacker.counter("goldBonus")
            attacker.setCounter("goldBonus", 0)
        }
        // Reductions against specific hits.
        var reduction = 0
        if (!isAim) reduction = reduction + Rules.r([0, 0.3, 0.5, 0.7], target.loadout.armourRank("Precision"))
        if (move.aim == "first") reduction = reduction + Rules.r([0, 0.1, 0.2, 0.3], target.loadout.armourRank("Piercing"))
        if (reduction > CAP_REDUCTION) reduction = CAP_REDUCTION
        final = final * (1 - reduction)
        // Guardian capstone: the ally directly behind a Guardian takes 15% less at all times.
        var ahead = _e.occupant(target.lane, target.row + _e.forward(target.side))
        if (ahead != null && ahead.side == target.side && ahead.loadout.capstone("Guardian")) final = final * 0.85
        if (final < 0) final = 0
        return [final.round, raw]
    }

    healAmount(attacker, move, share) {
        var weapon = weaponFor(attacker, move)
        var amount = (move.healAmount + weapon[0]) * (1 + scalingOf(attacker, move) / 100) * share
        if (attacker.startedAfterHold) amount = amount * (1 + Rules.r([0, 0.1, 0.15, 0.2], attacker.loadout.weaponRank("Focus")))
        if (attacker.loadout.pairing == "Tithe") amount = amount * 0.8
        return amount.round
    }

    // --- stagger ---------------------------------------------------------------------------

    // The raw stagger (seconds) this hit carries, before the target's own reductions and diminishing returns.
    staggerOf(attacker, target, move, share) {
        var weapon = weaponFor(attacker, move)
        var load = attacker.loadout
        var stagger = move.stagger * share * weapon[1]
        var bonus = Rules.r([0, 0.25, 0.35, 0.5], load.weaponRank("Heavy"))
        if (weapon[2] == "Iron") {
            bonus = bonus + (late(attacker) ? 0.3 : 0.2)                    // Quake
            if (target.frozen > 0) {
                bonus = bonus + 1                                    // Avalanche
                target.frozen = 0
                _e.note(target, "Avalanche")
            }
        }
        // Storm armour's Charges: spent on the next attack.
        var charges = attacker.counter("charges")
        if (charges > 0) {
            bonus = bonus + charges * (load.armourLate ? 0.2 : 0.15)
            if (load.pairing == "Cold Snap") addChill(target, attacker, charges, late(attacker))
            attacker.setCounter("charges", 0)
        }
        if (attacker.counter("ironEcho") > 0) {
            bonus = bonus + 0.1
            attacker.setCounter("ironEcho", 0)
        }
        stagger = stagger * (1 + bonus)
        if (attacker.counter("riposte") > 0 && load.weaponRank("Riposte") >= 3) stagger = stagger + 0.1
        if (load.weaponRank("Last Stand") >= 3 && attacker.stats.fraction < 0.3) stagger = stagger * 1.1
        return stagger
    }

    // The summed, capped share of a set-back `t` ignores right now.
    setbackReduction(target) {
        var load = target.loadout
        var cut = load.staggerRes
        cut = cut + Rules.r([0, 0.25, 0.35, 0.5], load.armourRank("Heavy"))
        if (target.inRecovery) cut = cut + Rules.r([0, 0.25, 0.4, 0.6], load.armourRank("Opportunist"))
        var move = target.move
        if (move != null && (move.isHeal || move.scaling == "mana")) cut = cut + Rules.r([0, 0.1, 0.2, 0.3], load.armourRank("Focus"))
        if (load.matched("Iron") && move != null && move.isDefend) cut = cut + (load.armourLate ? 0.5 : 0.3)
        if (target.has("staggerRes")) cut = cut + target.trait("staggerRes")
        return cut > CAP_SETBACK ? CAP_SETBACK : cut
    }

    // How a block's skip grows (Sentinel armour).
    skipBonus(target) { Rules.r([0, 0.1, 0.15, 0.2], target.loadout.armourRank("Sentinel")) }

    // What a guard-range cell's share becomes (Sentinel weapon / shield, Vanguard III armour on the sides).
    guardShare(defender, share, side) {
        var total = share + Rules.r([0, 0.15, 0.25, 0.35], defender.loadout.weaponRank("Sentinel"))
        if (side != 0 && defender.loadout.armourRank("Vanguard") >= 3) total = total + 0.25
        return total > 1 ? 1 : total
    }

    // --- statuses --------------------------------------------------------------------------

    statusTime_(target, element, seconds) {
        var res = Elements.resistance(bodyElement(target), element)
        if (res < 1) return seconds / 2
        if (res > 1) return seconds * 1.5
        return seconds
    }

    // Adds Burn stacks from `source` (null = nobody in particular - a flask). Burn meeting Chill reacts instead:
    // Thermal Shock, when the two came from different combatants (or the source carries Temper / Quench).
    addBurn(target, source, stacks, isLate) {
        if (!target.active) return
        if (target.chill > 0) {
            if (canShock_(source, target.chillSource)) thermalShock(target, source, isLate)
            return
        }
        var max = (source != null && source.loadout.matched("Flame") && source.loadout.weaponLate) ? 4 : 3
        var before = target.burn
        target.burn = (target.burn + stacks).min(max.max(before))
        target.burnTime = statusTime_(target, "Flame", BURN_TIME)
        target.burnSource = source
        target.burnRate = isLate ? 0.02 : 0.015
        if (target.burn != before) _e.status(target, "Burn")
    }

    addChill(target, source, stacks, isLate) {
        if (!target.active) return
        if (target.burn > 0) {
            if (canShock_(source, target.burnSource)) thermalShock(target, source, isLate)
            return
        }
        var before = target.chill
        target.chill = (target.chill + stacks).min(3)
        target.chillTime = statusTime_(target, "Frost", CHILL_TIME)
        target.chillSource = source
        target.chillStrength = isLate ? 0.5 : 0.35
        if (target.chill != before) _e.status(target, "Chill")
        // Frost matched set: three stacks become Frozen - the next set-back taken is multiplied, then it ends.
        if (target.chill >= 3 && source != null && source.loadout.matched("Frost") && target.frozen == 0) {
            target.frozen = source.loadout.weaponLate ? 2 : 1.5
            _e.status(target, "Frozen")
        }
    }

    canShock_(source, other) {
        if (source == null || other == null || source != other) return true
        var pair = source.loadout.pairing
        return pair == "Temper" || pair == "Quench"
    }

    // Burn + Chill on one target: a burst of 5% max HP (late 7%) and a 1.0 s set-back; both statuses are consumed.
    thermalShock(target, source, isLate) {
        target.burn = 0
        target.chill = 0
        target.frozen = 0
        _e.note(target, "Thermal Shock")
        _e.dealDirect(source, target, (target.stats.maxHp * (isLate ? 0.07 : 0.05)).round, "Thermal Shock")
        _e.setback(target, 1.0, source)
    }

    // The damage a target's Burn still has left to deal.
    burnLeft_(target) { target.stats.maxHp * target.burnRate * target.burn * target.burnTime }

    // Every step: Burn / Bleed damage (pro-rated, so it never jumps in chunks), status and shield timers.
    tick(dt) {
        for (fighter in _e.fighters) {
            if (!fighter.active) continue
            if (fighter.burn > 0) {
                var tick = fighter.stats.maxHp * fighter.burnRate * fighter.burn * dt
                _e.dealDirect(fighter.burnSource, fighter, tick, null)
                var source = fighter.burnSource
                if (source != null && source.active) {
                    // Flame matched set: Burn also drains stamina. Beacon: Burn on enemies you burned heals you.
                    if (source.loadout.matched("Flame")) fighter.stats.spendStamina((source.loadout.weaponLate ? 2 : 1) * dt)
                    if (source.loadout.pairing == "Beacon") source.stats.heal(tick * 0.2)
                }
                fighter.burnTime = fighter.burnTime - dt
                if (fighter.burnTime <= 0) fighter.burn = 0
            }
            if (fighter.chill > 0) {
                fighter.chillTime = fighter.chillTime - dt
                if (fighter.chillTime <= 0) fighter.chill = 0
            }
            if (fighter.bleed > 0) {
                _e.dealDirect(null, fighter, fighter.stats.maxHp * 0.01 * fighter.bleed * dt, null)
                fighter.bleedTime = fighter.bleedTime - dt
                if (fighter.bleedTime <= 0) fighter.bleed = 0
            }
            if (fighter.shield > 0) {
                fighter.shieldTime = fighter.shieldTime - dt
                if (fighter.shieldTime <= 0) fighter.shield = 0
            }
        }
    }

    // --- the attacker's side of a hit ------------------------------------------------------

    // A hit by `a` with `move` got through to `t` for `dealt` (> 0). cell: [lane, row] it landed in. stagger: what
    // it carried. Applies the weapon's element (after any party reaction it sets off) and the weapon effects.
    onHit(attacker, target, move, dealt, cell, isAim, stagger) {
        var load = attacker.loadout
        var weapon = weaponFor(attacker, move)
        var element = weapon[2]
        var isLate = late(attacker)

        // Streaks and one-shots.
        var momentum = load.weaponRank("Momentum")
        if (momentum > 0) attacker.addCounter("momentumHits", 1, [0, 3, 4, 5][momentum])
        attacker.setCounter("reprisal", 0)
        if (attacker.has("bleed")) {
            target.bleed = (target.bleed + 1).min(3)
            target.bleedTime = BLEED_TIME
            _e.status(target, "Bleed")
        }

        // Healing from damage dealt (Radiance, Bloodthirst, Tithe, Indulgence): summed, capped at 15% of the hit.
        var steal = Rules.r([0, 0.03, 0.05, 0.08], load.weaponRank("Bloodthirst"))
        var flatHeal = 0
        if (element == "Light") {
            var radiance = isLate ? 0.1 : 0.06
            if (load.pairing == "Second Wind" && attacker.counter("secondWind") > 0) {
                radiance = radiance * 2
                attacker.setCounter("secondWind", 0)
            }
            steal = steal + radiance
        }
        if (element == "Gold" && (load.pairing == "Indulgence" || load.pairing == "Tithe")) flatHeal = attacker.stats.maxHp * 0.02
        if (steal > CAP_LIFESTEAL) steal = CAP_LIFESTEAL
        var healing = dealt * steal + flatHeal
        if (healing > 0) {
            var over = attacker.stats.heal(healing)
            _e.healed(attacker, healing - over)
            if (over > 0) {
                // Light matched set / Bloodthirst capstone: overheal becomes a 5 s shield. Bastion: +10% defence.
                if (load.matched("Light") || load.capstone("Bloodthirst")) {
                    var cap = attacker.stats.maxHp * ((load.matched("Light") && load.weaponLate) ? 0.2 : 0.1)
                    attacker.shield = (attacker.shield + over).min(cap)
                    attacker.shieldTime = 5
                }
                if (load.pairing == "Bastion") {
                    attacker.setCounter("defBuff", 0.1)
                    attacker.setCounter("defBuffUntil", _e.clock + 3)
                }
            }
        }

        // Tempo (weapon): a landed hit refunds part of the move's cost - once per move, never more than it cost.
        var tempo = load.weaponRank("Tempo")
        if (tempo > 0 && !attacker.hitLanded) {
            var refund = attacker.cost * Rules.r([0, 0.15, 0.25, 0.35], tempo)
            if (load.capstone("Tempo") && attacker.chainIndex >= attacker.chain.count - 1) refund = attacker.cost
            attacker.stats.gainStamina(refund)
        }
        attacker.hitLanded = true

        // Guardian (weapon): hitting an enemy in your lane steels the ally behind you.
        var guardian = load.weaponRank("Guardian")
        if (guardian > 0 && target.lane == attacker.lane) {
            var behind = _e.occupant(attacker.lane, attacker.row - _e.forward(attacker.side))
            if (behind != null && behind.side == attacker.side) {
                behind.setCounter("defBuff", Rules.r([0, 0.1, 0.15, 0.2], guardian))
                behind.setCounter("defBuffUntil", _e.clock + 3)
            }
        }

        if (element != null) elementHit_(attacker, target, element, isLate, dealt, cell, stagger, 1)
    }

    // The on-hit effect of `element`, at `scale` (1 = a real hit; Echo passes its own reduced amounts instead).
    elementHit_(attacker, target, element, isLate, dealt, cell, stagger, scale) {
        var load = attacker.loadout
        var other = target.burnSource != attacker || target.chillSource != attacker
        if (element == "Flame") {
            addBurn(target, attacker, 1, isLate)
        } else if (element == "Frost") {
            addChill(target, attacker, 1, isLate)
        } else if (element == "Storm") {
            arc_(attacker, target, dealt * (isLate ? 0.3 : 0.2), stagger, cell, isLate)
        } else if (element == "Iron") {
            // Quake's stagger is already in staggerOf. Eruption: the Burn left on the target goes off at once,
            // half of it into the cells beside. Forge: Quake on a Burning target refreshes Burn instead.
            if (target.burn > 0) {
                if (load.pairing == "Forge") {
                    target.burnTime = statusTime_(target, "Flame", BURN_TIME)
                } else if (target.burnSource != attacker) {
                    var left = burnLeft_(target)
                    target.burn = 0
                    _e.note(target, "Eruption")
                    _e.dealDirect(attacker, target, left.round, "Eruption")
                    for (neighbour in _e.besideEnemies(target)) {
                        _e.dealDirect(attacker, neighbour, (left * 0.5).round, "Eruption")
                    }
                }
            }
        } else if (element == "Wild") {
            gust_(attacker, target, isLate, dealt, stagger)
        } else if (element == "Light") {
            // Radiance's healing is in onHit. Judgement: the Burn left goes off at once and heals the attacker.
            if (target.burn > 0 && target.burnSource != attacker) {
                var left = burnLeft_(target).round
                target.burn = 0
                _e.note(target, "Judgement")
                _e.dealDirect(attacker, target, left, "Judgement")
                _e.healed(attacker, left - attacker.stats.heal(left))
            }
            if (load.pairing == "Tithe") drain_(attacker, target, 2)
        } else if (element == "Gold") {
            var amount = isLate ? 5 : 3
            // Frostbite (another's Chill) / Frozen Assets (the pairing): Drain steals double; Chill refreshes.
            if (target.chill > 0 && (target.chillSource != attacker || load.pairing == "Frozen Assets")) {
                amount = amount * 2
                if (target.chillSource != attacker) {
                    target.chillTime = statusTime_(target, "Frost", CHILL_TIME)
                    _e.note(target, "Frostbite")
                }
            }
            drain_(attacker, target, amount)
        }
    }

    // Steals stamina from `t` for `a`; with the Gold matched set, what doesn't fit becomes bonus damage.
    drain_(attacker, target, amount) {
        if (target.has("noStamina")) return
        var taken = amount.min(target.stats.stamina)
        target.stats.spendStamina(taken)
        var over = attacker.stats.gainStamina(taken)
        if (over > 0 && attacker.loadout.matched("Gold")) {
            attacker.addCounter("goldBonus", over, attacker.loadout.weaponLate ? 25 : 10)
        }
    }

    // Storm's Arc: `amount` damage jumps to the enemies in the cells beside the hit cell.
    arc_(attacker, target, amount, stagger, cell, isLate) {
        var load = attacker.loadout
        var ignite = target.burn > 0 && target.burnSource != attacker
        for (neighbour in _e.enemiesBesideCell(attacker, cell[0], cell[1])) {
            var dmg = amount
            // Shatter: an Arc onto a Chilled target (another's Chill) deals double and consumes it.
            if (neighbour.chill > 0 && neighbour.chillSource != attacker) {
                dmg = dmg * 2
                neighbour.chill = 0
                neighbour.frozen = 0
                _e.note(neighbour, "Shatter")
            }
            _e.dealDirect(attacker, neighbour, dmg.round, "Arc")
            // Ignition: an Arc off a Burning target sets everyone it reaches alight.
            if (ignite) {
                addBurn(neighbour, attacker, 1, isLate)
                _e.note(neighbour, "Ignition")
            }
            // Storm matched set: Arcs carry stagger too. Grounded: they also carry Quake.
            if (load.matched("Storm") || load.pairing == "Grounded") {
                var carried = load.matched("Storm") ? stagger * (load.weaponLate ? 1 : 0.5) : 0
                if (load.pairing == "Grounded") carried = carried + stagger * 0.2
                if (carried > 0) _e.setback(neighbour, carried, attacker)
            }
            if (load.pairing == "Overload") {
                neighbour.stats.spendStamina(2)
            }
        }
    }

    // Wild's Gust: push the target one cell directly away from the attacker if that cell is free.
    gust_(attacker, target, isLate, dealt, stagger) {
        var load = attacker.loadout
        var burning = target.burn > 0 && target.burnSource != attacker
        var chilled = target.chill > 0 && target.chillSource != attacker
        var dir = _e.awayFrom(attacker, target)
        var toLane = target.lane + dir[0]
        var toRow = target.row + dir[1]
        // Firestorm: a Gust onto a Burning target spreads Burn to the cell it is pushed toward.
        if (burning) {
            var inWay = _e.occupant(toLane, toRow)
            if (inWay != null && inWay.side == target.side) {
                addBurn(inWay, attacker, 1, isLate)
                _e.note(inWay, "Firestorm")
            }
        }
        if (_e.push(target, dir[0], dir[1])) {
            if (load.pairing == "Scatter") drain_(attacker, target, 3)
            // Whiteout: a Gust onto a Chilled target chills everyone beside where it lands.
            if (chilled) {
                for (neighbour in _e.neighbours(target)) {
                    if (neighbour.side == target.side) addChill(neighbour, attacker, 1, isLate)
                }
                _e.note(target, "Whiteout")
            }
        } else {
            // A blocked push: the Wild matched set turns it into extra stagger; late Gust adds a 0.1 s set-back.
            if (load.matched("Wild")) {
                _e.setback(target, stagger * (load.weaponLate ? 0.5 : 0.3), attacker)
            } else if (isLate) {
                _e.setback(target, 0.1, attacker)
            }
            // Squall: a Gust that collides lets off an Arc beside the collision.
            if (load.pairing == "Squall") arc_(attacker, target, dealt * 0.2, 0, [toLane, toRow], isLate)
        }
    }

    // --- the defender's side ---------------------------------------------------------------

    // `t` took `dealt` damage from `a`'s hit.
    onDamaged(target, attacker, dealt) {
        var load = target.loadout
        var armour = bodyElement(target)
        // Momentum resets when you're hit (its capstone lets the streak survive the first one).
        if (target.counter("momentumHits") > 0) {
            if (load.capstone("Momentum") && target.counter("momentumSaved") == 0) {
                target.setCounter("momentumSaved", 1)
            } else {
                target.setCounter("momentumHits", 0)
                target.setCounter("momentumSaved", 0)
            }
        }
        target.setCounter("momentumBlocks", 0)
        if (load.weaponRank("Reprisal") > 0) target.setCounter("reprisal", 1)
        // Reprisal (armour): the attacker takes a share of the damage back.
        var reprisal = load.armourRank("Reprisal")
        if (reprisal > 0) {
            _e.dealDirect(target, attacker, (dealt * Rules.r([0, 0.05, 0.08, 0.12], reprisal)).round, "Reprisal")
            if (load.capstone("Reprisal")) _e.setback(attacker, 0.2, target)
        }
        // Guardian (armour): allies whose guard range covers the one who was hit gain stamina.
        for (ally in _e.alliesOf(target)) {
            var guardian = ally.loadout.armourRank("Guardian")
            if (guardian > 0 && _e.covers(ally, target)) ally.stats.gainStamina(Rules.r([0, 5, 8, 12], guardian))
        }

        var pair = load.pairing
        if (armour == "Frost" && pair != "Temper") {
            addChill(attacker, target, 1, load.armourLate)
            if (load.armourLate) attacker.setCounter("halveSetback", 1)
            echo_(target, attacker, dealt)
        } else if (armour == "Gold") {
            attacker.setCounter("goldTax", load.armourLate ? 0.2 : 0.1)
            echo_(target, attacker, dealt)
        }
    }

    // A hit landed on a Defend window. t: the one whose block absorbed it (the guarder, for a covered ally).
    // g: the guard level (0..1), near: seconds from the window's peak, s: the stagger the hit carried.
    onBlock(target, attacker, guard, near, stagger, dealt) {
        var load = target.loadout
        var armour = bodyElement(target)
        var pair = load.pairing
        var atPeak = guard >= 0.9
        if (atPeak) target.setCounter("blockPeakAt", _e.clock)
        var momentum = load.armourRank("Momentum")
        if (momentum > 0) target.addCounter("momentumBlocks", 1, [0, 3, 4, 5][momentum])
        // Breaker (armour): every hit you block drains the attacker.
        var breaker = load.armourRank("Breaker")
        if (breaker > 0 && !attacker.has("noStamina")) attacker.stats.spendStamina(Rules.r([0, 3, 5, 8], breaker))
        // Riposte (armour): a block at the peak returns part of the hit's stagger as a set-back.
        var riposte = load.armourRank("Riposte")
        if (riposte > 0 && atPeak) _e.setback(attacker, stagger * Rules.r([0, 0.3, 0.5, 0.7], riposte), target)
        // Permafrost: Chilled attackers that hit your guard take the stagger they would have dealt.
        if (pair == "Permafrost" && attacker.chill > 0) _e.setback(attacker, stagger, target)
        // Toll: a block at the peak also Drains the attacker, as if you'd hit them.
        if (pair == "Toll" && atPeak) drain_(target, attacker, load.weaponLate ? 5 : 3)

        if (armour == "Flame" && guard >= 0.75 && pair != "Quench") {
            addBurn(attacker, target, load.armourLate ? 2 : 1, load.armourLate)
            echo_(target, attacker, dealt)
        } else if (armour == "Light" && near <= 0.1) {
            var heal = target.stats.maxHp * (load.armourLate ? 0.05 : 0.03)
            if (pair == "Indulgence") heal = heal / 2
            _e.healed(target, heal - target.stats.heal(heal))
            echo_(target, attacker, dealt)
        } else if (armour == "Iron" && atPeak) {
            echo_(target, attacker, dealt)
        }
    }

    // `t` was inside a dodge window when `a`'s hit arrived.
    onDodge(target, attacker) {
        var load = target.loadout
        var armour = bodyElement(target)
        var pair = load.pairing
        var bloodthirst = load.armourRank("Bloodthirst")
        if (bloodthirst > 0) {
            var heal = target.stats.maxHp * Rules.r([0, 0.01, 0.02, 0.03], bloodthirst)
            _e.healed(target, heal - target.stats.heal(heal))
        }
        var tempo = load.armourRank("Tempo")
        if (tempo > 0) target.stats.gainStamina(target.cost * Rules.r([0, 0.3, 0.5, 0.7], tempo))
        if (pair == "Second Wind") target.setCounter("secondWind", 1)
        if (armour == "Storm") {
            var max = pair == "Magnetize" ? 2 : (load.armourLate ? 3 : 2)
            target.addCounter("charges", 1, max)
            _e.status(target, "Charge")
            echo_(target, attacker, 0)
        } else if (armour == "Wild") {
            target.setCounter("wildFree", 1)
            echo_(target, attacker, 0)
        }
        // Stormstep: a dodge that moves you fires a 0-damage Arc that sets back the cells beside where you land.
        if (pair == "Stormstep" && target.move != null && target.move.isMovement) {
            for (neighbour in _e.enemiesBesideCell(target, target.lane, target.row)) {
                _e.setback(neighbour, 0.2, target)
            }
        }
    }

    // Echo - the default for unnamed weapon x armour pairs: when the armour's effect triggers, the weapon's element
    // also fires once on that enemy at reduced strength.
    echo_(target, attacker, dealt) {
        var load = target.loadout
        if (load.pairing != "Echo") return
        var element = load.weaponElement
        if (element == "Flame") {
            addBurn(attacker, target, 1, false)
        } else if (element == "Frost") {
            addChill(attacker, target, 1, false)
        } else if (element == "Storm") {
            if (dealt > 0) arc_(target, attacker, dealt * 0.1, 0, [attacker.lane, attacker.row], false)
        } else if (element == "Iron") {
            target.setCounter("ironEcho", 1)
        } else if (element == "Wild") {
            var dir = _e.awayFrom(target, attacker)
            _e.push(attacker, dir[0], dir[1])
        } else if (element == "Light") {
            var heal = target.stats.maxHp * 0.02
            _e.healed(target, heal - target.stats.heal(heal))
        } else if (element == "Gold") {
            drain_(target, attacker, 2)
        }
        _e.note(target, "Echo")
    }
}
