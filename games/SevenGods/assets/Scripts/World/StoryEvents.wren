// World/StoryEvents.wren
// One-off story events, by id - the ids a level Door's "KeyIds" field lists (LDtk). A door with KeyIds opens by itself
// once every one of its events has happened (GameObjects/Door.wren), so "Defeat RunicConstuct1" or "Finn Opens
// Wisperwood" in LDtk is all it takes to wire a door to the story. Events are fired by the "triggerEvent" dialogue
// command (Dialogue/RegisteredFunctions/Commands.wren) and saved, so a door stays opened in later sessions.
import "flags" for FlagManager

class StoryEvents {
    // The saved flag behind an event.
    static flag_(id) { "Event %(id)" }

    static fire(id) {
        if (id == null || id == "" || StoryEvents.happened(id)) {
            return
        }
        FlagManager.setFlag(StoryEvents.flag_(id), true)
        FlagManager.save("flags.sav")
    }

    static happened(id) { FlagManager.getFlag(StoryEvents.flag_(id)) == true }

    static allHappened(ids) {
        for (id in ids) {
            if (!StoryEvents.happened(id)) {
                return false
            }
        }
        return true
    }
}
