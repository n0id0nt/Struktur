// Combat/Encounter.wren
// A scripted fight: who the opposition is and where it stands, in one or more stages played back to back in the same
// arena, with the lines that teach it. States/CombatState.wren runs one whenever a fight has a script - otherwise a
// fight is just "these opponents, lined up".
//
// The first arc's battle tutorial (First Arc Script) is built from these:
//   Thomas (the "startDuel Thomas" dialogue command)
//     Lesson 1  get close, then hit - the practice stump, which doesn't fight back: attacks only reach the next
//               cell, a chain fills about 4 seconds, and what Hold is for.
//     Lesson 2  hit, then block - the swing-post, whose arm comes round 0.8 s after every hit. Thomas teaches Brace.
//   The first wolves (any fight against Starving Thornwolves)
//     Lesson 3  block, then punish - a starving wolf bites the instant it starts a chain next to you, then circles.
//   The constructs, alone: read their timelines, Brace the heavy hit, then punish.
//   The Warden (the "startBattle Warden" command): the boss, one statue whatever the level holds.
//
// A stage's lines come in three kinds: `intro` (a prompt box before the first selection - the clock is stopped
// anyway), `outro` (after the stage is won) and call-outs raised from `onEvent` as the fight plays: host.say(text)
// puts a line on the banner without stopping anything, host.prompt(lines) queues a prompt box for the next
// selection stop. `hint` stays on the command panel while a chain is being built.
// Lines are [speaker, text]; a null speaker is a plain tip.
import "Combat/MoveBook" for MoveBook
import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Boss/Warden" for Warden

class EncounterStage {
    // enemies: [[def, lane, row], ...] - lane / row null lines them up in the back row.
    construct new(enemies) {
        _enemies = enemies
        _intro = []
        _outro = []
        _hint = null
        _onStart = null
        _onEvent = null
    }

    enemies { _enemies }
    intro { _intro }
    outro { _outro }
    hint { _hint }
    onStart { _onStart }
    onEvent { _onEvent }

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
}

class Encounter {
    construct new(name, stages) {
        _name = name
        _stages = stages
    }

    name { _name }
    stages { _stages }

    // The script for a fight started from dialogue against `challenger` ("startDuel" / "startBattle"), or null.
    static forChallenger(challenger) {
        if (challenger == "Thomas") return Encounter.thomasLessons
        if (challenger == "Warden") return Encounter.warden
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

    static thomasLessons {
        var stump = EncounterStage.new([[BattleCritter.practiceStump, 1, 2]])
        stump.intro([
            ["Thomas", "Everything you do in a fight takes time, and so does everything they do. Nobody can hurry that. Not you, not a wolf, not a lord. So you pick a few moves at a time, and then you watch them play out."],
            [null, "Choose moves until the chain bar fills (about 4 seconds). The fight runs until someone's chain ends, then you choose again."],
            ["Thomas", "That sword only reaches the spot right in front of you. So first you Step up, then you Slash. Go on. Get close, then hit."],
            [null, "Step moves you one cell in any direction. Slash hits the cell in front of you."]
        ])
        stump.hint("Try: Step, Slash, Slash, Hold, Slash")
        stump.onEvent(Fn.new { |host, event|
            // The player's first chain has played out on the stump.
            if (event["type"] == "chainEnd" && event["who"].name == "You" && host.once("stump.hold")) {
                var lines = [["Thomas", "Good! See that bit where you stood there puffing? That's Hold. It's the only way you get your wind back. Use it when nobody's swinging at you. Never twice in a row, mind: you'll fall asleep."]]
                if (host.inParty("Kira")) {
                    lines.add(["Kira", "And I don't have to Step at all. Fact."])
                    lines.add(["Thomas", "Your arrows fly till they hit something, love. His sword doesn't. So she shoots from the back, and you go to them."])
                    lines.add([null, "Quick Shot hits the first enemy in Kira's lane, however far away."])
                }
                host.prompt(lines)
            }
        })

        var post = EncounterStage.new([[BattleCritter.swingPost, 1, 2]])
        post.intro([
            ["Thomas", "Now this one hits back. Whack it and the arm comes round a breath later. So here's a new trick. Tuck in and take it. Brace."],
            [null, "New move: Brace. Kira learns it too. Brace blocks hits that land while you're tucked in, best right in the middle."],
            ["Thomas", "Get close. Hit it. Then Brace, before the arm comes round. Hit, then block. Hit, then block. That's how you stay standing."]
        ])
        post.hint("Try: Step, Slash, Brace, Slash, Brace")
        post.onStart(Fn.new { |host| host.teach("Brace") })
        post.onEvent(Fn.new { |host, event|
            // Slashed without Bracing: the arm catches them.
            if (event["type"] == "hit" && event["who"].side == 0 && !event["blocked"] && host.once("post.caught")) {
                host.say("Thomas: Ooh. You swung, it swung back. Never just keep swinging. Hit, then block.")
            }
            if (event["type"] == "block" && event["who"].side == 0 && host.once("post.blocked")) {
                host.say("Thomas: There! Tucked in right as it landed.")
            }
        })
        post.outro([["Thomas", "That's it! Big swing, little wait, tuck in. You'll be fine. Probably."]])

        return Encounter.new("Thomas's lessons", [stump, post])
    }

    // --- the first wolves (scene 6) ---------------------------------------------------------

    static wolves_ {
        var stage = EncounterStage.new([])
        stage.onStart(Fn.new { |host|
            if (host.inParty("Kira")) host.say("Kira: Thornwolves! They're so skinny... Remember the post! Get close, hit, block!")
        })
        stage.onEvent(Fn.new { |host, event|
            var type = event["type"]
            var who = event["who"]
            // The first bite that gets through.
            if (type == "hit" && who.name == "You" && !event["blocked"] && event["amount"] > 0 && host.once("wolf.bitten")) {
                host.prompt([
                    [host.inParty("Kira") ? "Kira" : null, "It bit you before you'd even moved! It's faster than the post!"],
                    [null, "Watch the wolf's timeline. A starving wolf bites the moment it starts a chain next to you. Start your chain with Brace when a wolf is beside you, then Slash it while it circles."]
                ])
            }
            // Block a bite, then land the punish before the wolf has finished its chain.
            if (type == "block" && who.name == "You") host.mark("wolf.blockedAt", event["time"])
            if (type == "hit" && who.side == 1 && event["by"].name == "You" && host.marked("wolf.blockedAt") &&
                event["time"] - host.markOf("wolf.blockedAt") < 2.5 && host.once("wolf.punished")) {
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
