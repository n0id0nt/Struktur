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
    attackElement(f) { f.has("element") ? f.trait("element") : f.loadout.weaponElement }
    bodyElement(f) { f.has("bodyElement") ? f.trait("bodyElement") : f.loadout.armourElement }
    late(f) { f.loadout.weaponLate || f.has("late") }

    // --- stamina -------------------------------------------------------------------------

    // What `move` costs `f` right now: base + weapon tax + armour tax (+ the armour's movement surcharge), then the
    // effects that bend it. consume: true when the move is really starting (one-shot discounts are used up).
    moveCost(f, move, consume) {
        if (move.isHold || f.has("noStamina")) return 0
        var load = f.loadout
        var tax = load.tax
        var weaponTax = tax[0]
        var armourTax = tax[1]
        var mobile = move.isMovement || move.isDodge
        // Wild armour: the next Move (late: or Dodge) after a dodge pays no armour tax.
        if (f.counter("wildFree") > 0 && (move.isMovement || (load.armourLate && move.isDodge))) {
            armourTax = 0
            if (consume) f.setCounter("wildFree", 0)
        }
        var cost = move.cost + weaponTax + armourTax + (f.has("tax") ? f.trait("tax") : 0)
        if (mobile) {
            cost = cost + load.surcharge - Rules.r([0, 1, 2, 3], load.armourRank("Skirmisher"))
            // Skirmisher capstone: the first movement move of each chain costs nothing.
            if (load.capstone("Skirmisher") && move.isMovement && f.counter("skirmishFree") == 0) {
                cost = 0
                if (consume) f.setCounter("skirmishFree", 1)
            }
        }
        // Gold armour hit this combatant: its next move costs more.
        if (f.counter("goldTax") > 0) {
            cost = cost * (1 + f.counter("goldTax"))
            if (consume) f.setCounter("goldTax", 0)
        }
        cost = cost.round
        return cost < 0 ? 0 : cost
    }

    // Stamina per second in Hold: Chill slows it, Last Stand's capstone doubles it below 20% HP.
    holdRecovery(f) {
        var rate = f.stats.recovery
        if (f.chill > 0) rate = rate * (1 - f.chillStrength)
        if (f.loadout.capstone("Last Stand") && f.stats.fraction < 0.2) rate = rate * 2
        return rate
    }

    onChainStart(f) {
        f.setCounter("skirmishFree", 0)
    }

    onMoveStart(f, move) {
        var load = f.loadout
        // Riposte (weapon): an attack that starts within 0.7 s of your block peak.
        var blockedAt = f.counter("blockPeakAt")
        f.setCounter("riposte", (move.isAttack && blockedAt > 0 && _e.clock - blockedAt <= 0.7) ? 1 : 0)
        // Wildfire: when you move, Burn spreads from any burning enemy beside you to its neighbours.
        if (move.isMovement && load.pairing == "Wildfire") {
            for (enemy in _e.adjacentEnemies(f)) {
                if (enemy.burn > 0) {
                    for (other in _e.neighbours(enemy)) {
                        if (other.side == enemy.side && other.burn == 0) addBurn(other, f, 1, false)
                    }
                }
            }
        }
    }

    // --- damage ----------------------------------------------------------------------------

    // The weapon numbers `move` uses for `f`: [power, stagger multiplier, element or null, item or null].
    weaponFor(f, move) {
        if (f.has("natural")) {
            var natural = f.trait("natural")
            return [natural[0], natural[1], attackElement(f), null]
        }
        var item = f.loadout.itemFor(move)[1]
        if (item == null) return [0, 1, null, null]
        return [item.power, item.type.staggerMul, item.element, item]
    }

    scalingOf(f, move) {
        var mana = move.scaling == "mana" || (f.has("scaling") && f.trait("scaling") == "mana")
        return mana ? f.stats.mana + f.loadout.manaBonus : f.stats.attack
    }

    // The summed, capped damage bonus % of every custom effect (and Magnetize) for this hit.
    damageBonus(a, t, move, isAim) {
        var load = a.loadout
        var bonus = 0
        if (a.counter("riposte") > 0) bonus = bonus + Rules.r([0, 0.2, 0.3, 0.4], load.weaponRank("Riposte"))
        var momentum = load.weaponRank("Momentum")
        if (momentum > 0) bonus = bonus + a.counter("momentumHits") * Rules.r([0, 0.06, 0.08, 0.1], momentum)
        if (load.weaponRank("Opportunist") > 0 && (t.inRecovery || t.counter("recoverUntil") > _e.clock)) {
            bonus = bonus + Rules.r([0, 0.25, 0.4, 0.6], load.weaponRank("Opportunist"))
        }
        if (load.weaponRank("Vanguard") > 0 && (_e.inFrontRow(a) || (load.capstone("Vanguard") && _e.inEnemyHalf(a)))) {
            bonus = bonus + Rules.r([0, 0.15, 0.2, 0.3], load.weaponRank("Vanguard"))
        }
        if (move.isMovement) bonus = bonus + Rules.r([0, 0.15, 0.25, 0.35], load.weaponRank("Skirmisher"))
        if (t.guardNow > 0) bonus = bonus + Rules.r([0, 0.5, 0.75, 1], load.weaponRank("Breaker"))
        var lastStand = load.weaponRank("Last Stand")
        if (lastStand > 0) bonus = bonus + ((1 - a.stats.fraction) * 10).floor * Rules.r([0, 0.01, 0.015, 0.02], lastStand)
        if (a.counter("reprisal") > 0) bonus = bonus + Rules.r([0, 0.15, 0.25, 0.35], load.weaponRank("Reprisal"))
        if (a.startedAfterHold && move.scaling == "mana") bonus = bonus + Rules.r([0, 0.1, 0.15, 0.2], load.weaponRank("Focus"))
        if (load.pairing == "Magnetize") bonus = bonus + a.counter("charges") * 0.1
        return bonus > CAP_DAMAGE ? CAP_DAMAGE : bonus
    }

    // The summed, capped defence bonus % - applies to passive defence and to the block alike.
    defenceBonus(t, g) {
        var load = t.loadout
        var bonus = 0
        var momentum = load.armourRank("Momentum")
        if (momentum > 0) bonus = bonus + t.counter("momentumBlocks") * Rules.r([0, 0.06, 0.08, 0.1], momentum)
        if (load.armourRank("Vanguard") > 0 && (_e.inFrontRow(t) || (load.capstone("Vanguard") && _e.inEnemyHalf(t)))) {
            bonus = bonus + Rules.r([0, 0.15, 0.2, 0.3], load.armourRank("Vanguard"))
        }
        var lastStand = load.armourRank("Last Stand")
        if (lastStand > 0) bonus = bonus + ((1 - t.stats.fraction) * 10).floor * Rules.r([0, 0.01, 0.015, 0.02], lastStand)
        // Timed buffs: Guardian (from the ally ahead) and Bastion (Radiance overheal).
        if (t.counter("defBuffUntil") > _e.clock) bonus = bonus + t.counter("defBuff")
        // Iron armour: stronger at the peak of a Defend move.
        if (bodyElement(t) == "Iron" && g >= 0.9) bonus = bonus + (t.loadout.armourLate ? 0.25 : 0.15)
        return bonus > CAP_DEFENCE ? CAP_DEFENCE : bonus
    }

    // The passive defence of `t` (its stat - or a boss's Granite Skin - plus armour), before bonuses.
    passiveDefence(t) {
        var base = t.stats.defence
        if (t.has("granite")) {
            var granite = t.trait("granite")
            base = t.exposedNow ? granite[1] : granite[0]
        }
        return base + t.loadout.armourDefence
    }

    // The value of a Defend move at guard level g (0..1) for its user: (base + the shield's guard) x g.
    blockValue(u, g) {
        var move = u.move
        if (move == null || !move.isDefend || g <= 0) return 0
        var value = move.defence
        var shield = u.loadout.shield
        if (move.addsShield && shield != null) value = value + shield.guard
        if (u.loadout.pairing == "Grounded") value = value * 0.85
        return value * g
    }

    // Final damage of one hit. share: the stencil share of the cell. isAim: the aim cell (vs a side / depth cell).
    // block: the active block value protecting the target (own or an ally's cover), already scaled by its curve.
    // Returns [final, raw].
    damage(a, t, move, share, isAim, depthCell, block, g) {
        var weapon = weaponFor(a, move)
        var power = weapon[0]
        var item = weapon[3]
        var raw = (move.damage + power) * (1 + scalingOf(a, move) / 100) * share

        // Type quirks from the weapon table.
        if (item != null) {
            var id = item.type.id
            if (id == "claymore" && !isAim && !depthCell) raw = raw * 1.15
            if (id == "spear" && depthCell) raw = raw * 1.25
            if (id == "pike" && move.aim == "reach" && move.reach == 1) raw = raw * 0.5
        }
        // Backstab: the target faces away from the attacker.
        if (move.backstab > 1 && _e.isBehind(a, t)) {
            var mult = move.backstab
            if (item != null && item.type.id == "stiletto" && mult < 2.5) mult = 2.5
            raw = raw * mult
        }
        if (!isAim) raw = raw * (1 + Rules.r([0, 0.25, 0.5, 0.75], a.loadout.weaponRank("Precision")))
        raw = raw * (1 + damageBonus(a, t, move, isAim))
        // Shatterpoint: Quake hits on a Frozen target deal +50% and consume Frozen.
        if (a.loadout.pairing == "Shatterpoint" && t.frozen > 0) {
            raw = raw * 1.5
            t.frozen = 0
            _e.note(t, "Shatterpoint")
        }
        // A boss part caught in an Exposed window.
        if (t.exposedNow) raw = raw * 1.5
        raw = raw * Elements.resistance(bodyElement(t), weapon[2])

        var bonus = 1 + defenceBonus(t, g)
        var final = raw * 100 / (100 + passiveDefence(t) * bonus) - block * bonus
        // Granite Skin: outside an Exposed window a hit only scratches.
        if (t.has("granite") && !t.exposedNow) final = final * t.trait("granite")[2]
        // Gold matched set: stamina stolen above the maximum, banked as flat bonus damage (outside the cap).
        if (a.counter("goldBonus") > 0) {
            final = final + a.counter("goldBonus")
            a.setCounter("goldBonus", 0)
        }
        // Reductions against specific hits.
        var reduction = 0
        if (!isAim) reduction = reduction + Rules.r([0, 0.3, 0.5, 0.7], t.loadout.armourRank("Precision"))
        if (move.aim == "first") reduction = reduction + Rules.r([0, 0.1, 0.2, 0.3], t.loadout.armourRank("Piercing"))
        if (reduction > CAP_REDUCTION) reduction = CAP_REDUCTION
        final = final * (1 - reduction)
        // Guardian capstone: the ally directly behind a Guardian takes 15% less at all times.
        var ahead = _e.occupant(t.lane, t.row + _e.forward(t.side))
        if (ahead != null && ahead.side == t.side && ahead.loadout.capstone("Guardian")) final = final * 0.85
        if (final < 0) final = 0
        return [final.round, raw]
    }

    healAmount(a, move, share) {
        var weapon = weaponFor(a, move)
        var amount = (move.healAmount + weapon[0]) * (1 + scalingOf(a, move) / 100) * share
        if (a.startedAfterHold) amount = amount * (1 + Rules.r([0, 0.1, 0.15, 0.2], a.loadout.weaponRank("Focus")))
        if (a.loadout.pairing == "Tithe") amount = amount * 0.8
        return amount.round
    }

    // --- stagger ---------------------------------------------------------------------------

    // The raw stagger (seconds) this hit carries, before the target's own reductions and diminishing returns.
    staggerOf(a, t, move, share) {
        var weapon = weaponFor(a, move)
        var load = a.loadout
        var s = move.stagger * share * weapon[1]
        var bonus = Rules.r([0, 0.25, 0.35, 0.5], load.weaponRank("Heavy"))
        if (weapon[2] == "Iron") {
            bonus = bonus + (late(a) ? 0.3 : 0.2)                    // Quake
            if (t.frozen > 0) {
                bonus = bonus + 1                                    // Avalanche
                t.frozen = 0
                _e.note(t, "Avalanche")
            }
        }
        // Storm armour's Charges: spent on the next attack.
        var charges = a.counter("charges")
        if (charges > 0) {
            bonus = bonus + charges * (load.armourLate ? 0.2 : 0.15)
            if (load.pairing == "Cold Snap") addChill(t, a, charges, late(a))
            a.setCounter("charges", 0)
        }
        if (a.counter("ironEcho") > 0) {
            bonus = bonus + 0.1
            a.setCounter("ironEcho", 0)
        }
        s = s * (1 + bonus)
        if (a.counter("riposte") > 0 && load.weaponRank("Riposte") >= 3) s = s + 0.1
        if (load.weaponRank("Last Stand") >= 3 && a.stats.fraction < 0.3) s = s * 1.1
        return s
    }

    // The summed, capped share of a set-back `t` ignores right now.
    setbackReduction(t) {
        var load = t.loadout
        var cut = load.staggerRes
        cut = cut + Rules.r([0, 0.25, 0.35, 0.5], load.armourRank("Heavy"))
        if (t.inRecovery) cut = cut + Rules.r([0, 0.25, 0.4, 0.6], load.armourRank("Opportunist"))
        var move = t.move
        if (move != null && (move.isHeal || move.scaling == "mana")) cut = cut + Rules.r([0, 0.1, 0.2, 0.3], load.armourRank("Focus"))
        if (load.matched("Iron") && move != null && move.isDefend) cut = cut + (load.armourLate ? 0.5 : 0.3)
        if (t.has("staggerRes")) cut = cut + t.trait("staggerRes")
        return cut > CAP_SETBACK ? CAP_SETBACK : cut
    }

    // How a block's skip grows (Sentinel armour).
    skipBonus(t) { Rules.r([0, 0.1, 0.15, 0.2], t.loadout.armourRank("Sentinel")) }

    // What a guard-range cell's share becomes (Sentinel weapon / shield, Vanguard III armour on the sides).
    guardShare(u, share, side) {
        var s = share + Rules.r([0, 0.15, 0.25, 0.35], u.loadout.weaponRank("Sentinel"))
        if (side != 0 && u.loadout.armourRank("Vanguard") >= 3) s = s + 0.25
        return s > 1 ? 1 : s
    }

    // --- statuses --------------------------------------------------------------------------

    statusTime_(t, element, seconds) {
        var res = Elements.resistance(bodyElement(t), element)
        if (res < 1) return seconds / 2
        if (res > 1) return seconds * 1.5
        return seconds
    }

    // Adds Burn stacks from `source` (null = nobody in particular - a flask). Burn meeting Chill reacts instead:
    // Thermal Shock, when the two came from different combatants (or the source carries Temper / Quench).
    addBurn(t, source, stacks, isLate) {
        if (!t.active) return
        if (t.chill > 0) {
            if (canShock_(source, t.chillSource)) thermalShock(t, source, isLate)
            return
        }
        var max = (source != null && source.loadout.matched("Flame") && source.loadout.weaponLate) ? 4 : 3
        var before = t.burn
        t.burn = (t.burn + stacks).min(max.max(before))
        t.burnTime = statusTime_(t, "Flame", BURN_TIME)
        t.burnSource = source
        t.burnRate = isLate ? 0.02 : 0.015
        if (t.burn != before) _e.status(t, "Burn")
    }

    addChill(t, source, stacks, isLate) {
        if (!t.active) return
        if (t.burn > 0) {
            if (canShock_(source, t.burnSource)) thermalShock(t, source, isLate)
            return
        }
        var before = t.chill
        t.chill = (t.chill + stacks).min(3)
        t.chillTime = statusTime_(t, "Frost", CHILL_TIME)
        t.chillSource = source
        t.chillStrength = isLate ? 0.5 : 0.35
        if (t.chill != before) _e.status(t, "Chill")
        // Frost matched set: three stacks become Frozen - the next set-back taken is multiplied, then it ends.
        if (t.chill >= 3 && source != null && source.loadout.matched("Frost") && t.frozen == 0) {
            t.frozen = source.loadout.weaponLate ? 2 : 1.5
            _e.status(t, "Frozen")
        }
    }

    canShock_(source, other) {
        if (source == null || other == null || source != other) return true
        var pair = source.loadout.pairing
        return pair == "Temper" || pair == "Quench"
    }

    // Burn + Chill on one target: a burst of 5% max HP (late 7%) and a 1.0 s set-back; both statuses are consumed.
    thermalShock(t, source, isLate) {
        t.burn = 0
        t.chill = 0
        t.frozen = 0
        _e.note(t, "Thermal Shock")
        _e.dealDirect(source, t, (t.stats.maxHp * (isLate ? 0.07 : 0.05)).round, "Thermal Shock")
        _e.setback(t, 1.0, source)
    }

    // The damage a target's Burn still has left to deal.
    burnLeft_(t) { t.stats.maxHp * t.burnRate * t.burn * t.burnTime }

    // Every step: Burn / Bleed damage (pro-rated, so it never jumps in chunks), status and shield timers.
    tick(dt) {
        for (f in _e.fighters) {
            if (!f.active) continue
            if (f.burn > 0) {
                var tick = f.stats.maxHp * f.burnRate * f.burn * dt
                _e.dealDirect(f.burnSource, f, tick, null)
                var source = f.burnSource
                if (source != null && source.active) {
                    // Flame matched set: Burn also drains stamina. Beacon: Burn on enemies you burned heals you.
                    if (source.loadout.matched("Flame")) f.stats.spendStamina((source.loadout.weaponLate ? 2 : 1) * dt)
                    if (source.loadout.pairing == "Beacon") source.stats.heal(tick * 0.2)
                }
                f.burnTime = f.burnTime - dt
                if (f.burnTime <= 0) f.burn = 0
            }
            if (f.chill > 0) {
                f.chillTime = f.chillTime - dt
                if (f.chillTime <= 0) f.chill = 0
            }
            if (f.bleed > 0) {
                _e.dealDirect(null, f, f.stats.maxHp * 0.01 * f.bleed * dt, null)
                f.bleedTime = f.bleedTime - dt
                if (f.bleedTime <= 0) f.bleed = 0
            }
            if (f.shield > 0) {
                f.shieldTime = f.shieldTime - dt
                if (f.shieldTime <= 0) f.shield = 0
            }
        }
    }

    // --- the attacker's side of a hit ------------------------------------------------------

    // A hit by `a` with `move` got through to `t` for `dealt` (> 0). cell: [lane, row] it landed in. stagger: what
    // it carried. Applies the weapon's element (after any party reaction it sets off) and the weapon effects.
    onHit(a, t, move, dealt, cell, isAim, stagger) {
        var load = a.loadout
        var weapon = weaponFor(a, move)
        var element = weapon[2]
        var isLate = late(a)

        // Streaks and one-shots.
        var momentum = load.weaponRank("Momentum")
        if (momentum > 0) a.addCounter("momentumHits", 1, [0, 3, 4, 5][momentum])
        a.setCounter("reprisal", 0)
        if (a.has("bleed")) {
            t.bleed = (t.bleed + 1).min(3)
            t.bleedTime = BLEED_TIME
            _e.status(t, "Bleed")
        }

        // Healing from damage dealt (Radiance, Bloodthirst, Tithe, Indulgence): summed, capped at 15% of the hit.
        var steal = Rules.r([0, 0.03, 0.05, 0.08], load.weaponRank("Bloodthirst"))
        var flatHeal = 0
        if (element == "Light") {
            var radiance = isLate ? 0.1 : 0.06
            if (load.pairing == "Second Wind" && a.counter("secondWind") > 0) {
                radiance = radiance * 2
                a.setCounter("secondWind", 0)
            }
            steal = steal + radiance
        }
        if (element == "Gold" && (load.pairing == "Indulgence" || load.pairing == "Tithe")) flatHeal = a.stats.maxHp * 0.02
        if (steal > CAP_LIFESTEAL) steal = CAP_LIFESTEAL
        var healing = dealt * steal + flatHeal
        if (healing > 0) {
            var over = a.stats.heal(healing)
            _e.healed(a, healing - over)
            if (over > 0) {
                // Light matched set / Bloodthirst capstone: overheal becomes a 5 s shield. Bastion: +10% defence.
                if (load.matched("Light") || load.capstone("Bloodthirst")) {
                    var cap = a.stats.maxHp * ((load.matched("Light") && load.weaponLate) ? 0.2 : 0.1)
                    a.shield = (a.shield + over).min(cap)
                    a.shieldTime = 5
                }
                if (load.pairing == "Bastion") {
                    a.setCounter("defBuff", 0.1)
                    a.setCounter("defBuffUntil", _e.clock + 3)
                }
            }
        }

        // Tempo (weapon): a landed hit refunds part of the move's cost - once per move, never more than it cost.
        var tempo = load.weaponRank("Tempo")
        if (tempo > 0 && !a.hitLanded) {
            var refund = a.cost * Rules.r([0, 0.15, 0.25, 0.35], tempo)
            if (load.capstone("Tempo") && a.chainIndex >= a.chain.count - 1) refund = a.cost
            a.stats.gainStamina(refund)
        }
        a.hitLanded = true

        // Guardian (weapon): hitting an enemy in your lane steels the ally behind you.
        var guardian = load.weaponRank("Guardian")
        if (guardian > 0 && t.lane == a.lane) {
            var behind = _e.occupant(a.lane, a.row - _e.forward(a.side))
            if (behind != null && behind.side == a.side) {
                behind.setCounter("defBuff", Rules.r([0, 0.1, 0.15, 0.2], guardian))
                behind.setCounter("defBuffUntil", _e.clock + 3)
            }
        }

        if (element != null) elementHit_(a, t, element, isLate, dealt, cell, stagger, 1)
    }

    // The on-hit effect of `element`, at `scale` (1 = a real hit; Echo passes its own reduced amounts instead).
    elementHit_(a, t, element, isLate, dealt, cell, stagger, scale) {
        var load = a.loadout
        var other = t.burnSource != a || t.chillSource != a
        if (element == "Flame") {
            addBurn(t, a, 1, isLate)
        } else if (element == "Frost") {
            addChill(t, a, 1, isLate)
        } else if (element == "Storm") {
            arc_(a, t, dealt * (isLate ? 0.3 : 0.2), stagger, cell, isLate)
        } else if (element == "Iron") {
            // Quake's stagger is already in staggerOf. Eruption: the Burn left on the target goes off at once,
            // half of it into the cells beside. Forge: Quake on a Burning target refreshes Burn instead.
            if (t.burn > 0) {
                if (load.pairing == "Forge") {
                    t.burnTime = statusTime_(t, "Flame", BURN_TIME)
                } else if (t.burnSource != a) {
                    var left = burnLeft_(t)
                    t.burn = 0
                    _e.note(t, "Eruption")
                    _e.dealDirect(a, t, left.round, "Eruption")
                    for (n in _e.besideEnemies(t)) {
                        _e.dealDirect(a, n, (left * 0.5).round, "Eruption")
                    }
                }
            }
        } else if (element == "Wild") {
            gust_(a, t, isLate, dealt, stagger)
        } else if (element == "Light") {
            // Radiance's healing is in onHit. Judgement: the Burn left goes off at once and heals the attacker.
            if (t.burn > 0 && t.burnSource != a) {
                var left = burnLeft_(t).round
                t.burn = 0
                _e.note(t, "Judgement")
                _e.dealDirect(a, t, left, "Judgement")
                _e.healed(a, left - a.stats.heal(left))
            }
            if (load.pairing == "Tithe") drain_(a, t, 2)
        } else if (element == "Gold") {
            var amount = isLate ? 5 : 3
            // Frostbite (another's Chill) / Frozen Assets (the pairing): Drain steals double; Chill refreshes.
            if (t.chill > 0 && (t.chillSource != a || load.pairing == "Frozen Assets")) {
                amount = amount * 2
                if (t.chillSource != a) {
                    t.chillTime = statusTime_(t, "Frost", CHILL_TIME)
                    _e.note(t, "Frostbite")
                }
            }
            drain_(a, t, amount)
        }
    }

    // Steals stamina from `t` for `a`; with the Gold matched set, what doesn't fit becomes bonus damage.
    drain_(a, t, amount) {
        if (t.has("noStamina")) return
        var taken = amount.min(t.stats.stamina)
        t.stats.spendStamina(taken)
        var over = a.stats.gainStamina(taken)
        if (over > 0 && a.loadout.matched("Gold")) {
            a.addCounter("goldBonus", over, a.loadout.weaponLate ? 25 : 10)
        }
    }

    // Storm's Arc: `amount` damage jumps to the enemies in the cells beside the hit cell.
    arc_(a, t, amount, stagger, cell, isLate) {
        var load = a.loadout
        var ignite = t.burn > 0 && t.burnSource != a
        for (n in _e.enemiesBesideCell(a, cell[0], cell[1])) {
            var dmg = amount
            // Shatter: an Arc onto a Chilled target (another's Chill) deals double and consumes it.
            if (n.chill > 0 && n.chillSource != a) {
                dmg = dmg * 2
                n.chill = 0
                n.frozen = 0
                _e.note(n, "Shatter")
            }
            _e.dealDirect(a, n, dmg.round, "Arc")
            // Ignition: an Arc off a Burning target sets everyone it reaches alight.
            if (ignite) {
                addBurn(n, a, 1, isLate)
                _e.note(n, "Ignition")
            }
            // Storm matched set: Arcs carry stagger too. Grounded: they also carry Quake.
            if (load.matched("Storm") || load.pairing == "Grounded") {
                var carried = load.matched("Storm") ? stagger * (load.weaponLate ? 1 : 0.5) : 0
                if (load.pairing == "Grounded") carried = carried + stagger * 0.2
                if (carried > 0) _e.setback(n, carried, a)
            }
            if (load.pairing == "Overload") {
                n.stats.spendStamina(2)
            }
        }
    }

    // Wild's Gust: push the target one cell directly away from the attacker if that cell is free.
    gust_(a, t, isLate, dealt, stagger) {
        var load = a.loadout
        var burning = t.burn > 0 && t.burnSource != a
        var chilled = t.chill > 0 && t.chillSource != a
        var dir = _e.awayFrom(a, t)
        var toLane = t.lane + dir[0]
        var toRow = t.row + dir[1]
        // Firestorm: a Gust onto a Burning target spreads Burn to the cell it is pushed toward.
        if (burning) {
            var inWay = _e.occupant(toLane, toRow)
            if (inWay != null && inWay.side == t.side) {
                addBurn(inWay, a, 1, isLate)
                _e.note(inWay, "Firestorm")
            }
        }
        if (_e.push(t, dir[0], dir[1])) {
            if (load.pairing == "Scatter") drain_(a, t, 3)
            // Whiteout: a Gust onto a Chilled target chills everyone beside where it lands.
            if (chilled) {
                for (n in _e.neighbours(t)) {
                    if (n.side == t.side) addChill(n, a, 1, isLate)
                }
                _e.note(t, "Whiteout")
            }
        } else {
            // A blocked push: the Wild matched set turns it into extra stagger; late Gust adds a 0.1 s set-back.
            if (load.matched("Wild")) {
                _e.setback(t, stagger * (load.weaponLate ? 0.5 : 0.3), a)
            } else if (isLate) {
                _e.setback(t, 0.1, a)
            }
            // Squall: a Gust that collides lets off an Arc beside the collision.
            if (load.pairing == "Squall") arc_(a, t, dealt * 0.2, 0, [toLane, toRow], isLate)
        }
    }

    // --- the defender's side ---------------------------------------------------------------

    // `t` took `dealt` damage from `a`'s hit.
    onDamaged(t, a, dealt) {
        var load = t.loadout
        var armour = bodyElement(t)
        // Momentum resets when you're hit (its capstone lets the streak survive the first one).
        if (t.counter("momentumHits") > 0) {
            if (load.capstone("Momentum") && t.counter("momentumSaved") == 0) {
                t.setCounter("momentumSaved", 1)
            } else {
                t.setCounter("momentumHits", 0)
                t.setCounter("momentumSaved", 0)
            }
        }
        t.setCounter("momentumBlocks", 0)
        if (load.weaponRank("Reprisal") > 0) t.setCounter("reprisal", 1)
        // Reprisal (armour): the attacker takes a share of the damage back.
        var reprisal = load.armourRank("Reprisal")
        if (reprisal > 0) {
            _e.dealDirect(t, a, (dealt * Rules.r([0, 0.05, 0.08, 0.12], reprisal)).round, "Reprisal")
            if (load.capstone("Reprisal")) _e.setback(a, 0.2, t)
        }
        // Guardian (armour): allies whose guard range covers the one who was hit gain stamina.
        for (ally in _e.alliesOf(t)) {
            var guardian = ally.loadout.armourRank("Guardian")
            if (guardian > 0 && _e.covers(ally, t)) ally.stats.gainStamina(Rules.r([0, 5, 8, 12], guardian))
        }

        var pair = load.pairing
        if (armour == "Frost" && pair != "Temper") {
            addChill(a, t, 1, load.armourLate)
            if (load.armourLate) a.setCounter("halveSetback", 1)
            echo_(t, a, dealt)
        } else if (armour == "Gold") {
            a.setCounter("goldTax", load.armourLate ? 0.2 : 0.1)
            echo_(t, a, dealt)
        }
    }

    // A hit landed on a Defend window. t: the one whose block absorbed it (the guarder, for a covered ally).
    // g: the guard level (0..1), near: seconds from the window's peak, s: the stagger the hit carried.
    onBlock(t, a, g, near, s, dealt) {
        var load = t.loadout
        var armour = bodyElement(t)
        var pair = load.pairing
        var atPeak = g >= 0.9
        if (atPeak) t.setCounter("blockPeakAt", _e.clock)
        var momentum = load.armourRank("Momentum")
        if (momentum > 0) t.addCounter("momentumBlocks", 1, [0, 3, 4, 5][momentum])
        // Breaker (armour): every hit you block drains the attacker.
        var breaker = load.armourRank("Breaker")
        if (breaker > 0 && !a.has("noStamina")) a.stats.spendStamina(Rules.r([0, 3, 5, 8], breaker))
        // Riposte (armour): a block at the peak returns part of the hit's stagger as a set-back.
        var riposte = load.armourRank("Riposte")
        if (riposte > 0 && atPeak) _e.setback(a, s * Rules.r([0, 0.3, 0.5, 0.7], riposte), t)
        // Permafrost: Chilled attackers that hit your guard take the stagger they would have dealt.
        if (pair == "Permafrost" && a.chill > 0) _e.setback(a, s, t)
        // Toll: a block at the peak also Drains the attacker, as if you'd hit them.
        if (pair == "Toll" && atPeak) drain_(t, a, load.weaponLate ? 5 : 3)

        if (armour == "Flame" && g >= 0.75 && pair != "Quench") {
            addBurn(a, t, load.armourLate ? 2 : 1, load.armourLate)
            echo_(t, a, dealt)
        } else if (armour == "Light" && near <= 0.1) {
            var heal = t.stats.maxHp * (load.armourLate ? 0.05 : 0.03)
            if (pair == "Indulgence") heal = heal / 2
            _e.healed(t, heal - t.stats.heal(heal))
            echo_(t, a, dealt)
        } else if (armour == "Iron" && atPeak) {
            echo_(t, a, dealt)
        }
    }

    // `t` was inside a dodge window when `a`'s hit arrived.
    onDodge(t, a) {
        var load = t.loadout
        var armour = bodyElement(t)
        var pair = load.pairing
        var bloodthirst = load.armourRank("Bloodthirst")
        if (bloodthirst > 0) {
            var heal = t.stats.maxHp * Rules.r([0, 0.01, 0.02, 0.03], bloodthirst)
            _e.healed(t, heal - t.stats.heal(heal))
        }
        var tempo = load.armourRank("Tempo")
        if (tempo > 0) t.stats.gainStamina(t.cost * Rules.r([0, 0.3, 0.5, 0.7], tempo))
        if (pair == "Second Wind") t.setCounter("secondWind", 1)
        if (armour == "Storm") {
            var max = pair == "Magnetize" ? 2 : (load.armourLate ? 3 : 2)
            t.addCounter("charges", 1, max)
            _e.status(t, "Charge")
            echo_(t, a, 0)
        } else if (armour == "Wild") {
            t.setCounter("wildFree", 1)
            echo_(t, a, 0)
        }
        // Stormstep: a dodge that moves you fires a 0-damage Arc that sets back the cells beside where you land.
        if (pair == "Stormstep" && t.move != null && t.move.isMovement) {
            for (n in _e.enemiesBesideCell(t, t.lane, t.row)) {
                _e.setback(n, 0.2, t)
            }
        }
    }

    // Echo - the default for unnamed weapon x armour pairs: when the armour's effect triggers, the weapon's element
    // also fires once on that enemy at reduced strength.
    echo_(t, a, dealt) {
        var load = t.loadout
        if (load.pairing != "Echo") return
        var element = load.weaponElement
        if (element == "Flame") {
            addBurn(a, t, 1, false)
        } else if (element == "Frost") {
            addChill(a, t, 1, false)
        } else if (element == "Storm") {
            if (dealt > 0) arc_(t, a, dealt * 0.1, 0, [a.lane, a.row], false)
        } else if (element == "Iron") {
            t.setCounter("ironEcho", 1)
        } else if (element == "Wild") {
            var dir = _e.awayFrom(t, a)
            _e.push(a, dir[0], dir[1])
        } else if (element == "Light") {
            var heal = t.stats.maxHp * 0.02
            _e.healed(t, heal - t.stats.heal(heal))
        } else if (element == "Gold") {
            drain_(t, a, 2)
        }
        _e.note(t, "Echo")
    }
}
