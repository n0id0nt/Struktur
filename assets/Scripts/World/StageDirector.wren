// World/StageDirector.wren
// Moves and animates characters for dialogue scenes - the "moveTo", "teleport", "face", "playAnimation" and "wait"
// dialogue commands (Dialogue/RegisteredFunctions/Commands.wren) hand their work to it. A command only starts the
// action; update() (called every frame by States/GameWorldState.wren) plays it out. Actions started with wait on
// (the default for moveTo and wait) make the conversation hold until they finish: States/InteractState.wren checks
// `blocking` before showing the next line, and hides the box meanwhile so the scene can be seen.
//
// Characters ("who") are found by name: "Player", a party member ("Kira"), or anything World/Npcs.wren finds - an
// LDtk NPC by its Name, an Interactable by its MessageKey. Where they go ("to") is the Name of an LDtk "Path"
// (GameObjects/Path.wren - walked through its WayPoints in order, so routes can turn corners) or of a "Marker"
// (GameObjects/Marker.wren - a single spot), found among the loaded rooms - the one nearest the character when several
// share it.
// Characters with a generated sprite sheet (Sprites/SpriteSheet.wren - the player, NPCs, party members) get the
// directional clip for what they're doing: "Walk" / "Run" while moving, "Idle" after, or whatever playAnimation names.
// A player or party member being moved is put in scripted mode (setScripted - its own movement code stands aside).
// So is one held in a pose by playAnimation (anything but Idle) - until a move, face or "Idle" replaces it, or the
// conversation ends (releasePoses, from States/InteractState.wren). An NPC keeps its pose after the conversation (Finn
// carries on Working at his anvil) until a scene changes it - its wandering (World/PathWalkers.wren) waits meanwhile.
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform, SpriteAnimation, Sprite, PhysicsBody
import "math" for Vec2, Vec3
import "app" for Time, Application
import "reflect" for Reflect
import "debug" for Debug
import "World/Npcs" for Npcs
import "World/Paths" for Paths
import "Sprites/SpriteSheet" for SpriteSheet

// Movement speeds in px per second - the player's own walk / run speeds (Player.wren: 1.5 / 3 m/s at 32 px/m).
var WALK_SPEED = 48.0
var RUN_SPEED = 96.0
// One LDtk grid cell, in px - how far in front of the player moveToPlayer stops.
var TILE = 16

// Direction words a dialogue can use (face / Marker Facing) -> a unit vector, +y down the screen.
var DIRECTIONS = {
    "up": [0, -1], "down": [0, 1], "left": [-1, 0], "right": [1, 0],
    "upleft": [-0.7071, -0.7071], "upright": [0.7071, -0.7071],
    "downleft": [-0.7071, 0.7071], "downright": [0.7071, 0.7071],
    "north": [0, -1], "south": [0, 1], "west": [-1, 0], "east": [1, 0]
}

class StageDirector {
    // True while an action started with wait on is still playing - the conversation holds until it's done.
    static blocking {
        for (action in StageDirector.actions_) {
            if (action["wait"]) return true
        }
        return false
    }

    static actions_ {
        if (__actions == null) __actions = []
        return __actions
    }

    // Characters held in a pose by playAnimation - entity -> true (see the header).
    static posed_ {
        if (__posed == null) __posed = {}
        return __posed
    }

    // Last facing set per entity (for characters with no facing of their own, e.g. NPCs) - entity -> Vec2.
    static facings_ {
        if (__facings == null) __facings = {}
        return __facings
    }

    // --- commands ---------------------------------------------------------------

    // True while a scene is moving or posing `entity` - its own wandering (World/PathWalkers.wren) waits.
    static directing(entity) {
        if (StageDirector.posed_.containsKey(entity)) return true
        for (action in StageDirector.actions_) {
            if (action["entity"] == entity) return true
        }
        return false
    }

    // Walks `who` along the Path called `to` (through each of its WayPoints), or to the Marker called `to`. `to` can also
    // be "Player": the same as moveToPlayer.
    // speed: "walk" (default), "run", or px per second. animation: the clip to move with instead of Walk / Run (e.g.
    // Finn's "StandardWalk"), or null.
    // A character already on a move (from an earlier command - even one in the same node) finishes it first: moves
    // queue up, so `moveTo FinnToTable` then `moveToPlayer` walks the Path and then over to the player.
    static moveTo(who, to, speed, wait, animation) {
        if (to == "Player") return StageDirector.moveToPlayer(who, speed, wait, animation)
        StageDirector.queueMove_(who, to, speed, wait, animation)
    }

    // Walks `who` to the tile in front of the player - one tile (TILE px) the way the player is facing - and turns them
    // to face the player. Where that is gets worked out when the walk starts, so after a queued Path it's wherever the
    // player is by then.
    static moveToPlayer(who, speed, wait, animation) {
        StageDirector.queueMove_(who, null, speed, wait, animation)
    }

    static queueMove_(who, to, speed, wait, animation) {
        var entity = StageDirector.actor(who)
        if (entity == null) return
        var pxPerSecond = WALK_SPEED
        var clip = "Walk"
        if (speed == "run") {
            pxPerSecond = RUN_SPEED
            clip = "Run"
        } else if (speed is Num && speed > 0) {
            pxPerSecond = speed
            clip = speed > (WALK_SPEED + RUN_SPEED) / 2 ? "Run" : "Walk"
        }
        if (animation != null && StageDirector.hasClip_(entity, animation, true)) clip = animation
        var leg = {"to": to, "speed": pxPerSecond, "clip": clip}
        var current = StageDirector.moveOf_(entity)
        if (current != null) {
            // Queue behind the move it's on; the conversation holds for the lot if any part waits.
            current["legs"].add(leg)
            if (wait) current["wait"] = true
            return
        }
        StageDirector.cancelFor_(entity)
        var action = {"kind": "move", "entity": entity, "wait": wait, "legs": [leg]}
        if (!StageDirector.startLeg_(action)) return
        StageDirector.posed_.remove(entity)
        StageDirector.setScripted_(entity, true)
        StageDirector.actions_.add(action)
    }

    // The move `entity` is on, or null.
    static moveOf_(entity) {
        for (action in StageDirector.actions_) {
            if (action["kind"] == "move" && action["entity"] == entity) return action
        }
        return null
    }

    // Takes the next leg off a move and works out its targets; false if no legs are left that lead anywhere.
    static startLeg_(action) {
        var entity = action["entity"]
        while (action["legs"].count > 0) {
            var leg = action["legs"].removeAt(0)
            var here = WorldTransform.getPosition(entity)
            var targets = null
            var facingAfter = null
            var facePlayer = false
            var to = leg["to"]
            if (to == null) {
                targets = StageDirector.frontOfPlayer_(entity)
                facePlayer = true
            } else {
                var path = Paths.named(to, here)
                if (path != null) {
                    targets = path.points.toList
                } else {
                    var marker = StageDirector.marker_(to, here, false)
                    if (marker == null) {
                        Debug.warning("[StageDirector] no Path or Marker named '%(to)' in the loaded rooms")
                    } else {
                        var p = WorldTransform.getPosition(marker)
                        targets = [Vec2.new(p.x, p.y)]
                        facingAfter = StageDirector.direction(Script.getInstance(marker).facing)
                    }
                }
            }
            if (targets != null && targets.count > 0) {
                action["target"] = targets[0]
                action["queue"] = targets.skip(1).toList
                action["speed"] = leg["speed"]
                action["clip"] = leg["clip"]
                action["facingAfter"] = facingAfter
                action["facePlayer"] = facePlayer
                return true
            }
        }
        return false
    }

    // The spot one tile in front of the player (the way they face, to the nearest of up / down / left / right), as a
    // one-target list - or null if there's no player, or `entity` is the player.
    static frontOfPlayer_(entity) {
        var player = StageDirector.actor("Player")
        if (player == null || player == entity) return null
        var p = WorldTransform.getPosition(player)
        var f = StageDirector.facingOf_(player)
        var dir = f.x.abs > f.y.abs ? Vec2.new(f.x.sign, 0) : Vec2.new(0, f.y >= 0 ? 1 : -1)
        return [Vec2.new(p.x + dir.x * TILE, p.y + dir.y * TILE)]
    }

    // Puts `who` at the start of the Path called `to` (its first WayPoint), or on the Marker called `to` (facing its
    // Facing, if it has one), at once.
    static teleport(who, to) {
        var entity = StageDirector.actor(who)
        if (entity == null) return
        var here = WorldTransform.getPosition(entity)
        var target = null
        var facing = null
        var path = Paths.named(to, here)
        if (path != null) {
            if (path.points.count == 0) return
            target = path.points[0]
        } else {
            var marker = StageDirector.marker_(to, here, false)
            if (marker == null) {
                Debug.warning("[StageDirector] no Path or Marker named '%(to)' in the loaded rooms")
                return
            }
            target = WorldTransform.getPosition(marker)
            facing = StageDirector.direction(Script.getInstance(marker).facing)
        }
        StageDirector.cancelFor_(entity)
        WorldTransform.setPosition(entity, Vec3.new(target.x, target.y, 0))
        if (facing != null) {
            StageDirector.face_(entity, facing, "Idle")
            StageDirector.pose_(entity, false)
        }
    }

    // Turns `who` to a direction word ("up", "downLeft", "North"...) or towards another character / Marker by name.
    static face(who, towards) {
        var entity = StageDirector.actor(who)
        if (entity == null) return
        var dir = StageDirector.direction(towards)
        if (dir == null) {
            var here = WorldTransform.getPosition(entity)
            var other = StageDirector.actor_(towards, false)
            if (other == null) other = StageDirector.marker_(towards, here, false)
            if (other == null) {
                Debug.warning("[StageDirector] face: '%(towards)' is neither a direction, a character nor a marker")
                return
            }
            dir = StageDirector.towards_(entity, other)
            if (dir == null) return
        }
        StageDirector.face_(entity, dir, "Idle")
        StageDirector.pose_(entity, false)
    }

    // Plays one of `who`'s clips in the way it is facing - every sheet has Idle, Walk, Run, Hurt, Roll and Death; Finn
    // also has Working, LayingIdle, GettingUp, PickingUp and StandardWalk. A clip the character's sheet doesn't have is
    // skipped with a warning. Anything but Idle is held as a pose (see the header).
    // wait: hold the conversation until the clip has played once. then: a clip to switch to once it has (e.g.
    // GettingUp then Idle), or null to stay on the clip's last frame (a one-shot) / keep looping it.
    static playAnimation(who, animation, wait, then) {
        var entity = StageDirector.actor(who)
        if (entity == null) return
        if (!StageDirector.hasClip_(entity, animation, true)) return
        var dir = StageDirector.facingOf_(entity)
        StageDirector.cancelFor_(entity)
        StageDirector.face_(entity, dir, animation)
        StageDirector.pose_(entity, animation != "Idle")
        if (then != null && !StageDirector.hasClip_(entity, then, true)) then = null
        var info = SpriteSheet.clip(entity, SpriteSheet.clipName(dir, animation))
        if (info != null && (wait || then != null)) {
            StageDirector.actions_.add({
                "kind": "clip", "entity": entity, "until": Time.scaledTime + info[2], "wait": wait, "then": then
            })
        }
    }

    // Holds the conversation for `seconds`.
    static wait(seconds) {
        StageDirector.actions_.add({"kind": "wait", "until": Time.scaledTime + seconds, "wait": true})
    }

    // --- every frame ---------------------------------------------------------------

    static update() {
        var actions = StageDirector.actions_
        if (actions.count == 0) return
        var dt = Time.scaledDelta
        var finished = []
        for (action in actions) {
            if (action["kind"] == "wait") {
                if (Time.scaledTime >= action["until"]) finished.add(action)
            } else if (action["kind"] == "clip") {
                if (Time.scaledTime >= action["until"]) {
                    finished.add(action)
                    var entity = action["entity"]
                    if (action["then"] != null && GameObject.isValid(entity)) {
                        StageDirector.face_(entity, StageDirector.facingOf_(entity), action["then"])
                        StageDirector.pose_(entity, action["then"] != "Idle")
                    }
                }
            } else if (StageDirector.stepMove_(action, dt)) {
                finished.add(action)
            }
        }
        for (action in finished) {
            actions.remove(action)
        }
    }

    // Drops every action (a level unloading, the gameplay state closing) and hands scripted characters back.
    static clear() {
        for (action in StageDirector.actions_) {
            if (action["kind"] == "move" && GameObject.isValid(action["entity"])) {
                StageDirector.setScripted_(action["entity"], false)
            }
        }
        StageDirector.releasePoses()
        __actions = []
        __facings = {}
    }

    // The conversation is over: players and party members held in a pose go back to their own control (and idle).
    // Their pending "then" clips are dropped; NPCs keep their poses.
    static releasePoses() {
        var posed = []
        for (entity in StageDirector.posed_.keys.toList) {
            var script = GameObject.isValid(entity) ? Script.getInstance(entity) : null
            if (script == null || Reflect.hasMethod(script, "setScripted(_)")) {
                posed.add(entity)
                StageDirector.posed_.remove(entity)
                if (script != null) script.setScripted(false)
            }
        }
        var actions = StageDirector.actions_
        var stale = actions.where {|a| a["kind"] == "clip" && posed.contains(a["entity"]) }.toList
        for (a in stale) {
            actions.remove(a)
        }
    }

    // --- lookups ---------------------------------------------------------------------

    // The entity called `who` (see the header), or null with a warning.
    static actor(who) { StageDirector.actor_(who, true) }

    static actor_(who, warn) {
        if (who == "Player") {
            var players = GameObject.getAllWithIdentifier("Player")
            if (players.count > 0) return players[0]
        } else {
            for (entity in GameObject.getAllWithIdentifier("PartyMember")) {
                var script = Script.getInstance(entity)
                if (script != null && script.name == who) return entity
            }
            var npc = Npcs.first(who)
            if (npc != null) return npc
        }
        if (warn) Debug.warning("[StageDirector] no character '%(who)' here")
        return null
    }

    // The Marker called `name` nearest `near` (a position), or null with a warning.
    static marker(name, near) { StageDirector.marker_(name, near, true) }

    static marker_(name, near, warn) {
        var best = null
        var bestDistance = null
        for (entity in GameObject.getAllWithIdentifier("Marker")) {
            var script = Script.getInstance(entity)
            if (script == null || script.name != name) continue
            var distance = near == null ? 0 : Vec3.distance(WorldTransform.getPosition(entity), near)
            if (best == null || distance < bestDistance) {
                best = entity
                bestDistance = distance
            }
        }
        if (best == null && warn) Debug.warning("[StageDirector] no Marker named '%(name)' in the loaded rooms")
        return best
    }

    // A direction word -> unit Vec2, or null if it isn't one.
    static direction(word) {
        if (!(word is String)) return null
        var key = StageDirector.lowercase_(word)
        if (!DIRECTIONS.containsKey(key)) return null
        var d = DIRECTIONS[key]
        return Vec2.new(d[0], d[1])
    }

    // --- internals ---------------------------------------------------------------------

    // ASCII lower-case (Wren's String has no lower).
    static lowercase_(text) {
        var out = ""
        for (c in text.codePoints) {
            out = out + String.fromCodePoint(c >= 65 && c <= 90 ? c + 32 : c)
        }
        return out
    }

    // Advances one move; true once it has arrived.
    static stepMove_(action, dt) {
        var entity = action["entity"]
        if (!GameObject.isValid(entity)) return true
        var here = WorldTransform.getPosition(entity)
        var target = action["target"]
        var delta = Vec2.new(target.x - here.x, target.y - here.y)
        var distance = delta.length()
        var step = action["speed"] * dt
        var script = Script.getInstance(entity)
        if (script != null && Reflect.hasMethod(script, "setScripted(_)")) {
            // A dynamic body (player, party member) - keep physics from adding its own drift.
            PhysicsBody.setLinearVelocity(entity, Vec2.new(0, 0))
        }
        if (distance <= step || distance < 0.5) {
            WorldTransform.setPosition(entity, Vec3.new(target.x, target.y, 0))
            // A Path: on to its next WayPoint.
            if (action["queue"].count > 0) {
                action["target"] = action["queue"].removeAt(0)
                return false
            }
            var facing = action["facingAfter"]
            if (action["facePlayer"]) facing = StageDirector.towards_(entity, StageDirector.actor_("Player", false))
            if (facing == null) facing = distance > 0.001 ? delta.normalize() : StageDirector.facingOf_(entity)
            // Queued moves: on to the next one.
            if (StageDirector.startLeg_(action)) {
                StageDirector.face_(entity, facing, action["clip"])
                return false
            }
            StageDirector.face_(entity, facing, "Idle")
            StageDirector.setScripted_(entity, false)
            StageDirector.posed_.remove(entity)
            return true
        }
        var dir = delta.normalize()
        WorldTransform.setPosition(entity, Vec3.new(here.x + dir.x * step, here.y + dir.y * step, 0))
        StageDirector.face_(entity, dir, action["clip"])
        return false
    }

    // Sets the entity's facing (remembered, and passed on to a player / party member) and plays `clip` that way.
    static face_(entity, dir, clip) {
        StageDirector.facings_[entity] = dir
        var script = Script.getInstance(entity)
        if (script != null && Reflect.hasMethod(script, "facing=(_)")) {
            script.facing = dir
        }
        if (Sprite.get(entity) != null) {
            SpriteAnimation.setCurrentAnimation(entity, SpriteSheet.clipName(dir, clip))
        }
    }

    // Unit Vec2 from `entity` towards `other`, or null if there's no other / they're on the same spot.
    static towards_(entity, other) {
        if (other == null || !GameObject.isValid(other)) return null
        var a = WorldTransform.getPosition(entity)
        var b = WorldTransform.getPosition(other)
        var d = Vec2.new(b.x - a.x, b.y - a.y)
        if (d.length() < 0.001) return null
        return d.normalize()
    }

    static facingOf_(entity) {
        var script = Script.getInstance(entity)
        if (script != null && Reflect.hasMethod(script, "facing")) {
            return script.facing
        }
        if (StageDirector.facings_.containsKey(entity)) return StageDirector.facings_[entity]
        return Vec2.new(0, 1)
    }

    // Whether `entity`'s sheet has `animation` ("Working"...). A character without a recorded sheet (no sprite, or one
    // not made by Sprites/SpriteSheet.create) passes, so the command just does what it can.
    static hasClip_(entity, animation, warn) {
        if (!SpriteSheet.sheets_.containsKey(entity)) return true
        if (SpriteSheet.clip(entity, SpriteSheet.clipName(Vec2.new(0, 1), animation)) != null) return true
        if (warn) Debug.warning("[StageDirector] this character's sheet has no '%(animation)' animation")
        return false
    }

    // Holds a character in its current pose, or lets it go again. A player / party member is put in scripted mode so
    // its own code doesn't reset it to idle; an NPC's wandering waits (directing).
    static pose_(entity, held) {
        var script = Script.getInstance(entity)
        var scriptable = script != null && Reflect.hasMethod(script, "setScripted(_)")
        if (held) {
            StageDirector.posed_[entity] = true
            if (scriptable) script.setScripted(true)
        } else if (StageDirector.posed_.containsKey(entity)) {
            StageDirector.posed_.remove(entity)
            if (scriptable) script.setScripted(false)
        }
    }

    static setScripted_(entity, value) {
        var script = Script.getInstance(entity)
        if (script != null && Reflect.hasMethod(script, "setScripted(_)")) {
            script.setScripted(value)
        }
    }

    // A new move or clip for an entity replaces the one it was doing.
    static cancelFor_(entity) {
        var actions = StageDirector.actions_
        var old = actions.where {|a| (a["kind"] == "move" || a["kind"] == "clip") && a["entity"] == entity }.toList
        for (a in old) {
            actions.remove(a)
        }
    }
}
