import "gameObjectComponents" for PhysicsBody
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "Util/CharacterCollider" for CharacterCollider
import "debug" for Debug
import "World/StoryGates" for StoryGates
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
import "Sprites/YellowPantsGirlSprite" for YellowPantsGirlSprite
import "Sprites/YellowPantsGirlBattleSprite" for YellowPantsGirlBattleSprite

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
            // Arc 1 cast (Levels/SevenGods.ldtk, Dialogue/Arc1Dialogue.json). Placeholder looks - six generated models
            // cover everyone until the cast gets their own (docs/FirstArcPlan.md).
            "Finn": NPCData.new(WrestlerSprite, WrestlerBattleSprite),
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
            "Warden": NPCData.new(WrestlerSprite, WrestlerBattleSprite),
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
    }

    name { _name }

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
