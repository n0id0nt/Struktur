// Dialogue/DialogueEntryPoints.wren
// Interactable name -> dialogue node id (Dialogue/Arc1Dialogue.json). The names are exactly as they are in the LDtk
// level (Levels/SevenGods.ldtk): an NPC's "Name" field, an Interactable's "MessageKey" or a Door's "Name". Anything not
// listed falls back to handling itself (an Item picks itself up, an Interactable shows its MessageKey as a localized
// line, a door with KeyIds says it's sealed). RestSpots pick their own node (GameObjects/RestSpot.wren).
import "debug" for Debug

class DialogueEntryPoints {
    static get(interactableName) {
        if (__entries == null) {
            __entries = {
                // --- NPCs (LDtk "NPC" entity, Name field) ---
                "Finn": "finn",
                "Elena": "elena",
                "Garrett": "garrett",
                "HavenVillageChild": "child",
                "Kira": "kira",
                "Thomas": "thomas",
                "Guide": "guide",
                // The shrine chamber's three constructs, each guarding its own door.
                "RunicConstuct1": "construct_1",
                "RunicConstruct2": "construct_2",
                "RunicConstruct3": "construct_3",

                // --- Built into the level (LDtk "Interactable" entity, MessageKey field) ---
                "Tablet1": "tablet_1",
                "Tablet2": "tablet_2",
                "Tablet3": "tablet_3",
                "Tablet4": "tablet_4",
                "Tablet5": "tablet_5",
                "ShrineCrystal": "shrine_crystal",
                "MemoryMuneral1": "memory_murals",
                "MemoryMuneral2": "memory_murals",
                "MemoryMuneral3": "memory_murals",
                "MemoryMuneral4": "memory_murals",
                "CentralMural": "central_mural",
                // All three Wardens - talking to any of them starts the one fight against all three.
                "Warden": "warden",
                "MemoryViewer": "memory_viewer",

                // --- Doors (LDtk "Door" entity, Name field) ---
                // Doors with KeyIds (the construct and warden doors) aren't listed: they say they're sealed until
                // their story events happen, then open by themselves.
                "WisperWoodsDoor": "wisperwood_door",
                "WisperWoodsDoor2": "wisperwood_door",
                "ShrineEntranceDoor": "shrine_door",

                // --- Written but not placed in the level yet: an Interactable with this MessageKey picks it up ---
                "Village Well": "village_well",
                "Claw Marks": "claw_marks",
                "Blue Lights": "blue_lights",
                "Stone Pillars": "stone_pillars",
                "Shrine Entry": "shrine_entry_kira",
                "Shifting Floor": "shifting_floor",
                "Wounded Thornwolf": "wounded_wolf"
            }
        }
        var entry = __entries[interactableName]
        if (entry == null) {
            Debug.info("No dialogue entry point for '%(interactableName)' - it handles itself")
        }
        return entry
    }
}
