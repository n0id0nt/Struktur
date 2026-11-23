// Combat/Config/BattleCritter.wren
// The battle-side definition of one fighter that isn't the player (Combat/Config/BattlePlayer.wren has the same
// shape for them): its stats, what it carries (Combat/Gear.wren) or its natural weapon, the moves it knows, the
// combo menu its AI picks chains from (Combat/EnemyAI.wren) and how it looks in the arena. Combat/Battler.wren
// turns one into a live entity; makeCombatant mints the Combatant that fights.
//
// Humanoids use real gear and the human moves directly. Monsters use reskins - a human move renamed and re-animated
// to use a body part (Stab -> Bite, Lunge -> Pounce, Roll -> Leap Back): the reskin keeps the base move's types,
// times, windows, stencil and cost; the monster's natural weapon [power, stagger multiplier] replaces weapon power.
// A themed Hold ("Circle", "Grind") is just a Hold under the monster's own name - its tell.
//
// Stats follow the tiers of the battle system doc (early: 100 HP, 10 Attack / Mana, 5 Defence, 80 Stamina, 12
// recovery), so an early enemy is a fair fight for an early party.
import "math" for Vec2, Vec4
import "Combat/Combatant" for Combatant
import "Combat/Stats" for Stats
import "Combat/Move" for Move
import "Combat/Gear" for GearItem, Loadout
import "Combat/EnemyAI" for ComboMenu, ComboBrain, PlannerBrain
import "Combat/Config/Moves" for Moves
import "Combat/Config/SheetAnims" for SheetAnims
import "GameObjects/NPC" for NPCData
import "Sprites/GoblinVikingBattleSprite" for GoblinVikingBattleSprite
import "Sprites/WrestlerBattleSprite" for WrestlerBattleSprite
import "Sprites/YellowPantsGirlBattleWeaponSprite" for YellowPantsGirlBattleWeaponSprite

class BattleCritter {
    // anims: { role: [startFrame, endFrameExclusive, seconds] }. facing: 1 = art faces right, -1 = left.
    construct new(name, texturePath, cols, rows, pivot, facing, anims) {
        _name = name
        _texturePath = texturePath
        _cols = cols
        _rows = rows
        _pivot = pivot
        _facing = facing
        _anims = anims
        _animsLeft = null
        _weaponTexturePath = null
        _numbers = [100, 80, 10, 10, 5, 12]
        _gear = [null, null, null]
        _kit = []
        _menu = null
        _traits = {}
        _tint = null
        _read = 0
        _brainFn = null
    }

    // --- builders (each returns this) --------------------------------------------------

    // Gives the def left-facing art: the same roles as `anims`, played (instead of flipping the sheet) while the
    // battler faces left.
    withLeftAnims(animsLeft) {
        _animsLeft = animsLeft
        return this
    }
    // Draws `sheet`'s weapon layer over the body (generated with the body sheet, on the same grid).
    weaponLayer(sheet) {
        _weaponTexturePath = sheet.data["texture"]
        return this
    }
    stats(maxHp, maxStamina, attack, mana, defence, recovery) {
        _numbers = [maxHp, maxStamina, attack, mana, defence, recovery]
        return this
    }
    // Weapon / armour type ids (Combat/Config/GearTypes.wren), early quality, or null for nothing.
    gear(main, off, armour) {
        _gear = [main, off, armour]
        return this
    }
    // The moves it knows, by catalog name ("Slash", "Slash II").
    kit(names) {
        _kit = names
        return this
    }
    // A Fn returning a fresh ComboMenu - what its AI picks chains from.
    menu(fn) {
        _menu = fn
        return this
    }
    // A Fn returning a custom brain (a boss) instead of a menu or the planner.
    brain(fn) {
        _brainFn = fn
        return this
    }
    // A natural weapon: every move uses [power, stagger multiplier] in place of a held weapon.
    natural(power, staggerMul) {
        _traits["natural"] = [power, staggerMul]
        return this
    }
    trait(key, value) {
        _traits[key] = value
        return this
    }
    tint(color) {
        _tint = color
        return this
    }
    read(level) {
        _read = level
        return this
    }

    name { _name }
    texturePath { _texturePath }
    cols { _cols }
    rows { _rows }
    pivot { _pivot }
    facing { _facing }
    anims { _anims }
    animsLeft { _animsLeft }   // null = no left-facing art; Battler flips `anims` instead
    // The weapon layer drawn over the sheet, frame for frame (a Sprites/<Name>BattleWeaponSprite) - null for none.
    weaponTexturePath { _weaponTexturePath }
    tint { _tint }
    maxHp { _numbers[0] }
    maxStamina { _numbers[1] }
    hasMenu { _menu != null || _brainFn != null }
    passive { _traits.containsKey("passive") }

    kit { _kit.map { |name| Moves.byName(name) }.where { |move| move != null }.toList }
    kitNames { _kit }

    loadout {
        var main = _gear[0] == null ? null : GearItem.weapon(_gear[0])
        var off = _gear[1] == null ? null : GearItem.weapon(_gear[1])
        var armour = _gear[2] == null ? null : GearItem.armour(_gear[2])
        return Loadout.new(main, off, armour)
    }

    newStats { Stats.new(_numbers[0], _numbers[1], _numbers[2], _numbers[3], _numbers[4], _numbers[5]) }

    // A fresh Combatant for one instance of this fighter. `entity` is the overworld entity it came from (kept so a
    // defeated critter can be removed from the field), or null.
    makeCombatant(entity) { makeCombatant(entity, _name, null, null) }

    // The same with its own name, loadout and moves (the battle test's pickers) - null keeps the def's.
    makeCombatant(entity, name, loadout, moves) {
        var combatant = Combatant.new(name, newStats, loadout == null ? this.loadout : loadout,
            moves == null ? kit : moves, entity)
        combatant.def = this
        combatant.read = _read
        for (key in _traits.keys) {
            combatant.traits[key] = _traits[key]
        }
        return combatant
    }

    // The AI for a combatant minted from this def fighting on the other side: its own brain, its combo menu, or -
    // with neither - the planner. null for a passive prop.
    makeBrain() {
        if (passive) return null
        if (_brainFn != null) return _brainFn.call()
        if (_menu != null) return ComboBrain.new(_menu.call())
        return PlannerBrain.new()
    }

    // --- lookup ----------------------------------------------------------------------------

    // The def for an overworld critter species, an NPC or a prop, by name. Unknown names fight as a Chicken.
    static forName(name) {
        if (name == "Chinlin") return BattleCritter.chinlin
        if (name == "Villager") return BattleCritter.villager
        if (name == "Duelist") return BattleCritter.duelist
        if (name == "Kira") return BattleCritter.kira
        if (name == "Thomas") return BattleCritter.thomas
        if (name == "Finn" || name == "Master Finn") return BattleCritter.finn
        if (name == "Runic Construct") return BattleCritter.runicConstruct(7)
        // The shrine chamber's three constructs (LDtk NPC names) - a little tougher each time.
        if (name == "RunicConstuct1") return BattleCritter.runicConstruct(5)
        if (name == "RunicConstruct2") return BattleCritter.runicConstruct(6)
        if (name == "RunicConstruct3") return BattleCritter.runicConstruct(7)
        if (name == "Starving Thornwolf") return BattleCritter.starvingThornwolf
        if (name == "Corrupted Thornwolf" || name == "Thornwolf") return BattleCritter.thornwolf
        if (name == "Twisted Sprite") return BattleCritter.twistedSprite
        if (name == "Stone Guardian") return BattleCritter.stoneGuardian
        if (name == "Desperate Raider") return BattleCritter.raider
        if (name == "Practice Stump") return BattleCritter.practiceStump
        if (name == "Swing-post") return BattleCritter.swingPost
        if (name == "Snap-post") return BattleCritter.snapPost
        if (name == "Training Dummy") return BattleCritter.trainingDummy
        return BattleCritter.chicken
    }

    // A Fn that mints `base` at `tier` under a monster's own name.
    static reskin(base, tier, name) {
        return Fn.new { Moves.get(base, tier).rename(name) }
    }

    // --- critters (the overworld's Chicken and Chinlin - Levels/SevenGods.ldtk) ----------------------

    // Actors/chicken.png - 7x6 grid, 16px cells, pivot (8,8). Twitchy but feeble: pecks, then flutters off.
    static chicken {
        var anims = {"idle": [6, 9, 0.5], "attack": [32, 39, 1.0], "hurt": [15, 18, 0.35], "run": [15, 18, 0.35]}
        var def = BattleCritter.new("Chicken", "Actors/chicken.png", 7, 6, Vec2.new(8, 8), 1, anims)
        return def.stats(2, 60, 0, 0, 0, 12).natural(0, 0.6).menu(Fn.new {
            var moves = {}
            moves["Hop"] = BattleCritter.reskin("Step", 1, "Hop")
            moves["Peck"] = BattleCritter.reskin("Stab", 1, "Peck")
            moves["Flutter"] = BattleCritter.reskin("Roll", 1, "Flutter")
            var combos = {}
            combos["Fuss"] = [["Hop", "toward"], ["Cluck", 0.8], ["Hop", "toward"], ["Peck"], ["Peck"], ["Flutter", "away"]]
            combos["Flap"] = [["Peck"], ["Peck"], ["Cluck", 1.2], ["Flutter", "away"], ["Hop", "toward"]]
            return ComboMenu.new(moves, combos, "Fuss").followUp("Fuss", "Flap", 0.6).weights("healthy", {"Fuss": 60, "Flap": 40})
        })
    }

    // Actors/chinlin.png - 4x4 grid, 24px cells, pivot (12,16). A charging bruiser: one slow marked stomp, a gore.
    static chinlin {
        var anims = {"idle": [4, 6, 0.8], "attack": [12, 15, 0.45], "hurt": [12, 15, 0.45], "run": [12, 15, 0.45]}
        var def = BattleCritter.new("Chinlin", "Actors/chinlin.png", 4, 4, Vec2.new(12, 16), 1, anims)
        return def.stats(6, 80, 0, 0, 0, 12).natural(0, 1.1).trait("tax", 1).menu(Fn.new {
            var moves = {}
            moves["Trot"] = BattleCritter.reskin("Step", 1, "Trot")
            moves["Gore"] = BattleCritter.reskin("Lunge", 1, "Gore")
            moves["Butt"] = BattleCritter.reskin("Shove", 1, "Butt")
            moves["Stomp"] = Fn.new { Moves.get("Overhead Cleave", 1).rename("Stomp").heavy() }
            var combos = {}
            combos["Charge"] = [["Trot", "toward"], ["Paw", 0.8], ["Gore"], ["Butt"], ["Snort", 1.2]]
            combos["Trample"] = [["Stomp"], ["Snort", 0.8], ["Trot", "toward"], ["Butt"], ["Trot", "toward"]]
            var menu = ComboMenu.new(moves, combos, "Charge").followUp("Charge", "Trample", 0.6).followUp("Trample", "Charge", 0.5)
            return menu.weights("healthy", {"Charge": 50, "Trample": 50}).weights("wounded", {"Charge": 30, "Trample": 70})
        })
    }

    // --- early enemies (the enemy types and combo patterns doc) --------------------------------------

    // The first arc's tutorial wolf: 3 HP (three Slashes) and two combos. It closes in one cell at a time (Stalk); whenever someone
    // is in reach as it starts a chain it bites straight away, circles, then leaps back (Snap) - brace, then punish.
    static starvingThornwolf {
        var def = BattleCritter.wolfArt_("Starving Thornwolf")
        return def.stats(3, 60, 0, 0, 0, 12).natural(0, 0.7).tint(Vec4.new(205, 200, 190, 255)).menu(Fn.new {
            var moves = BattleCritter.wolfMoves_()
            var combos = {}
            combos["Stalk"] = [["Lope", "toward"], ["Circle", 1.2], ["Lope", "toward"], ["Circle", 1.2], ["Lope", "toward"]]
            combos["Snap"] = [["Bite"], ["Circle", 1.2], ["Leap Back", "away"], ["Circle", 1.2], ["Lope", "toward"]]
            return ComboMenu.new(moves, combos, "Stalk").trigger("inReach", "Snap", 1).trigger("notInReach", "Stalk", 1)
        })
    }

    // Corrupted Thornwolf: opens with Stalk (a Lope and a growling Circle, then a Pounce, two Bites and a Leap
    // Back), and a Bite-heavy Worry almost always follows.
    static thornwolf {
        var def = BattleCritter.wolfArt_("Corrupted Thornwolf")
        return def.stats(5, 70, 0, 0, 0, 12).natural(0, 0.8).tint(Vec4.new(190, 160, 215, 255)).menu(Fn.new {
            var moves = BattleCritter.wolfMoves_()
            var combos = {}
            combos["Stalk"] = [["Lope", "toward"], ["Circle", 0.6], ["Pounce"], ["Bite"], ["Bite"], ["Leap Back", "away"]]
            combos["Worry"] = [["Bite"], ["Bite"], ["Circle", 0.8], ["Bite"], ["Bite"], ["Leap Back", "away"], ["Lope", "toward"]]
            combos["Hit and Run"] = [["Pounce"], ["Leap Back", "away"], ["Circle", 1.2], ["Lope", "toward"], ["Bite"]]
            var menu = ComboMenu.new(moves, combos, "Stalk").followUp("Stalk", "Worry", 0.7).followUp("Worry", "Hit and Run", 0.5)
            menu.followUp("Hit and Run", "Worry", 0.6)
            return menu.weights("healthy", {"Worry": 50, "Hit and Run": 50}).weights("wounded", {"Worry": 30, "Hit and Run": 70})
        })
    }

    static wolfMoves_() {
        var moves = {}
        moves["Lope"] = BattleCritter.reskin("Step", 1, "Lope")
        moves["Bite"] = BattleCritter.reskin("Stab", 1, "Bite")
        moves["Pounce"] = BattleCritter.reskin("Lunge", 1, "Pounce")
        moves["Leap Back"] = BattleCritter.reskin("Roll", 1, "Leap Back")
        return moves
    }

    // Placeholder look: the Chinlin's sheet until the wolves get their own.
    static wolfArt_(name) {
        var anims = {"idle": [4, 6, 0.8], "attack": [12, 15, 0.45], "hurt": [12, 15, 0.45], "run": [12, 15, 0.45]}
        return BattleCritter.new(name, "Actors/chinlin.png", 4, 4, Vec2.new(12, 16), 1, anims)
    }

    // Twisted Sprite: an evasive flyer that pesters from range and flits aside. Scales with Mana.
    static twistedSprite {
        var anims = {"idle": [6, 9, 0.5], "attack": [32, 39, 1.0], "hurt": [15, 18, 0.35], "run": [15, 18, 0.35]}
        var def = BattleCritter.new("Twisted Sprite", "Actors/chicken.png", 7, 6, Vec2.new(8, 8), 1, anims)
        def.stats(3, 70, 0, 0, 0, 14).natural(0, 0.6).tint(Vec4.new(150, 235, 215, 255))
        return def.menu(Fn.new {
            var moves = {}
            moves["Pixie Bolt"] = BattleCritter.reskin("Spark", 1, "Pixie Bolt")
            moves["Flit"] = BattleCritter.reskin("Roll", 1, "Flit")
            var combos = {}
            combos["Pester"] = [["Pixie Bolt"], ["Pixie Bolt"], ["Flit", "side"], ["Hover", 1.2], ["Pixie Bolt"]]
            combos["Tease"] = [["Flit", "side"], ["Pixie Bolt"], ["Hover", 0.8], ["Flit", "away"], ["Pixie Bolt"], ["Pixie Bolt"]]
            var menu = ComboMenu.new(moves, combos, "Pester").followUp("Pester", "Tease", 0.6).followUp("Tease", "Pester", 0.5)
            return menu.weights("healthy", {"Pester": 50, "Tease": 50})
        })
    }

    // Runic Construct: an ancient shrine guardian - slow and heavy, with long marked wind-ups. Brace or step out of
    // the big hit, then punish the recovery.
    static runicConstruct(maxHp) {
        var def = BattleCritter.fromSheet("Runic Construct", GoblinVikingBattleSprite)
        return def.stats(maxHp, 90, 0, 0, 0, 12).natural(0, 1.2).trait("tax", 1).tint(Vec4.new(170, 200, 230, 255)).menu(Fn.new {
            var moves = {}
            moves["Trudge"] = BattleCritter.reskin("Step", 1, "Trudge")
            moves["Rune Slam"] = Fn.new { Moves.get("Overhead Cleave", 1).rename("Rune Slam").heavy() }
            moves["Stone Sweep"] = Fn.new { Moves.get("Cross Cut", 1).rename("Stone Sweep").heavy() }
            moves["Ward Stance"] = BattleCritter.reskin("Brace", 1, "Ward Stance")
            var combos = {}
            combos["Wake"] = [["Hum", 1.2], ["Trudge", "toward"], ["Rune Slam"], ["Settle", 0.8]]
            combos["Hammer"] = [["Trudge", "toward"], ["Rune Slam"], ["Settle", 1.2], ["Stone Sweep"]]
            combos["Guarded"] = [["Ward Stance"], ["Stone Sweep"], ["Settle", 1.2], ["Trudge", "toward"]]
            var menu = ComboMenu.new(moves, combos, "Wake").followUp("Wake", "Hammer", 0.6).followUp("Hammer", "Guarded", 0.5)
            return menu.weights("healthy", {"Hammer": 50, "Guarded": 50}).weights("wounded", {"Hammer": 65, "Guarded": 35})
        })
    }

    // Stone Guardian (early elite): a long-window guard and one slow telegraphed slam.
    static stoneGuardian {
        var def = BattleCritter.fromSheet("Stone Guardian", GoblinVikingBattleSprite)
        return def.stats(10, 100, 0, 0, 0, 12).natural(1, 1.2).trait("tax", 2).tint(Vec4.new(190, 190, 180, 255)).menu(Fn.new {
            var moves = {}
            moves["Rumble Step"] = BattleCritter.reskin("Step", 1, "Rumble Step")
            moves["Stone Slam"] = Fn.new { Moves.get("Overhead Cleave", 1).rename("Stone Slam").heavy() }
            moves["Tremor"] = Fn.new { Moves.get("Cross Cut", 1).rename("Tremor").heavy() }
            moves["Stone Guard"] = BattleCritter.reskin("Hanging Guard", 1, "Stone Guard")
            var combos = {}
            combos["Awaken"] = [["Grind", 1.2], ["Rumble Step", "toward"], ["Stone Slam"], ["Grind", 0.8]]
            combos["Bulwark"] = [["Stone Guard"], ["Stone Slam"], ["Grind", 1.2]]
            combos["Quake"] = [["Rumble Step", "toward"], ["Tremor"], ["Grind", 0.8], ["Stone Guard"]]
            var menu = ComboMenu.new(moves, combos, "Awaken").followUp("Awaken", "Bulwark", 0.6).followUp("Bulwark", "Quake", 0.6)
            return menu.weights("healthy", {"Bulwark": 50, "Quake": 50}).weights("wounded", {"Bulwark": 35, "Quake": 65})
        })
    }

    // Desperate Raider (human): rushes in - a Step, then a Lunge.
    static raider {
        var def = BattleCritter.fromSheet("Desperate Raider", WrestlerBattleSprite)
        return def.stats(6, 80, 0, 0, 0, 12).gear("sabre", null, "padded").kit(["Step", "Lunge", "Slash", "Stab"]).menu(Fn.new {
            var moves = {}
            moves["Step"] = Fn.new { Moves.get("Step", 1) }
            moves["Lunge"] = Fn.new { Moves.get("Lunge", 1) }
            moves["Slash"] = Fn.new { Moves.get("Slash", 1) }
            moves["Stab"] = Fn.new { Moves.get("Stab", 1) }
            var combos = {}
            combos["Rush"] = [["Step", "toward"], ["Lunge"], ["Slash"], ["Pant", 1.2], ["Stab"]]
            combos["Flail"] = [["Slash"], ["Stab"], ["Stab"], ["Pant", 1.2], ["Slash"]]
            combos["Back Off"] = [["Step", "away"], ["Pant", 2.0], ["Step", "toward"], ["Slash"]]
            var menu = ComboMenu.new(moves, combos, "Rush").followUp("Rush", "Flail", 0.6).followUp("Flail", "Back Off", 0.5)
            return menu.weights("healthy", {"Rush": 50, "Flail": 50}).weights("wounded", {"Rush": 30, "Flail": 30, "Back Off": 40})
        })
    }

    // --- NPC fighters ---------------------------------------------------------------
    // They wear their battle sheet (GameObjects/NPC.wren's NPCData.battleSprite) with real gear and human moves. In
    // the party the human picks their chains; on the other side the planner does.

    // Kira - the hunter's daughter and first companion: a short bow from the back row.
    static kira {
        return BattleCritter.fromNpc("Kira").weaponLayer(YellowPantsGirlBattleWeaponSprite).stats(4, 80, 0, 0, 0, 12).gear("shortbow", null, "clothes").kit(["Step", "Quick Shot", "Brace"])
    }

    // Thomas - Kira's father, who teaches the three lessons. A plain sword, swung at the right time.
    static thomas {
        return BattleCritter.fromNpc("Thomas").stats(8, 80, 0, 0, 0, 12).gear("armingsword", null, "padded").kit(["Step", "Slash", "Brace", "Pommel Strike"])
    }

    // Master Finn, sparring: exactly the player's kit, holding back - big slow openings between his swings, and his
    // Slash pulled (a quarter of a real one's base damage, no Attack behind it), so you can see him choosing not to
    // hurt you (First Arc Script, scene 11).
    static finn {
        // Mid-fight, barely looking, he catches Kira's first arrow with a pair of tongs.
        var def = BattleCritter.fromNpc("Finn").stats(10, 110, 0, 0, 0, 16).gear("armingsword", null, "leather").trait("catches", "Kira")
        return def.kit(["Step", "Slash", "Brace"]).menu(Fn.new {
            var moves = {}
            moves["Step"] = Fn.new { Moves.get("Step", 1) }
            moves["Slash"] = Fn.new { Moves.get("Slash", 1).attackFlat(0.5, 1, 0.3) }
            moves["Brace"] = Fn.new { Moves.get("Brace", 1) }
            var combos = {}
            combos["Measure"] = [["Step", "toward"], ["Wait", 1.2], ["Slash"], ["Brace"], ["Wait", 0.8]]
            combos["Lesson"] = [["Slash"], ["Wait", 1.2], ["Slash"], ["Brace"]]
            combos["Invite"] = [["Brace"], ["Wait", 2.0], ["Step", "toward"], ["Slash"]]
            var menu = ComboMenu.new(moves, combos, "Measure").followUp("Measure", "Lesson", 0.6).followUp("Lesson", "Invite", 0.5)
            return menu.weights("healthy", {"Lesson": 50, "Invite": 50})
        })
    }

    // The Villager - a support caster who joins the party after the locket quest.
    static villager {
        return BattleCritter.fromNpc("Villager").stats(5, 80, 0, 0, 0, 12).gear("wand", null, "robes").kit(["Step", "Spark", "Mend", "Ward"])
    }

    // The Duelist - a trained fencer you must beat to recruit: parry and riposte into a lunge.
    static duelist {
        var def = BattleCritter.fromNpc("Duelist").stats(6, 90, 0, 0, 0, 13).gear("rapier", "parrydagger", "leather")
        return def.kit(["Step", "Stab", "Slash", "Lunge", "Parry", "Riposte"])
    }

    // --- props and test fighters ---------------------------------------------------------

    // Lesson 1's rotten stump (first arc, scene 4): it doesn't fight back.
    static practiceStump {
        return BattleCritter.fromSheet("Practice Stump", GoblinVikingBattleSprite).stats(8, 0, 0, 0, 0, 0).trait("passive", true).trait("immovable", true).tint(Vec4.new(170, 130, 95, 255))
    }

    // Lesson 2's swing-post: every time it is hit while still, its arm comes round at the cell in front of it 0.8 s
    // later.
    static swingPost {
        var def = BattleCritter.fromSheet("Swing-post", GoblinVikingBattleSprite).stats(6, 0, 0, 0, 0, 0)
        def.trait("passive", true).trait("immovable", true).trait("noStamina", true).tint(Vec4.new(205, 175, 120, 255))
        return def.trait("reactive", Fn.new {
            return Move.new("Arm Swing", 1, 1.2, 0).attackFlat(0.8, 2, 0.4).reach(1).heavy().react("unflinching").anim("attack")
        })
    }

    // Thomas's training dummy (first arc, the lesson's first part): it never fights back, and the lesson ends by
    // script rather than by breaking it.
    static lessonDummy {
        return BattleCritter.fromSheet("Training Dummy", GoblinVikingBattleSprite).stats(9, 0, 0, 0, 0, 0).trait("passive", true).trait("immovable", true).trait("endless", true).tint(Vec4.new(170, 130, 95, 255))
    }

    // Thomas's snap-post (the lesson's last part, standing in for a thornwolf): whenever someone starts a chain in the
    // cell in front of it, it snaps at them half a second later - Brace first, then hit it while it's still.
    static snapPost {
        var def = BattleCritter.fromSheet("Snap-post", GoblinVikingBattleSprite).stats(30, 0, 0, 0, 0, 0)
        def.trait("passive", true).trait("immovable", true).trait("noStamina", true).tint(Vec4.new(150, 120, 90, 255))
        return def.trait("snaps", Fn.new {
            return Move.new("Snap", 1, 0.9, 0).attackFlat(0.5, 1, 0.3).reach(1).anim("attack")
        })
    }

    // The Training Dummy (States/BattleTestState.wren): a punching bag for trying moves and gear. Effectively endless
    // HP and it never acts, so the damage and stagger it shows are a neutral baseline.
    static trainingDummy {
        return BattleCritter.fromSheet("Training Dummy", GoblinVikingBattleSprite).stats(9, 0, 0, 0, 0, 0).trait("passive", true).trait("endless", true)
    }

    // A def wearing the generated battle sheet of the NPC called `name`.
    static fromNpc(name) { BattleCritter.fromSheet(name, NPCData.forName(name).battleSprite) }

    // A def wearing a generated battle sheet (a Sprites/<Name>BattleSprite class): right-facing anims plus the
    // left-facing ones, so Battler never has to mirror it.
    static fromSheet(name, sheet) {
        var data = sheet.data
        var origin = data["origin"]
        var def = BattleCritter.new(name, data["texture"], data["columns"], data["rows"], Vec2.new(origin[0], origin[1]),
            1, SheetAnims.forSheet(data, "right", name))
        return def.withLeftAnims(SheetAnims.forSheet(data, "left", name))
    }
}
