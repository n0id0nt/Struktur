// Story/QuestTracker.wren
// Watches the quest log (Story/QuestLog.wren) for changes and announces them with a banner
// (UI/Notifications/Notifications.wren): a quest starting ("New quest"), an objective done ("Quest updated"), and a
// quest finishing ("Quest complete"). Quest progress is worked out from flags, so instead of every flag-setter
// reporting in, this compares the log against the last look, every POLL_SECONDS.
//
// States/GameWorldState.wren calls poll() every frame and reset() when gameplay closes. The first look after a reset
// only takes note of where things stand - so loading a save doesn't announce every quest already in it.
import "app" for Time
import "localization" for Localization
import "Story/QuestLog" for QuestLog
import "UI/Notifications/Notifications" for Notifications

var POLL_SECONDS = 0.25

class QuestTracker {
    static poll() {
        if (__since == null) __since = POLL_SECONDS
        __since = __since + Time.scaledDelta
        if (__since < POLL_SECONDS) return
        __since = 0

        var now = {}
        for (quest in QuestLog.all) {
            now[quest["id"]] = quest
        }
        var before = __last
        __last = now
        if (before == null) return

        for (id in now.keys) {
            var quest = now[id]
            var old = before[id]
            var oldState = old == null ? QuestLog.hidden : old["state"]
            var state = quest["state"]
            if (state == QuestLog.hidden) continue
            var title = Localization.get("quest.%(id).title")
            if (oldState == QuestLog.hidden) {
                Notifications.push("quest", Localization.get("notify.quest.started"), title)
            }
            if (state == QuestLog.complete) {
                if (oldState != QuestLog.complete) {
                    Notifications.push("quest", Localization.get("notify.quest.complete"), title)
                }
                continue
            }
            // Objectives done since the last look.
            for (i in 0...quest["objectives"].count) {
                var objective = quest["objectives"][i]
                var wasDone = old != null && old["objectives"][i]["done"]
                if (objective["done"] && !wasDone) {
                    var objectiveId = objective["id"]
                    Notifications.push("quest", Localization.get("notify.quest.updated"),
                        Localization.get("quest.%(id).%(objectiveId)"))
                }
            }
        }
    }

    // Forget the last look (gameplay closing, or a save being loaded) - the next poll starts fresh.
    static reset() {
        __last = null
        __since = null
    }
}
