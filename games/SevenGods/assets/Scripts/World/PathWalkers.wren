// World/PathWalkers.wren
// NPCs wandering along Paths (GameObjects/Path.wren). update() is called every frame by States/GameWorldState.wren.
//
// An NPC wanders the Path its Entity_ref LDtk field points at (GameObjects/NPC.wren wanderPathIid); other Paths are
// only walked when dialogue sends someone along them. Several NPCs can share one Path.
// How it's walked is its PathFollowType: Once (through once, then stand), Loop (round and round), PingPong (there and
// back), Random (point to random point). The walker starts at whichever point is nearest, pausing a moment at each.
//
// The walker stands aside:
//   - while a conversation is open, everyone stops, and an NPC close to the player turns to face them;
//   - while a dialogue scene is moving or posing it (World/StageDirector.wren). An NPC a scene left in a pose (Finn
//     Working) stays put until a scene changes it;
//   - while it's hidden (World/StoryGates.wren).
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "math" for Vec2, Vec3
import "app" for Time
import "random" for Random
import "World/Paths" for Paths
import "World/StoryGates" for StoryGates
import "World/StageDirector" for StageDirector

// A little slower than the player's walk (StageDirector WALK_SPEED 48) - an amble.
var WANDER_SPEED = 36.0
// Seconds stood at each point, picked at random between these.
var PAUSE_MIN = 1.0
var PAUSE_MAX = 3.0
// How close (px) an NPC must be to the player to turn to them when a conversation opens.
var TALK_RADIUS = 48.0
// How often (seconds) NPCs without their Path look for it again - a level (or a story-gated NPC) can appear later.
var ASSIGN_INTERVAL = 1.0

class PathWalkers {
    // NPC entity -> walker state (a map: see assign_).
    static walkers_ {
        if (__walkers == null) __walkers = {}
        return __walkers
    }

    static random_ {
        if (__random == null) __random = Random.new()
        return __random
    }

    static update() {
        if (__nextAssign == null || Time.scaledTime >= __nextAssign) {
            __nextAssign = Time.scaledTime + ASSIGN_INTERVAL
            PathWalkers.assign_()
        }
        var talkingTo = PathWalkers.playerInConversation_()
        var dt = Time.scaledDelta
        for (npc in PathWalkers.walkers_.keys.toList) {
            var state = PathWalkers.walkers_[npc]
            if (!GameObject.isValid(npc) || !GameObject.isValid(state["path"].entity)) {
                PathWalkers.walkers_.remove(npc)
            } else {
                PathWalkers.step_(state, talkingTo, dt)
            }
        }
    }

    // Forgets every walker (the gameplay state closing).
    static clear() {
        __walkers = {}
        __nextAssign = null
    }

    // The Path (GameObjects/Path.wren) `npc` is wandering, or null.
    static pathOf(npc) {
        var state = PathWalkers.walkers_[npc]
        return state == null ? null : state["path"]
    }

    // --- internals ---------------------------------------------------------------------

    // Finds the Path of each NPC that has one but isn't walking it yet (see the header).
    static assign_() {
        var byIid = null
        for (npc in GameObject.getAllWithIdentifier("NPC")) {
            if (PathWalkers.walkers_.containsKey(npc)) continue
            var script = Script.getInstance(npc)
            if (script == null || script.wanderPathIid == null) continue
            if (byIid == null) {
                byIid = {}
                for (path in Paths.all) {
                    byIid[path.iid] = path
                }
            }
            var path = byIid[script.wanderPathIid]
            if (path == null) continue
            var points = path.points
            if (points.count == 0) continue
            PathWalkers.walkers_[npc] = {
                "npc": npc, "path": path, "index": PathWalkers.nearestIndex_(points, npc), "step": 1,
                "pauseUntil": 0, "done": false, "moving": false, "turnedToTalk": false
            }
        }
    }

    static nearestIndex_(points, npc) {
        var here = WorldTransform.getPosition(npc)
        var best = 0
        var bestDistance = null
        for (i in 0...points.count) {
            var dx = points[i].x - here.x
            var dy = points[i].y - here.y
            var distance = dx * dx + dy * dy
            if (bestDistance == null || distance < bestDistance) {
                best = i
                bestDistance = distance
            }
        }
        return best
    }

    // The player while a conversation is open (GameObjects/Player.wren interacting), else null.
    static playerInConversation_() {
        var players = GameObject.getAllWithIdentifier("Player")
        if (players.count == 0) return null
        var script = Script.getInstance(players[0])
        if (script != null && script.interacting) return players[0]
        return null
    }

    static step_(state, talkingTo, dt) {
        var npc = state["npc"]
        if (!StoryGates.isVisible(npc) || StageDirector.directing(npc)) {
            state["moving"] = false
            return
        }
        if (talkingTo != null) {
            if (state["moving"]) {
                state["moving"] = false
                StageDirector.face_(npc, StageDirector.facingOf_(npc), "Idle")
            }
            if (!state["turnedToTalk"]) {
                state["turnedToTalk"] = true
                var me = WorldTransform.getPosition(npc)
                var them = WorldTransform.getPosition(talkingTo)
                var toPlayer = Vec2.new(them.x - me.x, them.y - me.y)
                if (toPlayer.length() < TALK_RADIUS && toPlayer.length() > 0.001) {
                    StageDirector.face_(npc, toPlayer.normalize(), "Idle")
                }
            }
            return
        }
        state["turnedToTalk"] = false
        if (state["done"] || Time.scaledTime < state["pauseUntil"]) return

        var points = state["path"].points
        if (points.count == 0) return
        if (state["index"] >= points.count) state["index"] = 0
        var target = points[state["index"]]
        var here = WorldTransform.getPosition(npc)
        var delta = Vec2.new(target.x - here.x, target.y - here.y)
        var distance = delta.length()
        var stepLength = WANDER_SPEED * dt
        if (distance <= stepLength || distance < 0.5) {
            WorldTransform.setPosition(npc, Vec3.new(target.x, target.y, 0))
            StageDirector.face_(npc, StageDirector.facingOf_(npc), "Idle")
            state["moving"] = false
            state["pauseUntil"] = Time.scaledTime + PAUSE_MIN + PathWalkers.random_.float() * (PAUSE_MAX - PAUSE_MIN)
            PathWalkers.advance_(state, points.count)
            return
        }
        var dir = delta.normalize()
        WorldTransform.setPosition(npc, Vec3.new(here.x + dir.x * stepLength, here.y + dir.y * stepLength, 0))
        StageDirector.face_(npc, dir, "Walk")
        state["moving"] = true
    }

    // Picks the next point by the Path's PathFollowType.
    static advance_(state, count) {
        var type = state["path"].followType
        var index = state["index"]
        if (count < 2) {
            state["done"] = true
        } else if (type == "Loop") {
            state["index"] = (index + 1) % count
        } else if (type == "PingPong") {
            var next = index + state["step"]
            if (next < 0 || next >= count) {
                state["step"] = -state["step"]
                next = index + state["step"]
            }
            state["index"] = next
        } else if (type == "Random") {
            var next = PathWalkers.random_.int(count - 1)
            state["index"] = next >= index ? next + 1 : next
        } else {
            // Once
            if (index + 1 >= count) {
                state["done"] = true
            } else {
                state["index"] = index + 1
            }
        }
    }
}
