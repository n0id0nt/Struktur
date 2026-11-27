"""Temporarily adds a self-running check to a game's scripts. run_check.ps1 calls this; see tools/checks/README.md.

    python scenarios.py apply <scenario> <game folder> <backup folder>
    python scenarios.py restore <game folder> <backup folder>

Each scenario mutes the game, drives itself, writes what it saw to check_report.txt in the save folder, and names the
screenshots it wants through check_shot.txt ("done" ends the run).
"""
import pathlib
import shutil
import sys

MUTE = '''        // CHECK (temporary)
        Audio.setMasterVolume(0)
'''

# Shared by every scenario's hook class: a report that is rewritten whole, and the screenshot marker.
REPORT_METHODS = '''    checkNote_(line) {
        _checkLines.add(line)
        FileSystem.writeString("check_report.txt", _checkLines.join("\\n"))
        Debug.info("CHECK %(line)")
    }

    checkShot_(name) {
        FileSystem.writeString("check_shot.txt", name)
    }

'''


class Patcher:
    def __init__(self, game, backup):
        self.game = pathlib.Path(game)
        self.backup = pathlib.Path(backup)
        self.touched = []

    def substitute(self, relative, old, new):
        path = self.game / relative
        if relative not in self.touched:
            copy = self.backup / relative
            copy.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, copy)
            self.touched.append(relative)
            (self.backup / "manifest.txt").write_text("\n".join(self.touched), encoding="utf-8")
        text = path.read_bytes().decode("utf-8")
        newline = "\r\n" if "\r\n" in text else "\n"
        text = text.replace("\r\n", "\n")
        if old not in text:
            raise SystemExit("%s: not found: %s" % (relative, old.strip()[:80]))
        text = text.replace(old, new, 1)
        path.write_bytes(text.replace("\n", newline).encode("utf-8"))


MAIN = "assets/Scripts/Main.wren"


def ride(patcher):
    """The indoor cycling screens on simulated devices (run with --demo): free ride, workout, free ride again."""
    patcher.substitute(MAIN, 'import "debug" for Debug\n', '''import "debug" for Debug
import "audio" for Audio
import "fitness" for Fitness
import "world" for World
''')
    patcher.substitute(MAIN, '        _stateManager.changeState("DevicePairing")\n    }\n', MUTE + '''        var profile = RiderProfiles.newProfile()
        profile["name"] = "Check Rider"
        RiderProfiles.select(RiderProfiles.create(profile))
        Fitness.connectRemembered()
        _checkStart = Time.unscaledTime
        _checkStep = 0
        _checkLines = []
        _stateManager.changeState("MainMenu")
    }

''' + REPORT_METHODS + '''    checkRide_(mode) {
        var params = {}
        params["mode"] = mode
        params["workoutId"] = "Workouts/Demo Quick Tour.zwo"
        _stateManager.changeState("Ride", params)
    }

    checkRiding_(label) {
        checkNote_("%(label): state %(_stateManager.currentState.name), world loaded %(World.isLoaded), " +
            "cadence %(Input.trainer["cadence"] != null), moving %(Input.trainer["speed"] > 1)")
    }

    check_() {
        var time = Time.unscaledTime - _checkStart
        if (_checkStep == 0 && time > 4) {
            checkRide_("freeRide")
            _checkStep = 1
        } else if (_checkStep == 1 && time > 6) {
            World.jumpTo(1300)
            _checkStep = 2
        } else if (_checkStep == 2 && time > 20) {
            checkRiding_("free ride")
            checkShot_("free-ride")
            _checkStep = 3
        } else if (_checkStep == 3 && time > 22) {
            _stateManager.changeState("MainMenu")
            _checkStep = 4
        } else if (_checkStep == 4 && time > 25) {
            checkNote_("menu: world loaded %(World.isLoaded)")
            checkShot_("menu")
            _checkStep = 5
        } else if (_checkStep == 5 && time > 27) {
            checkRide_("workout")
            _checkStep = 6
        } else if (_checkStep == 6 && time > 45) {
            checkRiding_("workout")
            checkShot_("workout")
            _checkStep = 7
        } else if (_checkStep == 7 && time > 47) {
            _stateManager.changeState("MainMenu")
            _checkStep = 8
        } else if (_checkStep == 8 && time > 49) {
            checkRide_("freeRide")
            _checkStep = 9
        } else if (_checkStep == 9 && time > 70) {
            checkRiding_("second free ride")
            checkShot_("second-free-ride")
            _checkStep = 10
        } else if (_checkStep == 10 && time > 72) {
            _stateManager.changeState("MainMenu")
            _checkStep = 11
        } else if (_checkStep == 11 && time > 75) {
            checkNote_("end: world loaded %(World.isLoaded)")
            checkShot_("done")
            _checkStep = 12
        }
    }
''')
    patcher.substitute(MAIN, '    update() {\n', '    update() {\n        check_()\n')


def overworld(patcher):
    """Seven Gods: starts in the overworld with the installed saves and reports what they loaded."""
    patcher.substitute(MAIN, 'import "debug" for Debug\n', 'import "debug" for Debug\nimport "audio" for Audio\n')
    patcher.substitute(MAIN, '        _stateManager.changeState("GameWorld")\n    }\n', MUTE + '''        _checkStart = Time.unscaledTime
        _checkStep = 0
        _checkLines = []
        _stateManager.changeState("GameWorld")
    }

''' + REPORT_METHODS + '''    check_() {
        var time = Time.unscaledTime - _checkStart
        if (_checkStep == 0 && time > 40) {
            var members = []
            for (member in Party.members) members.add(member.name)
            var items = []
            for (stack in Inventory.stacks) items.add("%(stack.id) x%(stack.count)")
            checkNote_("state: %(_stateManager.currentState.name)")
            checkNote_("room: %(MapLog.current)")
            checkNote_("visited: %(MapLog.visited)")
            checkNote_("party: %(members)")
            checkNote_("items: %(items)")
            checkNote_("gold: %(Inventory.gold)")
            checkNote_("moves equipped: %(MoveBook.equipped)")
            checkNote_("moves unlocked: %(MoveBook.unlocked)")
            checkNote_("defeated: %(Defeated.iids.count)")
            checkShot_("overworld")
            _checkStep = 1
        } else if (_checkStep == 1 && time > 50) {
            checkNote_("after 50 s: state %(_stateManager.currentState.name)")
            checkShot_("done")
            _checkStep = 2
        }
    }
''')
    patcher.substitute(MAIN, '    update() {\n', '    update() {\n        check_()\n')


def battle(patcher, scenario):
    """Seven Gods: a battle-test fight in which both sides play themselves."""
    patcher.substitute(MAIN, 'import "debug" for Debug\n', 'import "debug" for Debug\nimport "audio" for Audio\n')
    patcher.substitute(MAIN, '        _stateManager.changeState("GameWorld")\n    }\n', MUTE + '''        _stateManager.changeState("BattleTest")
    }
''')
    setup = "assets/Scripts/States/BattleTestState.wren"
    patcher.substitute(setup, 'import "app" for Time\n', '''import "app" for Time
import "fileSystem" for FileSystem
import "debug" for Debug
''' if 'import "debug" for Debug' not in (patcher.game / setup).read_text(encoding="utf-8") else '''import "app" for Time
import "fileSystem" for FileSystem
''')
    patcher.substitute(setup, '    update(stateManager) {\n        if (!_inBattle) {\n', '''    ''' + REPORT_METHODS.strip() + '''

    check_() {
        if (_checkLines == null) {
            _checkLines = []
            _checkFrames = 0
            _checkStarted = null
            _checkEnded = false
        }
        _checkFrames = _checkFrames + 1
        if (_checkFrames == 59) _scenario = ''' + str(scenario) + '''
        if (_checkFrames == 60) {
            startBattle_()
            _checkStarted = Time.unscaledTime
            checkNote_("started: %(SCENARIOS[_scenario]), in battle %(_inBattle)")
        }
        if (_checkStarted == null || _checkEnded) return
        var time = Time.unscaledTime - _checkStarted
        if (_checkShots != 1 && time > 6) {
            _checkShots = 1
            checkShot_("battle")
        }
        if (!_inBattle || time > 240) {
            _checkEnded = true
            checkNote_(_inBattle ? "still fighting after 240 s" : "finished after %(time.floor) s")
            checkShot_("done")
        }
    }

    update(stateManager) {
        check_()
        if (!_inBattle) {
''')
    patcher.substitute(setup, '            _view.setResult(resultText_(CombatOutcome.take()))\n', '''            var checkResult = resultText_(CombatOutcome.take())
            _view.setResult(checkResult)
            checkNote_(checkResult)
''')
    combat = "assets/Scripts/States/CombatState.wren"
    patcher.substitute(combat, 'import "Combat/Move" for Move\n',
                       'import "Combat/Move" for Move\nimport "Combat/EnemyAI" for PlannerBrain\n')
    patcher.substitute(combat, '    update(stateManager) {\n        if (_engine == null) return\n', '''    update(stateManager) {
        if (_engine == null) return
        // CHECK (temporary): the player's side plays itself.
        for (fighter in _engine.fighters) {
            if (fighter.side == 0 && fighter.brain == null) fighter.brain = PlannerBrain.new()
        }
        if (_phase == "prompt" && armed_) nextPrompt_()
''')


SCENARIOS = {
    "ride": ride,
    "overworld": overworld,
    "battle-wolves": lambda patcher: battle(patcher, 2),
    "battle-warden": lambda patcher: battle(patcher, 3),
}


def restore(game, backup):
    backup = pathlib.Path(backup)
    manifest = backup / "manifest.txt"
    if not manifest.exists():
        return
    for relative in manifest.read_text(encoding="utf-8").splitlines():
        shutil.copy2(backup / relative, pathlib.Path(game) / relative)
    manifest.unlink()


def main(arguments):
    if len(arguments) == 4 and arguments[0] == "apply" and arguments[1] in SCENARIOS:
        pathlib.Path(arguments[3]).mkdir(parents=True, exist_ok=True)
        patcher = Patcher(arguments[2], arguments[3])
        SCENARIOS[arguments[1]](patcher)
        print("patched: " + ", ".join(patcher.touched))
    elif len(arguments) == 3 and arguments[0] == "restore":
        restore(arguments[1], arguments[2])
    else:
        raise SystemExit(__doc__ + "\nScenarios: " + ", ".join(SCENARIOS))


if __name__ == "__main__":
    main(sys.argv[1:])
