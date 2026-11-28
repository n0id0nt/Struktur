// Story/QuestLog.wren
// The quest system. Quests are defined in JSON (QUEST_FILES, e.g. assets/Quests/Arc1Quests.json):
//   { "categories": [category ids, in display order],
//     "quests": [ { "id", "category", "start": [conditions], "objectives": [ { "id", "done": [conditions] } ],
//                   "complete": [conditions] (optional) } ] }
// A condition is { "flag", "op" ("==" "!=" ">=" ">" "<=" "<"), "value" } on an int flag - the same flags the
// dialogue sets (arc1_stage, well_fixed...) - and a list of them must all hold. So progress is worked out from the
// saved flags every time it's asked for: nothing extra to save, and older saves show the right progress at once.
//
// A quest is:
//   hidden    until its "start" conditions hold (or any objective is done, or a dialogue starts it - startQuest);
//   active    after that;
//   complete  once its "complete" conditions hold - or, with none, once every objective is done (or completeQuest).
// The dialogue commands startQuest / completeObjective / completeQuest (Dialogue/RegisteredFunctions/Commands.wren)
// set the flags "quest.<id>" (1 started, 2 complete) and "quest.<id>.<objective>" (1 done), for quests that aren't
// tied to existing story flags.
//
// Changes are announced as game events (questStarted / questUpdated / questCompleted - Events/GameEvents.wren) by
// setFlag, the one way quest-relevant flags are written - so loading a save announces nothing.
//
// Display text comes from the localization file: "quest.category.<category>", "quest.<id>.title",
// "quest.<id>.summary" and "quest.<id>.<objective>".
import "flags" for FlagManager
import "fileSystem" for FileSystem
import "serialisation" for Json
import "debug" for Debug
import "Events/GameEvents" for GameEvents

var QUEST_FILES = ["Quests/Arc1Quests.json"]

var HIDDEN = "hidden"
var ACTIVE = "active"
var COMPLETE = "complete"

class QuestLog {
    static hidden { HIDDEN }
    static active { ACTIVE }
    static complete { COMPLETE }

    // Category ids in display order.
    static categories {
        QuestLog.load_()
        return __categories
    }

    // Every quest's progress, in definition order - see status(id) for the shape.
    static all {
        QuestLog.load_()
        return __quests.map { |def| QuestLog.evaluate_(def) }.toList
    }

    // { "id", "category", "state" (hidden / active / complete), "objectives": [ { "id", "done" } ],
    //   "done" (objectives done), "total", "fraction" (0..1 - 1 when complete) }, or null for an unknown id.
    static status(id) {
        var def = QuestLog.def_(id)
        return def == null ? null : QuestLog.evaluate_(def)
    }

    // How far through every quest the player is: objectives done / objectives in all quests (0..1).
    static overallFraction {
        var done = 0
        var total = 0
        for (quest in QuestLog.all) {
            done = done + quest["done"]
            total = total + quest["total"]
        }
        return total == 0 ? 0 : done / total
    }

    // --- changing progress ---------------------------------------------------------------

    // Sets the int flag `name` to `value` (and saves the flags), then sends a game event (Events/GameEvents.wren) for
    // every quest change it caused: a quest starting, an objective done, a quest completing. Every int-flag write
    // that can move a quest goes through here (the setIntFlag dialogue command, the commands below, a won duel) -
    // quest conditions only read int flags, so nothing else can change a quest.
    static setFlag(name, value) {
        var before = QuestLog.all
        FlagManager.setIntFlag(name, value)
        FlagManager.save("flags.sav")
        QuestLog.announce_(before, QuestLog.all)
    }

    // Sends the events for what changed between two QuestLog.all snapshots (same order - definition order).
    static announce_(before, after) {
        for (i in 0...after.count) {
            var quest = after[i]
            var old = before[i]
            var id = quest["id"]
            var state = quest["state"]
            var oldState = old["state"]
            if (state == HIDDEN || state == oldState && state == COMPLETE) continue
            if (oldState == HIDDEN) GameEvents.questStarted(id)
            if (state == COMPLETE) {
                GameEvents.questCompleted(id)
                continue
            }
            for (j in 0...quest["objectives"].count) {
                var objective = quest["objectives"][j]
                if (objective["done"] && !old["objectives"][j]["done"]) GameEvents.questUpdated(id, objective["id"])
            }
        }
    }

    // --- dialogue commands --------------------------------------------------------------

    static start(id) {
        if (!QuestLog.known_(id)) return
        var key = "quest.%(id)"
        if (QuestLog.flag_(key) < 1) QuestLog.set_(key, 1)
    }

    static completeObjective(id, objective) {
        if (!QuestLog.known_(id)) return
        QuestLog.start(id)
        QuestLog.set_("quest.%(id).%(objective)", 1)
    }

    static finish(id) {
        if (!QuestLog.known_(id)) return
        QuestLog.set_("quest.%(id)", 2)
    }

    // --- internals ---------------------------------------------------------------------

    static load_() {
        if (__quests != null) return
        __quests = []
        __categories = []
        for (path in QUEST_FILES) {
            var result = FileSystem.readString(path)
            if (!result.success) {
                Debug.error("[QuestLog] couldn't read %(path): %(result.errorMessage)")
                continue
            }
            var data = Json.parse(result.value)
            if (!(data is Map)) {
                Debug.error("[QuestLog] couldn't parse %(path)")
                continue
            }
            var categories = data["categories"]
            if (categories is List) {
                for (category in categories) {
                    if (!__categories.contains(category)) __categories.add(category)
                }
            }
            var quests = data["quests"]
            if (quests is List) {
                for (quest in quests) {
                    if (quest["objectives"] == null) quest["objectives"] = []
                    __quests.add(quest)
                    var category = quest["category"]
                    if (category != null && !__categories.contains(category)) __categories.add(category)
                }
            }
        }
    }

    static def_(id) {
        QuestLog.load_()
        for (def in __quests) {
            if (def["id"] == id) return def
        }
        return null
    }

    static known_(id) {
        if (QuestLog.def_(id) != null) return true
        Debug.warning("[QuestLog] no quest '%(id)'")
        return false
    }

    static evaluate_(def) {
        var id = def["id"]
        var objectives = []
        var done = 0
        for (objective in def["objectives"]) {
            var objectiveId = objective["id"]
            var isDone = QuestLog.flag_("quest.%(id).%(objectiveId)") >= 1 || QuestLog.holds_(objective["done"], false)
            if (isDone) done = done + 1
            objectives.add({"id": objectiveId, "done": isDone})
        }
        var total = objectives.count
        var questFlag = QuestLog.flag_("quest.%(id)")
        var state = HIDDEN
        var completeConditions = def["complete"]
        var isComplete = questFlag >= 2
        if (!isComplete) {
            isComplete = completeConditions != null ? QuestLog.holds_(completeConditions, false) : (total > 0 && done == total)
        }
        if (isComplete) {
            state = COMPLETE
            done = total
            for (objective in objectives) {
                objective["done"] = true
            }
        } else if (questFlag >= 1 || done > 0 || QuestLog.holds_(def["start"], false)) {
            state = ACTIVE
        }
        var status = {}
        status["id"] = id
        status["category"] = def["category"]
        status["state"] = state
        status["objectives"] = objectives
        status["done"] = done
        status["total"] = total
        status["fraction"] = state == COMPLETE ? 1 : (total == 0 ? 0 : done / total)
        return status
    }

    // Whether every condition in the list holds; `empty` is the answer for a missing or empty list.
    static holds_(conditions, empty) {
        if (!(conditions is List) || conditions.count == 0) return empty
        for (condition in conditions) {
            if (!QuestLog.test_(condition)) return false
        }
        return true
    }

    static test_(condition) {
        var value = QuestLog.flag_(condition["flag"])
        var target = condition["value"]
        if (!(target is Num)) target = 1
        var op = condition["op"]
        if (op == null || op == ">=") return value >= target
        if (op == "==") return value == target
        if (op == "!=") return value != target
        if (op == ">") return value > target
        if (op == "<=") return value <= target
        if (op == "<") return value < target
        Debug.warning("[QuestLog] unknown condition op '%(op)'")
        return false
    }

    static flag_(name) {
        if (name == null || !FlagManager.hasIntFlag(name)) return 0
        return FlagManager.getIntFlag(name)
    }

    static set_(name, value) { QuestLog.setFlag(name, value) }
}
