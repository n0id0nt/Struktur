import "gameObjectComponents" for PhysicsBody
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "debug" for Debug
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
            // Overworld NPCs (States/ExperimentState.wren spawns them; Dialogue/OverworldDialogue.json).
            "Villager": NPCData.new(PeasantGirlSprite, PeasantGirlBattleSprite),
            "Traveler": NPCData.new(ClaireSprite, ClaireBattleSprite),
            "Duelist": NPCData.new(YellowPantsGirlSprite, YellowPantsGirlBattleSprite),
            // Memory Palace NPCs (Levels/MemoryPalace.ldtk).
            "Scholar": NPCData.new(ClaireSprite, ClaireBattleSprite),
            "Gardener": NPCData.new(YellowPantsGirlSprite, YellowPantsGirlBattleSprite),
            "Cook": NPCData.new(PeasantGirlSprite, PeasantGirlBattleSprite),
            "Inventor": NPCData.new(YellowPantsGirlSprite, YellowPantsGirlBattleSprite),
            "Dreamer": NPCData.new(PinkHairGirlSprite, PinkHairGirlBattleSprite),
            "Astronomer": NPCData.new(PinkHairGirlSprite, PinkHairGirlBattleSprite),
            "Merchant": NPCData.new(WrestlerSprite, WrestlerBattleSprite),
            "Guardian": NPCData.new(GoblinVikingSprite, GoblinVikingBattleSprite),
            "Cordelia": NPCData.new(PinkHairGirlSprite, PinkHairGirlBattleSprite)
        }
        return table.containsKey(name) ? table[name] : null
    }
}

class NPC {

    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
    }

    name { _name }

    // Whether beating this NPC in a duel recruits them into the party (see CombatResolver.win + Party/Party.wren).
    // A method rather than a getter so callers can duck-type it with Reflect.hasMethod(script, "joinsPartyOnDefeat()").
    joinsPartyOnDefeat() { _name == "Duelist" }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        var npcData = NPCData.forName(_name)
        Debug.assertMsg(npcData != null, "No Data for NPC %(_name)")

        // RenderLayer.ENTITIES already y-sorts by world position, so no manual offset is needed here.
        npcData.createSprite(_entity)
        
        var bodyDef = BodyDefinition.new(BodyType.STATIC_BODY)
        var playerShape = PhysicsCircleShape.new(0.25)
        PhysicsBody.create(_entity, bodyDef, playerShape)
    }
    
    update() {

    }
    
    onDestroy() {
    }
    
    onEvent(type, data) {
        
    }
    
    isInteractable() {
        return true
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
