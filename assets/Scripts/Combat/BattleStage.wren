// Combat/BattleStage.wren
// The *physical* side of a fight - everything you can see in the world while States/CombatState.wren runs the
// battle (Combat/BattleEngine.wren) on top. The camera cuts to a separate arena, where the engine's lanes x rows
// grid is drawn as cells on the ground and a Battler (Combat/Battler.wren) stands in for each combatant.
//
// The arena is an authored "<Terrain>BattleStage" level (GameWorldState.arenaIndexFor_), loaded and activated by
// CombatState with every other loaded level deactivated for the duration, so its own tiles read as the battle's
// backdrop. Falls back to a fixed patch of world (DEFAULT_ARENA) or an authored "BattleAnchor" marker entity.
//
// The grid: rows run left to right (the party's back row on the left, the opposition's on the right), lanes top to
// bottom. Cells are tinted by what matters right now:
//   - a faint wash for each side's half;
//   - red where an opponent's move in progress will land (stronger as its impact nears) - and, for slow marked
//     hits, from the moment the chain is committed (shadow marks);
//   - during selection: the cells the highlighted move would cover from where the chain leaves you, the cells a
//     movement move may go to, and the cursor.
//
// update() reads the engine every frame: battlers follow their combatants (cell, facing, move, stagger), newcomers
// (a boss's shards) get a battler, and the engine's events turn into reactions and particles.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Camera, World, Sprite, RenderLayer
import "math" for Math, Vec2, Vec3, Vec4
import "resourceManager" for Texture
import "renderer" for FlipBit
import "app" for Time, Application
import "Util/Colors" for WHITE
import "Combat/Battler" for Battler
import "Combat/PlanGhost" for PlanGhost

// Where the fight happens when there's no arena level or authored BattleAnchor - far above the level grid.
var DEFAULT_ARENA = Vec3.new(1200, -1500, 0)

// The grid, in world pixels around the camera focus (+y is down). At CAMERA_ZOOM 3 the camera sees ~427x240 world
// px; the bar along the bottom (UI/Combat/CombatUI.wren) leaves about 156 above it, the cards stacked in the top
// corners beside the grid.
var CELL_WIDTH = 50
var CELL_HEIGHT = 34
var GRID_CENTRE_Y = -26          // the middle lane's feet, relative to the camera focus
var CAMERA_FOCUS = Vec2.new(0, 24)
var CAMERA_ZOOM = 3
var PLAN_PAUSE = 0.8             // seconds the plan ghosts hold their final cells before running again
var CAMERA_DROP = 90             // the camera eases down this far onto the formation during the intro
var SLIDE_DISTANCE = 110         // how far off its mark each battler starts the slide-in

var CELL_PARTY = Vec4.new(110, 160, 255, 150)
var CELL_ENEMY = Vec4.new(255, 140, 110, 150)
var CELL_THREAT = Vec4.new(255, 60, 50, 255)
var CELL_MARK = Vec4.new(255, 120, 60, 150)
var CELL_AIM = Vec4.new(255, 235, 120, 230)
var CELL_GOTO = Vec4.new(120, 255, 160, 210)
var CELL_HEAL = Vec4.new(140, 255, 170, 220)
var CURSOR_COLOR = Vec4.new(255, 255, 255, 255)

class BattleStage {
    // anchorPosition: a world-space Vec3 - the centre of the authored battle level, an authored "BattleAnchor"
    // marker's position, or null to use DEFAULT_ARENA. playerEntity: the real overworld player (hidden for the
    // fight) - null when there is none (the battle test). worldParent: entity to parent every spawned battle entity
    // under (the world root). engine: the battle. defFor: Fn(combatant) -> its battle def. battleLevelEntity: the
    // level entity CombatState loaded for this fight, or null when fighting in place.
    construct new(anchorPosition, playerEntity, worldParent, engine, battleLevelEntity) {
        _anchor = anchorPosition == null ? DEFAULT_ARENA : anchorPosition
        _playerEntity = playerEntity
        _worldParent = worldParent
        _battleLevelEntity = battleLevelEntity
        _engine = engine
        _focus = Vec3.new(_anchor.x + CAMERA_FOCUS.x, _anchor.y + CAMERA_FOCUS.y, 0)
        _zoom = (engine.lanes <= 3 && engine.rows <= 4) ? CAMERA_ZOOM : 2

        // Hide the overworld player. Its Camera drops out of the active set with it, leaving _camEntity below as
        // the only camera.
        if (_playerEntity != null) {
            GameObject.setInactive(_playerEntity)
        }
        // The arena level stands in for the backdrop, so every other loaded level is deactivated for the duration.
        if (_battleLevelEntity != null) {
            setOtherLevelsActive_(false)
        }

        buildGrid_()
        _battlers = []
        for (fighter in engine.fighters) {
            addBattler_(fighter)
        }
        _highlights = []
        _cursor = null
        _ghosts = []          // [combatant, its PlanGhost] while plans are shown (drawPlans)
        _planKey = null       // what the plans shown were, to restart their clock when they change
        _planStart = 0

        // Battle camera - its own entity. Starts a little high and eases down onto the formation.
        _camStart = Vec3.new(_focus.x, _focus.y - CAMERA_DROP, 0)
        _camEntity = GameObject.create("BattleCamera", worldParent)
        WorldTransform.setPosition(_camEntity, _camStart)
        _camera = Camera.create(_camEntity)
        _camera.zoom = _zoom

        intro(0)
    }

    engine { _engine }
    zoom { _zoom }

    // --- the grid --------------------------------------------------------------------------

    // The world position of a cell's centre - where a battler's feet go.
    cellWorld(lane, row) {
        var x = _focus.x + (row - (_engine.rows - 1) / 2) * CELL_WIDTH
        var y = _focus.y + GRID_CENTRE_Y + (lane - (_engine.lanes - 1) / 2) * CELL_HEIGHT
        return Vec3.new(x, y, 0)
    }

    // The same in screen pixels (the battle camera is fixed once the entrance is over) - for the UI's floating text.
    cellScreen(lane, row) {
        var world = cellWorld(lane, row)
        return Vec2.new(Application.gameWidth / 2 + (world.x - _focus.x) * _zoom,
                        Application.gameHeight / 2 + (world.y - _focus.y) * _zoom)
    }

    buildGrid_() {
        _cells = []
        var texture = Texture.load("UI/Battle/cell.png")
        for (lane in 0..._engine.lanes) {
            var line = []
            for (row in 0..._engine.rows) {
                var entity = GameObject.create("BattleCell", _worldParent)
                Sprite.create(entity, texture, WHITE, Vec2.new(23, 18), 1, 1, FlipBit.NONE, 0, RenderLayer.ENTITIES, -100)
                WorldTransform.setPosition(entity, cellWorld(lane, row))
                line.add(entity)
            }
            _cells.add(line)
        }
        texture.unload()
        var cursorTexture = Texture.load("UI/Battle/cursor.png")
        _cursorEntity = GameObject.create("BattleCursor", _worldParent)
        Sprite.create(_cursorEntity, cursorTexture, CURSOR_COLOR, Vec2.new(23, 18), 1, 1, FlipBit.NONE, 0, RenderLayer.ENTITIES, -90)
        cursorTexture.unload()
        hideCursor_()
    }

    hideCursor_() {
        WorldTransform.setPosition(_cursorEntity, Vec3.new(_focus.x, _focus.y - 4000, 0))
    }

    // Selection overlays. cells: [[lane, row, kind], ...] with kind "aim" (a move's cells), "goto" (a legal
    // destination) or "heal". cursor: [lane, row] or null.
    setHighlights(cells, cursor) {
        _highlights = cells == null ? [] : cells
        _cursor = cursor
    }
    clearHighlights() { setHighlights(null, null) }

    // Recolours every cell: the half it belongs to, then threats, then the selection overlays.
    paintGrid_() {
        var half = (_engine.rows / 2).floor
        var threat = {}   // "lane,row" -> 0..1
        var marks = {}
        for (fighter in _engine.fighters) {
            if (!fighter.active || fighter.side == 0) continue
            var move = fighter.move
            if (move != null && move.isAttack && !fighter.impactDone && move.impact != null) {
                var closeness = Math.clamp01(1 - (move.impact - fighter.progress) / 2)
                for (cell in fighter.cells) {
                    var key = cell[0] * 100 + cell[1]
                    var level = 0.35 + 0.65 * closeness
                    if (!threat.containsKey(key) || threat[key] < level) threat[key] = level
                }
            }
            // Shadow marks: a slow marked hit later in the chain shades its cells from the commit - while nothing
            // before it moves the attacker (after a move the cells are only known once it lands).
            var index = fighter.chainIndex + 1
            var chain = fighter.chain
            var moving = move != null && move.isMovement && !fighter.moved
            while (!moving && index < chain.count) {
                var next = chain[index].move
                if (next.isMovement) break
                if (next.heavy && next.isAttack) {
                    for (cell in _engine.cellsAt(fighter, next, fighter.lane, fighter.row, fighter.facing)) {
                        marks[cell[0] * 100 + cell[1]] = true
                    }
                }
                index = index + 1
            }
        }
        var overlay = {}
        for (highlight in _highlights) {
            overlay[highlight[0] * 100 + highlight[1]] = highlight[2]
        }
        var pulse = 0.75 + 0.25 * Math.sin(Time.unscaledTime * 7)
        for (lane in 0..._engine.lanes) {
            for (row in 0..._engine.rows) {
                var key = lane * 100 + row
                var color = row < half ? CELL_PARTY : CELL_ENEMY
                if (marks.containsKey(key)) color = CELL_MARK
                if (threat.containsKey(key)) {
                    color = Vec4.new(CELL_THREAT.x, CELL_THREAT.y, CELL_THREAT.z, 90 + 165 * threat[key])
                }
                if (overlay.containsKey(key)) {
                    var kind = overlay[key]
                    var base = kind == "goto" ? CELL_GOTO : (kind == "heal" ? CELL_HEAL : CELL_AIM)
                    color = Vec4.new(base.x, base.y, base.z, base.w * pulse)
                }
                var sprite = Sprite.get(_cells[lane][row])
                if (sprite != null) sprite.color = color
            }
        }
        if (_cursor != null) {
            var at = cellWorld(_cursor[0], _cursor[1])
            WorldTransform.setPosition(_cursorEntity, at)
        } else {
            hideCursor_()
        }
    }

    // --- battlers --------------------------------------------------------------------------

    addBattler_(fighter) {
        var battler = Battler.new(fighter.def, fighter, _worldParent, cellWorld(fighter.lane, fighter.row))
        battler.face(fighter.facing)
        _battlers.add(battler)
        return battler
    }

    battlerFor(combatant) {
        for (battler in _battlers) {
            if (battler.combatant == combatant) return battler
        }
        return null
    }

    // The board changed wholesale (the next stage of an encounter): battlers for combatants no longer on it go, the
    // newcomers get one, and everybody stands on their cell.
    resync() {
        var kept = []
        for (battler in _battlers) {
            if (_engine.fighters.contains(battler.combatant)) {
                kept.add(battler)
            } else {
                battler.teardown()
            }
        }
        _battlers = kept
        for (fighter in _engine.fighters) {
            var battler = battlerFor(fighter)
            if (battler == null) battler = addBattler_(fighter)
            battler.place(cellWorld(fighter.lane, fighter.row))
            battler.order = fighter.lane * 10
        }
    }

    // Where a combatant is on screen right now (its feet) - for floating text.
    screenOf(combatant) {
        var battler = battlerFor(combatant)
        if (battler == null) return cellScreen(combatant.lane, combatant.row)
        var position = battler.position
        return Vec2.new(Application.gameWidth / 2 + (position.x - _focus.x) * _zoom,
                        Application.gameHeight / 2 + (position.y - _focus.y) * _zoom)
    }

    // --- the entrance ------------------------------------------------------------------------

    // t: 0..1 progress through the entrance. Eases the camera down onto the formation and slides every battler in
    // from its wing.
    intro(t) {
        var eased = Math.smoothstep(t)
        WorldTransform.setPosition(_camEntity, Vec3.lerp(_camStart, _focus, eased))
        _camera.forcePosition = true
        for (battler in _battlers) {
            var combatant = battler.combatant
            var home = cellWorld(combatant.lane, combatant.row)
            var off = combatant.side == 0 ? -SLIDE_DISTANCE : SLIDE_DISTANCE
            battler.place(Vec3.lerp(Vec3.new(home.x + off, home.y, 0), home, eased))
            battler.order = combatant.lane * 10
        }
        paintGrid_()
    }

    playEntrance() {
        for (battler in _battlers) {
            battler.playEntrance()
        }
    }

    settle() {
        intro(1)
        for (battler in _battlers) {
            battler.playIdle()
        }
    }

    // --- every frame -------------------------------------------------------------------------

    // dt: game seconds. events: what the engine reported since the last frame (BattleEngine.takeEvents).
    update(dt, events) {
        for (event in events) {
            react_(event)
        }
        for (battler in _battlers) {
            var combatant = battler.combatant
            if (combatant.active) {
                battler.target = cellWorld(combatant.lane, combatant.row)
                battler.order = combatant.lane * 10
            }
            battler.update(dt)
        }
        removeGone_()
        paintGrid_()
    }

    // Battlers that have finished fading out leave the stage.
    removeGone_() {
        var i = _battlers.count - 1
        while (i >= 0) {
            if (_battlers[i].gone) {
                _battlers[i].teardown()
                _battlers.removeAt(i)
            }
            i = i - 1
        }
    }

    // Keeps the grid overlays (and the selection cursor) fresh while game time is paused for a menu.
    refresh() { paintGrid_() }

    // While the fight is frozen for move picking: the defeated finish fading out (and leave the stage) and a blow's
    // flash and bump finish - nobody else moves (Battler.settle).
    settleFrozen() {
        for (battler in _battlers) {
            if (battler.settling) battler.settle()
        }
        removeGone_()
    }

    react_(event) {
        var type = event["type"]
        var who = event["who"]
        if (type == "spawn") {
            addBattler_(who).playImpact("crushShock")
            return
        }
        var battler = battlerFor(who)
        if (battler == null) return
        if (type == "hit") {
            battler.playHitCloud(event["element"], event["blocked"], event["amount"])
            if (!event["blocked"] && event["amount"] > 0) battler.hurt(battlerFor(event["by"]))
            var move = event["move"]
            if (event["blocked"]) {
                battler.playImpact("parrySpark")
            } else if (move != null && move.scaling == "mana") {
                battler.playImpact("boltSpark")
            } else {
                battler.playImpact(event["amount"] >= 30 ? "cleaveBurst" : "slashSpark")
            }
        } else if (type == "dodge") {
            battler.hop()
        } else if (type == "heal") {
            battler.playImpact("healGlow")
        } else if (type == "defeat" || type == "remove") {
            battler.defeat()
        } else if (type == "direct") {
            battler.playImpact("boltSpark")
        }
    }

    // t: 0..1 progress of the party running off-stage (the flee animation).
    runOff(t) {
        for (battler in _battlers) {
            var combatant = battler.combatant
            if (combatant.side != 0 || !combatant.active) continue
            var home = cellWorld(combatant.lane, combatant.row)
            battler.face(-1)
            battler.playEntrance()
            battler.place(Vec3.new(home.x - 220 * Math.smoothstep(t), home.y, 0))
        }
    }

    // --- plan ghosts ---------------------------------------------------------------------------

    // While moves are picked: a see-through ghost for each party member with a plan plays it through - and, with Soul
    // Sight, one for each opponent that plays only as far as Soul Sight sees and then holds. All on one clock that
    // loops - as long as the longest ghost sequence, then a short pause on the final positions - and starts over
    // whenever a plan changes.
    // plans: [[combatant, steps, seconds], ...] - steps as ChainBuilder.steps / BattlePlan.stepsOf give them; seconds:
    // how far into them the ghost may play (null for all of it).
    drawPlans(plans) {
        var key = ""
        var length = 0
        var lengths = []
        for (plan in plans) {
            key = key + plan[0].name + ":"
            var planLength = 0
            for (step in plan[1]) {
                key = key + step["move"].name + ","
                planLength = planLength.max(step["end"])
            }
            if (plan[2] != null) planLength = planLength.min(plan[2])
            lengths.add(planLength)
            length = length.max(planLength)
            key = key + ";"
        }
        if (key != _planKey) {
            _planKey = key
            _planStart = Time.unscaledTime
        }
        var clock = (Time.unscaledTime - _planStart) % (length + PLAN_PAUSE)
        var kept = []
        for (i in 0...plans.count) {
            var plan = plans[i]
            var fighter = plan[0]
            var battler = battlerFor(fighter)
            if (battler == null || !fighter.active) continue
            var ghost = ghostFor_(fighter)
            if (ghost == null) ghost = PlanGhost.new(battler.def, battler.wearsWeapon, _worldParent, fighter.side != 0)
            // Each ghost plays its own sequence, then holds on its last pose until the loop comes round.
            ghost.show(clock.min(lengths[i]), plan[1], fighter, this)
            kept.add([fighter, ghost])
        }
        for (pair in _ghosts) {
            var stillShown = false
            for (keep in kept) {
                if (keep[1] == pair[1]) stillShown = true
            }
            if (!stillShown) pair[1].teardown()
        }
        _ghosts = kept
    }

    ghostFor_(fighter) {
        for (pair in _ghosts) {
            if (pair[0] == fighter) return pair[1]
        }
        return null
    }

    hidePlans() {
        if (_ghosts.count == 0) return
        for (pair in _ghosts) pair[1].teardown()
        _ghosts = []
        _planKey = null
    }

    // Fold the arena away: destroy every battler, the grid and the camera, bring the real player and every other
    // level back.
    teardown() {
        hidePlans()
        for (battler in _battlers) {
            battler.teardown()
        }
        _battlers = []
        for (line in _cells) {
            for (entity in line) {
                if (GameObject.isValid(entity)) GameObject.destroy(entity)
            }
        }
        _cells = []
        if (GameObject.isValid(_cursorEntity)) GameObject.destroy(_cursorEntity)
        if (GameObject.isValid(_camEntity)) {
            GameObject.destroy(_camEntity)
        }
        if (_playerEntity != null) {
            GameObject.setActive(_playerEntity)
            // forcePosition is a one-shot flag - this just snaps the return to the player's own camera instead of
            // letting the damped follow lerp it back in from the arena.
            var playerCamera = Camera.get(_playerEntity)
            if (playerCamera != null) {
                playerCamera.forcePosition = true
            }
        }
        if (_battleLevelEntity != null) {
            setOtherLevelsActive_(true)
            // Deactivate (not unload) the battle level itself so the next fight in the same arena doesn't pay a
            // reload cost.
            if (GameObject.isValid(_battleLevelEntity)) {
                GameObject.setInactive(_battleLevelEntity)
            }
        }
    }

    // Sets every *other* currently-loaded level (i.e. not _battleLevelEntity) active/inactive - used to hide the
    // overworld behind the battle arena and bring it back afterwards.
    setOtherLevelsActive_(active) {
        for (i in 0...World.getLevelsCount(_worldParent)) {
            var levelEntity = World.getLoadedLevelEntity(_worldParent, i)
            if (levelEntity == null || levelEntity == _battleLevelEntity) {
                continue
            }
            if (active) {
                GameObject.setActive(levelEntity)
            } else {
                GameObject.setInactive(levelEntity)
            }
        }
    }
}
