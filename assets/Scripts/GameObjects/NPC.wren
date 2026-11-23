import "gameObjectComponents" for PhysicsBody
import "Util/Shadow" for Shadow
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "Util/CharacterCollider" for CharacterCollider
import "debug" for Debug
import "World/StoryGates" for StoryGates
import "Sprites/FinnSprite" for FinnSprite
import "Sprites/FinnBattleSprite" for FinnBattleSprite
import "Sprites/ClaireSprite" for ClaireSprite
import "Sprites/ClaireBattleSprite" for ClaireBattleSprite
import "Sprites/GoblinVikingSprite" for GoblinVikingSprite
import "Sprites/GoblinVikingBattleSprite" for GoblinVikingBattleSprite
import "Sprites/PeasantGirlSprite" for PeasantGirlSprite
import "Sprites/PeasantGirlBattleSprite" for PeasantGirlBattleSprite
import "Sprites/PinkHairGirlSprite" for PinkHairGirlSprite
import "Sprites/PinkHairGirlBattleSprite" for PinkHairGirlBattleSprite
import "Sprites/WrestlerSprite" for WrestlerSprite
import "Sprites/WrestlerBattleSprite" for WrestlerBattleSprite
import "Sprites/MutantBattleSprite" for MutantBattleSprite
import "Sprites/YellowPantsGirlSprite" for YellowPantsGirlSprite
import "Sprites/YellowPantsGirlBattleSprite" for YellowPantsGirlBattleSprite
import "gameObjectComponents" for SpriteAnimation
import "math" for Vec2
import "Sprites/SpriteSheet" for SpriteSheet

// The clip an NPC is already playing when it appears, by Name and its LDtk Versions (the story stages it's shown in):
// [name, versions, clip]. Finn is at his anvil / forge in the workshop. An NPC's own "Animation" field (LDtk String,
// optional - not added to the NPC entity yet) wins over this table.
// NPCs only shown while an int flag holds a value, by their LDtk iid: [flag, value] (World/StoryGates.wren withFlag).
// Thomas has three spots during his lessons (arc1_stage 2): "thomas_place" 0 his cabin, 1 the training grounds,
// 2 the north gate (Dialogue/Arc1/Thomas.json moves him). Kept here, not as an LDtk field, as the NPC entity has no
// field for it.
var FLAG_GATES = {
    "c96a0770-bde0-11f1-99f1-014b901230a9": ["thomas_place", 0],   // ThomasCabin
    "07e1c04a-e6ea-11f1-a87b-28d043bb8e9f": ["thomas_place", 1],   // HavenVillageTrainingGrounds
    "07e22958-e6ea-11f1-9b7e-28d043bb8e9f": ["thomas_place", 2]    // HavenVillageBorderNorth
}

var STARTING_POSES = [
    ["Finn", [0, 1, 2], "Working"],
    ["Finn", [9, 10], "Working"]
]

class NPCData {
    // The generated sheets this NPC wears (made by 3dModelsToSprites/build_sprites.bat):
    // sprite - Sprites/<Name>Sprite, the overworld movement sheet (idle / walk / run, 8 directions);
    // battleSprite - Sprites/<Name>BattleSprite, the fight sheet (left / right) - Combat/Config/BattleCritter.fromNpc.
    construct new(sprite, battleSprite) {
        _sprite = sprite
        _battleSprite = battleSprite
    }

    sprite { _sprite }
    battleSprite { _battleSprite }

    // Adds the NPC's sprite and every <direction><Animation>Animation clip to `entity`, idling facing down.
    createSprite(entity) { _sprite.create(entity) }

    // The sprite for the NPC called `name`, or null. Six models cover the twelve NPCs, so some share a look.
    // Shared by the standing NPC (this file), its party follower (GameObjects/PartyMember.wren) and its battle
    // look (Combat/Config/BattleCritter.wren, which uses battleSprite).
    static forName(name) {
        var table = {
            // Arc 1 cast (Levels/SevenGods.ldtk, Dialogue/Arc1/). Finn has his own model (with the cutscene
            // clips - Working, LayingIdle, GettingUp, PickingUp); the rest wear placeholder looks until
            // the cast gets their own (docs/FirstArcPlan.md).
            "Finn": NPCData.new(FinnSprite, FinnBattleSprite),
            "Elena": NPCData.new(PeasantGirlSprite, PeasantGirlBattleSprite),
            "Garrett": NPCData.new(WrestlerSprite, WrestlerBattleSprite),
            "Child": NPCData.new(PinkHairGirlSprite, PinkHairGirlBattleSprite),
            "Kira": NPCData.new(YellowPantsGirlSprite, YellowPantsGirlBattleSprite),
            "Thomas": NPCData.new(GoblinVikingSprite, GoblinVikingBattleSprite),
            "Guide": NPCData.new(ClaireSprite, ClaireBattleSprite),
            "Runic Construct": NPCData.new(GoblinVikingSprite, GoblinVikingBattleSprite),
            // Level names (the LDtk NPC Name field) that differ from the character's.
            "HavenVillageChild": NPCData.new(PinkHairGirlSprite, PinkHairGirlBattleSprite),
            "RunicConstuct1": NPCData.new(GoblinVikingSprite, GoblinVikingBattleSprite),
            "RunicConstruct2": NPCData.new(GoblinVikingSprite, GoblinVikingBattleSprite),
            "RunicConstruct3": NPCData.new(GoblinVikingSprite, GoblinVikingBattleSprite),
            "Warden": NPCData.new(WrestlerSprite, MutantBattleSprite),   // the mutant fights; no overworld sheet of its own yet
            // Older test fighters (Combat/Config/BattleCritter.wren, the battle test roster).
            "Villager": NPCData.new(PeasantGirlSprite, PeasantGirlBattleSprite),
            "Duelist": NPCData.new(YellowPantsGirlSprite, YellowPantsGirlBattleSprite),
            "Guardian": NPCData.new(GoblinVikingSprite, GoblinVikingBattleSprite)
        }
        return table.containsKey(name) ? table[name] : null
    }
}

class NPC {

    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        // Optional story window (the Versions LDtk field - the stages it's shown in) - World/StoryGates.wren.
        _gate = StoryGates.fromArgs(args)
        var flagGate = FLAG_GATES[args["Iid"]]
        if (flagGate != null) _gate = StoryGates.withFlag(_gate, flagGate[0], flagGate[1])
        // Optional: the iid of the Path it wanders (the Entity_ref LDtk field - World/PathWalkers.wren).
        _wanderPathIid = args["Entity_ref"]
        _startingClip = NPC.startingClip_(_name, _gate == null ? null : _gate[3], args["Animation"])
    }

    // The clip it starts in (STARTING_POSES), or null for its idle.
    static startingClip_(name, versions, animation) {
        if (animation is String && animation != "") return animation
        if (!(versions is List)) return null
        for (pose in STARTING_POSES) {
            if (pose[0] != name || pose[1].count != versions.count) continue
            var same = true
            for (i in 0...versions.count) {
                if (pose[1][i] != versions[i]) same = false
            }
            if (same) return pose[2]
        }
        return null
    }

    name { _name }
    wanderPathIid { _wanderPathIid }

    // Whether beating this NPC in a duel recruits them into the party (see CombatResolver.win + Party/Party.wren).
    // A method rather than a getter so callers can duck-type it with Reflect.hasMethod(script, "joinsPartyOnDefeat()").
    joinsPartyOnDefeat() { _name == "Duelist" }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        var npcData = NPCData.forName(_name)
        // An NPC placed in the level whose Name isn't in the table yet still shows up - as a stand-in villager.
        if (npcData == null) {
            Debug.warning("No sprite data for NPC %(_name) - add them to NPCData.forName; using a placeholder")
            npcData = NPCData.new(PeasantGirlSprite, PeasantGirlBattleSprite)
        }

        // RenderLayer.ENTITIES already y-sorts by world position, so no manual offset is needed here.
        npcData.createSprite(_entity)
        Shadow.attach(_entity)
        if (_startingClip != null) {
            // Facing down the screen, like the scenes that set it (Dialogue/Arc1/Finn.json "finn_working").
            var clipName = SpriteSheet.clipName(Vec2.new(0, 1), _startingClip)
            if (SpriteSheet.clip(_entity, clipName) != null) {
                SpriteAnimation.setCurrentAnimation(_entity, clipName)
            } else {
                Debug.warning("NPC %(_name): no %(clipName) clip to start in")
            }
        }

        var bodyDef = BodyDefinition.new(BodyType.STATIC_BODY)
        PhysicsBody.create(_entity, bodyDef, CharacterCollider.shape())

        StoryGates.register(_entity, _gate)
    }
    
    // Level viewer (editor): just the sprite, no body. An unknown name gets none, so the viewer shows a placeholder.
    static preview(entity, args) {
        var npcData = NPCData.forName(args["Name"])
        if (npcData != null) {
            npcData.createSprite(entity)
        }
    }

    update() {

    }
    
    onDestroy() {
        StoryGates.unregister(_entity)
    }
    
    onEvent(type, data) {
        
    }
    
    isInteractable() {
        return StoryGates.isVisible(_entity)
    }

    // Only talked to / read when the player is facing it (Player.getInteractEntity).
    requiresFacing() { true }

    getEntity() {
        return _entity
    }

    getInteractId() {
        return "Entrance Door"
    }
}
