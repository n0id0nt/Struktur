// Party.wren
// The player's party: an ordered list of Members (Party/Member.wren). Order is the follow chain - every NPC
// follows the member directly AHEAD of it in the list (see GameObjects/PartyMember.wren), players never follow.
// With several players in the party (`[P1, N1, P2, N2]`) the same rule just works: N1 trails P1, N2 trails P2.
//
//   - Joining/leaving only changes this list (and saves it); States/ExperimentState.syncPartyFollowers() spawns
//     and removes the matching world entities.
//   - Dialogue asks `Party.contains(name)` through the `inParty` condition (Dialogue/RegisteredFunctions/
//     Conditions.wren).
//   - Battles build their human-controlled side from `Party.combatants` (States/CombatState.wren).
//   - tick() records every member's path each fixed step so followers can walk the route their leader took
//     (Party/PartyTrail.wren, Party/TrailFollow.wren).
// Membership - and each NPC member's HP / stamina - is saved to party.sav like the inventory (players are rebuilt
// from the world each run, at full health).
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem
import "debug" for Debug
import "Party/Member" for Member

class Party {
    static init() {
        __members = []
        __version = 0
    }

    // Every member, in follow order (index 0 leads).
    static members { __members }

    // Bumped whenever the membership changes - a cheap "did anything change" check.
    static version { __version }

    // True when an NPC (or player) with this name is in the party. This is all the dialogue condition needs.
    static contains(name) { Party.member(name) != null }

    static member(name) {
        for (m in __members) {
            if (m.name == name) {
                return m
            }
        }
        return null
    }

    static indexOf(member) {
        for (i in 0...__members.count) {
            if (__members[i] == member) {
                return i
            }
        }
        return -1
    }

    // The member directly in front of `member` in the party list (who it follows), null for the leader.
    static ahead(member) {
        var i = Party.indexOf(member)
        return i > 0 ? __members[i - 1] : null
    }

    // --- players ---------------------------------------------------------------------

    // A human-controlled player entity enters the world: it takes a slot in front of every NPC (after any
    // earlier players). Safe to call again for the same name - it just rebinds the entity.
    static registerPlayer(entity, name) {
        var existing = Party.member(name)
        if (existing != null) {
            existing.entity = entity
            return existing
        }
        var index = 0
        while (index < __members.count && __members[index].isPlayer) {
            index = index + 1
        }
        var member = Member.new(name, "player", entity, index)
        __members.insert(index, member)
        __version = __version + 1
        return member
    }

    // The world is going away (or the player entity is being rebuilt): drop the player slots, keep the NPCs.
    static unregisterPlayers() {
        var kept = []
        for (m in __members) {
            if (!m.isPlayer) {
                kept.add(m)
            }
        }
        for (m in kept) {
            m.entity = null   // their follower entities die with the world
        }
        __members = kept
        __version = __version + 1
    }

    // --- NPCs ------------------------------------------------------------------------

    // Adds an NPC to the end of the party. spawnPosition (a Vec3, or null) is where their follower first
    // appears - normally where the NPC was standing when they joined. Safe to call twice.
    static join(name, spawnPosition) {
        if (Party.add_(name, spawnPosition)) {
            Party.save("party.sav")
        }
    }

    static join(name) { Party.join(name, null) }

    static add_(name, spawnPosition) {
        if (Party.contains(name)) {
            return false
        }
        var member = Member.new(name, "npc", null, 0)
        member.spawnPosition = spawnPosition
        __members.add(member)
        __version = __version + 1
        Debug.info("[Party] %(name) joined the party")
        return true
    }

    static leave(name) {
        var member = Party.member(name)
        if (member == null) {
            return
        }
        if (member.entity != null && GameObject.isValid(member.entity) && !member.isPlayer) {
            GameObject.destroy(member.entity)
        }
        __members.remove(member)
        __version = __version + 1
        Debug.info("[Party] %(name) left the party")
        Party.save("party.sav")
    }

    // --- world / battle helpers -------------------------------------------------------

    // The member's current world position (Vec3), or null if they have no live entity.
    static positionOf(member) {
        if (member.entity == null || !GameObject.isValid(member.entity)) {
            return null
        }
        return WorldTransform.getPosition(member.entity)
    }

    // Records every member's position on their trail. Call once per fixed step, before follower scripts run.
    static tick() {
        for (m in __members) {
            var position = Party.positionOf(m)
            if (position != null) {
                m.trail.record(position.x, position.y)
            }
        }
    }

    // The battle Combatant for a member (null if a player has no live Player script).
    static combatantOf(member) {
        if (member.isPlayer) {
            var script = Script.getInstance(member.entity)
            return script == null ? null : script.combatant
        }
        return member.npcCombatant
    }

    // Everyone who fights on the player's side, in party order (players first).
    static combatants {
        var out = []
        for (m in __members) {
            var c = Party.combatantOf(m)
            if (c != null) {
                out.add(c)
            }
        }
        return out
    }

    // --- health -------------------------------------------------------------------------

    // True when at least one member can fight - nobody standing means no fight can start (ExperimentState).
    static anyoneStanding {
        for (m in __members) {
            var c = Party.combatantOf(m)
            if (c != null && c.alive) {
                return true
            }
        }
        return false
    }

    // After a fight the party won or fled: a player on 0 HP gets back up on 1 - the leader can't stay down (only
    // NPC members are "downed" until rested - see Member.downed). A loss is handled by the game-over flow instead.
    static reviveLeaders() {
        for (m in __members) {
            var c = m.isPlayer ? Party.combatantOf(m) : null
            if (c != null && !c.alive) {
                c.stats.heal(1)
            }
        }
    }

    // After a duel (won or lost): nobody is really hurt - every member on 0 HP, NPCs included, gets back up on 1.
    // A duel never downs anyone or ends in a game over.
    static reviveAllFallen() {
        for (m in __members) {
            var c = Party.combatantOf(m)
            if (c != null && !c.alive) {
                c.stats.heal(1)
            }
        }
    }

    // A rest spot (the "restParty" dialogue command): everyone back to full HP and stamina - downed members
    // included - then saved.
    static restAll() {
        for (m in __members) {
            var c = Party.combatantOf(m)
            if (c != null) {
                c.stats.heal(c.stats.maxHp)
                c.stats.gainStamina(c.stats.maxStamina)
            }
        }
        Party.save("party.sav")
    }

    // --- persistence -----------------------------------------------------------------

    // Saves the NPC members' names plus their HP / stamina (parallel lists; -1 = no Combatant minted yet, i.e.
    // still at full health). Players are rebuilt from the world each run.
    static save(path) {
        var names = []
        var hp = []
        var stamina = []
        for (m in __members) {
            if (!m.isPlayer) {
                names.add(m.name)
                var c = Party.combatantOf(m)
                hp.add(c == null ? -1 : c.stats.hp)
                stamina.add(c == null ? -1 : c.stats.stamina)
            }
        }
        var json = Json.stringify(MapUtil.mapToPairs({"members": names, "hp": hp, "stamina": stamina}))
        var result = FileSystem.writeString(path, json)
        if (result.success) {
            FileSystem.syncSaves()
        }
    }

    static load(path) {
        var result = FileSystem.readString(path)
        if (!result.success) {
            return
        }
        var data = Json.parse(result.value)
        var names = data == null ? null : data["members"]
        var hp = data == null ? null : data["hp"]
        var stamina = data == null ? null : data["stamina"]
        if (names != null) {
            for (i in 0...names.count) {
                Party.add_(names[i], null)
                // Saves from before HP was persisted have no hp/stamina lists - those members load at full health.
                var savedHp = (hp != null && i < hp.count && hp[i] >= 0) ? hp[i] : null
                var savedStamina = (stamina != null && i < stamina.count && stamina[i] >= 0) ? stamina[i] : null
                Party.member(names[i]).restore(savedHp, savedStamina)
            }
        }
    }
}
