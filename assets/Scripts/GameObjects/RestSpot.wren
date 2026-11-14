// GameObjects/RestSpot.wren
// A place to rest - a bed, a fireplace, a shrine (LDtk "RestSpot" entity; the art is part of the level's tiles, so
// this draws nothing). Interacting with it heals the whole party (the "rest_spot" / "rest_bed" dialogue nodes run the
// "restParty" command) and makes its linked Spawn (the "SpawnPoint" field) the place the player starts a session and
// wakes up after a wipe (World/SpawnPoint.wren).
//   RestSpotType - Bed (the workshop bed: the opening narration the first time - "rest_bed"), FirePlace, Shrine.
import "World/SpawnPoint" for SpawnPoint
import "debug" for Debug

class RestSpot {
    construct new(entity, args) {
        _entity = entity
        _type = args["RestSpotType"]
        // The linked Spawn's iid, and the level it's in (World::Level fills "<field>Levels" in for entity refs).
        _spawnIid = args["SpawnPoint"]
        _spawnLevel = args["SpawnPointLevels"]
    }

    name { "RestSpot" }

    // The conversation it starts (States/InteractState.wren).
    dialogueEntryPoint() { _type == "Bed" ? "rest_bed" : "rest_spot" }

    // Called by States/InteractState.wren as the interaction begins: this becomes the spawn point.
    onInteractStarted() {
        if (_spawnIid == null || _spawnIid == "") {
            Debug.warning("[RestSpot] no SpawnPoint linked - the spawn point stays where it was")
            return
        }
        SpawnPoint.set(_spawnIid, _spawnLevel)
    }

    start() {}

    update() {}

    onDestroy() {}

    onEvent(type, data) {}

    isInteractable() { true }

    requiresFacing() { true }

    getEntity() { _entity }
}
