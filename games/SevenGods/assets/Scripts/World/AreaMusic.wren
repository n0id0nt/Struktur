// World/AreaMusic.wren
// The overworld song for each kind of place: a level's LDtk "Terrain" field (the same one that picks its battle arena)
// names it. States/GameWorldState.wren switches songs as the player walks into a level of another terrain.

var MUSIC_DIR = "Audio/Musics/"

var TERRAIN_SONGS = {
    "Grass": MUSIC_DIR + "33 - Calm Village.ogg",     // Haven and its fields
    "IndoorBrick": MUSIC_DIR + "9 - Quiet.ogg",       // houses and workshops
    "Woods": MUSIC_DIR + "37 - Dark Forest.ogg",      // Whisperwood
    "Shrine": MUSIC_DIR + "12 - Temple.ogg"           // the shrine's halls
}
var DEFAULT_SONG = MUSIC_DIR + "33 - Calm Village.ogg"

class AreaMusic {
    // The song for a level whose Terrain field is `terrain` (null or unknown: the default).
    static songFor(terrain) {
        if (terrain is String && TERRAIN_SONGS.containsKey(terrain)) return TERRAIN_SONGS[terrain]
        return DEFAULT_SONG
    }
}
