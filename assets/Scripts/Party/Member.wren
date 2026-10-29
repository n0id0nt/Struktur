// Party/Member.wren
// One slot in the party (see Party.wren): who they are, what they look like in the world, and the persistent
// battle Combatant they fight as.
//
//   kind "player" - a human-controlled Player entity (never follows anyone).
//   kind "npc"    - an NPC that follows the member ahead of them in the party list (GameObjects/PartyMember.wren)
//                   and is human-controlled in battle (the human picks their moves).
import "Party/PartyTrail" for PartyTrail
import "Combat/Config/BattleCritter" for BattleCritter

// Trail tuning: record a point every 4px of travel and keep ~320px of path behind the member.
var TRAIL_MIN_STEP = 4
var TRAIL_MAX_LENGTH = 320

class Member {
    // entity: the world entity (a Player for kind "player"; null for an NPC until its follower is spawned).
    // ownerPlayer: index of the human who controls this member in battle (0 = the first/only player). Today one
    // human controls everyone; real co-op will route different controllers by this.
    construct new(name, kind, entity, ownerPlayer) {
        _name = name
        _kind = kind
        _entity = entity
        _ownerPlayer = ownerPlayer
        _combatant = null
        _savedHp = null
        _savedStamina = null
        _spawnPosition = null
        _trail = PartyTrail.new(TRAIL_MIN_STEP, TRAIL_MAX_LENGTH)
    }

    name { _name }
    kind { _kind }
    isPlayer { _kind == "player" }

    entity { _entity }
    entity=(value) { _entity = value }

    ownerPlayer { _ownerPlayer }

    // Where the follower should appear the first time it is spawned (the position the NPC stood at when
    // they joined) - null means "right behind the member ahead". Cleared once used.
    spawnPosition { _spawnPosition }
    spawnPosition=(value) { _spawnPosition = value }

    // This member's recent path - what the member behind them follows.
    trail { _trail }

    // An NPC member's persistent battle Combatant (HP / stamina carry between fights, and across runs via
    // party.sav - see restore), minted once from its battle profile. (A player member's lives on its Player script
    // instead - see Party.combatantOf.)
    npcCombatant {
        if (_combatant == null) {
            _combatant = BattleCritter.forName(_name).makeCombatant(null)
            applySaved_()
        }
        return _combatant
    }

    // True for an NPC member knocked out in a fight and not healed since (0 HP): they sit out every fight
    // (States/CombatState.wren) and their follower is greyed out (GameObjects/PartyMember.wren) until a rest spot
    // (Party.restAll) gets them back up. Players are never "downed" - see Party.reviveLeaders.
    downed { !isPlayer && !npcCombatant.alive }

    // HP / stamina from party.sav (Party.load), applied to the Combatant now or whenever it's first minted.
    restore(hp, stamina) {
        _savedHp = hp
        _savedStamina = stamina
        if (_combatant != null) {
            applySaved_()
        }
    }

    applySaved_() {
        var s = _combatant.stats
        if (_savedHp != null && _savedHp < s.maxHp) {
            s.takeDamage(s.maxHp - _savedHp)
        }
        if (_savedStamina != null && _savedStamina < s.maxStamina) {
            s.spendStamina(s.maxStamina - _savedStamina)
        }
        _savedHp = null
        _savedStamina = null
    }
}
