// World/StoryTriggers.wren
// Walk-in story moments: a Trigger (GameObjects/Trigger.wren - the LDtk "Trigger" entity) asks for its dialogue
// node here, and the gameplay state (States/GameWorldState.wren) plays it as soon as nothing else is on screen. Each
// node plays once per save - every Trigger with the same Node shares that, so a moment can be spread over several
// areas (Kira's Big Book scene covers all of Whisperwood) and still happen once.
import "flags" for FlagManager

class StoryTriggers {
    static flag_(node) { "trigger_%(node)" }

    static fired(node) { FlagManager.getIntFlag(StoryTriggers.flag_(node)) != 0 }

    // Queues `node` (the first request wins until it's taken).
    static request(node) {
        if (__pending == null && !StoryTriggers.fired(node)) __pending = node
    }

    // The queued node, marked as played - or null.
    static take() {
        var node = __pending
        __pending = null
        if (node == null || StoryTriggers.fired(node)) return null
        FlagManager.setIntFlag(StoryTriggers.flag_(node), 1)
        FlagManager.save("flags.sav")
        return node
    }

    // A Trigger's "RequiresFlag": "<int flag> <op> <number>" (op: == != >= > <= <), e.g. "defeated_StarvingThornwolf >= 2".
    // Empty or null always holds.
    static holds(condition) {
        if (condition == null || condition == "") return true
        var parts = condition.split(" ").where { |part| part != "" }.toList
        if (parts.count != 3) return false
        var have = FlagManager.getIntFlag(parts[0])
        var want = Num.fromString(parts[2])
        if (want == null) return false
        var op = parts[1]
        if (op == "==") return have == want
        if (op == "!=") return have != want
        if (op == ">=") return have >= want
        if (op == ">") return have > want
        if (op == "<=") return have <= want
        if (op == "<") return have < want
        return false
    }
}
