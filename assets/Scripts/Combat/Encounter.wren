// Combat/Encounter.wren
// A scripted fight: who the opposition is and where it stands, in one or more stages played back to back in the same
// arena, with the lines that teach it. States/CombatState.wren runs one whenever a fight has a script - otherwise a
// fight is just "these opponents, lined up".
//
// The first arc's battle tutorial (First Arc Script) is built from these:
//   Thomas's lesson (the "startDuel Thomas" dialogue command at the training grounds), the player alone
//     The dummy     one chain of nothing but Steps - then the player is set in front of the dummy - one of nothing
//                   but Slashes, then out of breath: Hold (never twice in a row), and the rest of that chain is
//                   theirs. The dummy can't be beaten (it never loses health).
//   The perimeter wolf (Thomas talks you through it at the north gate and teaches Brace there - Dialogue/
//     Arc1/Thomas.json "thomas_gate"; it charges and you fight it alone - GameObjects/PerimeterWolf.wren)
//   The first wolves (any fight against Starving Thornwolves)
//   Thomas's test (the same command once you're back from the hunt): Thomas himself, against you and Kira.
//   The constructs, alone: read their timelines, Brace the heavy hit, then punish.
//   The Warden (the "startBattle Warden" command): the boss, one statue whatever the level holds.
//   Finn's sparring match (the "startDuel Finn" command, home from the shrine): call-outs only.
//
// A stage's lines come in three kinds: `intro` (a prompt box before the first selection - the clock is stopped
// anyway), `outro` (after the stage is won) and call-outs raised from `onEvent` as the fight plays: host.say(text)
// puts a line on the banner without stopping anything, host.prompt(lines) queues a prompt box for the next
// selection stop. `hint` stays on the command panel while a chain is being built. A lesson steps through `onRound`
// (each time the clock stops for new chains) and `onPick` (each move added), using host.restrict / hint / exhaust /
// finish (States/CombatState.wren).
// Lines are [speaker, text]; a null speaker is a plain tip.
import "Combat/MoveBook" for MoveBook
import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Boss/Warden" for Warden
import "flags" for FlagManager

// Thomas's part of the first arc, an int flag (Dialogue/Arc1/Thomas.json): 0 not taught, 1 lesson done and the
// wolf hunt given, 3 back from the hunt - the test is next.
var LESSON_FLAG = "thomas_lesson"

class EncounterStage {
    // enemies: [[def, lane, row], ...] - lane / row null lines them up in the back row.
    construct new(enemies) {
        _enemies = enemies
        _intro = []
        _outro = []
        _hint = null
        _onStart = null
        _onEvent = null
        _onRound = null
        _onPick = null
    }

    enemies { _enemies }
    intro { _intro }
    outro { _outro }
    hint { _hint }
    onStart { _onStart }
    onEvent { _onEvent }
    onRound { _onRound }
    onPick { _onPick }

    intro(lines) {
        _intro = lines
        return this
    }
    outro(lines) {
        _outro = lines
        return this
    }
    hint(text) {
        _hint = text
        return this
    }
    // fn(host) as the stage begins (teach a move, say).
    onStart(fn) {
        _onStart = fn
        return this
    }
    // fn(host, event) for every engine event while the stage runs.
    onEvent(fn) {
        _onEvent = fn
        return this
    }
    // fn(host, round) each time the clock stops for the party's next chains (1 = the stage's first) - a lesson's
    // next step: restrict, hint, prompt, exhaust or finish.
    onRound(fn) {
        _onRound = fn
        return this
    }
    // fn(host, move) each time a move joins the chain being built.
    onPick(fn) {
        _onPick = fn
        return this
    }
}

class Encounter {
    construct new(name, stages) {
        _name = name
        _stages = stages
        _solo = false
    }

    name { _name }
    stages { _stages }
    // Only the party's leader fights - the rest watch (Thomas's lesson).
    solo { _solo }

    soloFight() {
        _solo = true
        return this
    }

    // The script for a fight started from dialogue against `challenger` ("startDuel" / "startBattle"), or null.
    static forChallenger(challenger) {
        // Thomas: the lesson first; the test once the perimeter wolf is dealt with.
        if (challenger == "Thomas") return FlagManager.getIntFlag(LESSON_FLAG) >= 3 ? Encounter.thomasDuel : Encounter.thomasLessons
        if (challenger == "Warden") return Encounter.warden
        if (challenger == "Finn") return Encounter.finnSpar
        return null
    }

    // Lines for an ordinary fight against these enemy names (the first wolves, the constructs), or null. Wraps the
    // opponents the fight already has: `enemies` is left empty.
    static forEnemies(names, partySize) {
        if (names.any { |name| name == "Starving Thornwolf" }) return Encounter.wolves_
        if (partySize == 1 && names.any { |name| name == "Runic Construct" }) return Encounter.constructs_
        return null
    }

    // --- Thomas teaches you to fight (scene 4) -------------------------------------------------
    // One step per chain, each allowing only the kind of move it teaches: Step about, then Slash (from the cell in front
    // of the dummy, so every swing lands), then (out of breath) Hold - after which the chain is the player's to finish.
    // Brace and the wolf come after, at the north gate.

    static thomasLessons {
        var dummy = EncounterStage.new([[BattleCritter.lessonDummy, 1, 2]])
        dummy.onRound(Fn.new { |host, round|
            if (round == 1) {
                host.restrict("movement")
                host.hint("Fill the chain with Step")
                host.prompt([
                    ["Thomas", "Right. In a fight everything takes time - what you do, and what they do. So you don't just flail. You pick a few moves, then you watch them play out."],
                    [null, "Choose moves until the chain bar fills (about 3 seconds). Then they play out one after another, and you choose again."],
                    ["Thomas", "Feet first. Step about. Fill the whole chain with steps."],
                    [null, "Moves > Step, then pick a cell. For now, moving is all you can choose."]
                ])
            } else if (round == 2) {
                // Wherever the steps ended, the player starts this round right in front of the dummy - every Slash lands.
                host.placeInFrontOfFoe()
                host.restrict("attack")
                host.hint("Fill the chain with Slash")
                host.prompt([
                    ["Thomas", "Good. Now step up and hit it. That sword only reaches the spot right in front of you, mind."],
                    [null, "Slash hits the cell in front of you. This time only attacks can be chosen - fill the chain with them."]
                ])
            } else if (round == 3) {
                host.exhaust()
                host.restrict("hold")
                host.hint("Pick Hold")
                host.prompt([
                    ["Thomas", "Ha! Look at you, puffing like a bellows. Every swing costs you breath."],
                    [null, "Your stamina (the bar under your health) is empty. Moves cost stamina - without it you can't do anything."],
                    ["Thomas", "So you Hold. Stand still and get your wind back. Go on."],
                    [null, "Pick Hold, then how long to rest. The longer you hold, the more stamina comes back."]
                ])
            } else if (round == 4) {
                host.finish()
            }
        })
        dummy.onPick(Fn.new { |host, move|
            if (move.isHold && host.once("dummy.held")) {
                host.restrict(null)
                host.hint("Finish it however you like")
                host.prompt([
                    ["Thomas", "That's it. Just never Hold twice in a row. Stand about that long and you'll nod off."],
                    [null, "Hold can't follow Hold. Finish this chain however you like."]
                ])
            }
        })
        dummy.outro([["Thomas", "Feet, sword, breath. That's most of it."]])

        return Encounter.new("Thomas's lesson", [dummy]).soloFight()
    }

    // The test after the wolf hunt: Thomas himself, against the player and Kira.
    static thomasDuel {
        var stage = EncounterStage.new([])
        stage.intro([["Thomas", "No dummies this time. The both of you, against me. Show me you'll come home."]])
        return Encounter.new("Thomas's test", [stage])
    }

    // --- Finn's sparring match (scene 11) -------------------------------------------------------
    // Finn fights with the player's own kit and leaves big openings (Combat/Config/BattleCritter.finn). He calls out
    // the two timings Thomas taught - hit then block, block then hit - and always catches Kira's first arrow with tongs.

    static finnSpar {
        var stage = EncounterStage.new([])
        stage.onEvent(Fn.new { |host, event|
            var type = event["type"]
            var who = event["who"]
            // A party hit on Finn: remember it for "hit, then block"; after a block, it's "block, then hit".
            if (type == "hit" && who.side == 1 && event["by"].side == 0) {
                if (host.marked("finn.blockedAt") && event["time"] - host.markOf("finn.blockedAt") < 2.5 && host.once("finn.blockThenHit")) {
                    host.say("Finn: Block, then hit. Good. Thomas taught you something after all.")
                }
                host.mark("finn.hitAt", event["time"])
            }
            if (type == "block" && who.side == 0) {
                if (host.marked("finn.hitAt") && event["time"] - host.markOf("finn.hitAt") < 2.5 && host.once("finn.hitThenBlock")) {
                    host.say("Finn: Hit, then block. Good.")
                }
                host.mark("finn.blockedAt", event["time"])
            }
            // Kira's first arrow: Finn catches it with a pair of tongs, barely looking (his "catches" trait).
            if (type == "caught") host.say("(Finn catches Kira's arrow with a pair of tongs, without looking up.)")
        })
        return Encounter.new("Sparring with Finn", [stage])
    }

    // --- the first wolves (scene 6) ---------------------------------------------------------

    // Every wolf fight has these, but each line plays once per save (host.firstTime) - there are a lot of wolves.
    static wolves_ {
        var stage = EncounterStage.new([])
        stage.onStart(Fn.new { |host|
            if (host.inParty("Kira") && host.firstTime("seen_wolves_kira")) host.say("Kira: Thornwolves! They're so skinny... Remember what Dad said! Brace the bite, then hit it!")
        })
        stage.onEvent(Fn.new { |host, event|
            var type = event["type"]
            var who = event["who"]
            // The first bite that gets through.
            if (type == "hit" && who.name == "You" && !event["blocked"] && event["amount"] > 0 && host.firstTime("seen_wolves_bite")) {
                host.prompt([
                    [host.inParty("Kira") ? "Kira" : null, "It bit you before you'd even moved! Just like Dad said!"],
                    [null, "Watch the wolf's timeline. A starving wolf bites the moment it starts a chain next to you. Start your chain with Brace when a wolf is beside you, then Slash it while it circles."]
                ])
            }
            // Block a bite, then land the punish before the wolf has finished its chain.
            if (type == "block" && who.name == "You") host.mark("wolf.blockedAt", event["time"])
            if (type == "hit" && who.side == 1 && event["by"].name == "You" && host.marked("wolf.blockedAt") &&
                event["time"] - host.markOf("wolf.blockedAt") < 2.5 && host.firstTime("seen_wolves_punish")) {
                host.say(host.inParty("Kira") ? "Kira: That's it! It bites, you block, then you bonk it!" : "It bites, you block, then you punish.")
            }
        })
        return Encounter.new("The first wolves", [stage])
    }

    // --- separated (scene 8) ------------------------------------------------------------------

    static constructs_ {
        var stage = EncounterStage.new([])
        stage.intro([[null, "No one's covering you. Read their timelines, Brace the heavy hit, then punish."]])
        return Encounter.new("Runic Constructs", [stage])
    }

    // --- the Warden (scene 10) ----------------------------------------------------------------

    static warden {
        var stage = EncounterStage.new([[Warden.def, 1, 3]])
        stage.intro([[null, "The Warden. Normal hits barely scratch its granite skin - wait for the moment it is stuck or wide open (the gold bands on its timeline), and never swing into its guard."]])
        return Encounter.new("The Warden", [stage])
    }
}
