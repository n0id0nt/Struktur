import "gameObjectComponents" for PhysicsBody, Shader as ShaderComponent
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Shader as ShaderResource
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "debug" for Debug
import "Sprites/ClaireSprite" for ClaireSprite
import "Sprites/GoblinVikingSprite" for GoblinVikingSprite
import "Sprites/PeasantGirlSprite" for PeasantGirlSprite
import "Sprites/PinkHairGirlSprite" for PinkHairGirlSprite
import "Sprites/WrestlerSprite" for WrestlerSprite
import "Sprites/YellowPantsGirlSprite" for YellowPantsGirlSprite

class NPCData {
    // sprite: the generated Sprites/<Name>Sprite class this NPC wears (made by 3dModelsToSprites/build_sprites.bat).
    construct new(sprite) {
        _sprite = sprite
    }

    sprite { _sprite }

    // Adds the NPC's sprite and every <direction><Animation>Animation clip to `entity`, idling facing down.
    createSprite(entity) { _sprite.create(entity) }

    // Atlas size in pixels - the soul shader's offsets are in texture UVs, so NPC.start scales them by this to
    // keep them a fixed number of pixels on any sheet.
    textureWidth { _sprite.data["columns"] * _sprite.data["cellSize"][0] }
    rows { _sprite.data["rows"] }

    // The sprite for the NPC called `name`, or null. Six models cover the twelve NPCs, so some share a look.
    // Shared by the standing NPC (this file), its party follower (GameObjects/PartyMember.wren) and its battle
    // look (Combat/Config/BattleCritter.wren).
    static forName(name) {
        var table = {
            // Overworld NPCs (States/ExperimentState.wren spawns them; Dialogue/OverworldDialogue.json).
            "Villager": NPCData.new(PeasantGirlSprite),
            "Traveler": NPCData.new(ClaireSprite),
            "Duelist": NPCData.new(YellowPantsGirlSprite),
            // Memory Palace NPCs (Levels/MemoryPalace.ldtk).
            "Scholar": NPCData.new(ClaireSprite),
            "Gardener": NPCData.new(YellowPantsGirlSprite),
            "Cook": NPCData.new(PeasantGirlSprite),
            "Inventor": NPCData.new(YellowPantsGirlSprite),
            "Dreamer": NPCData.new(PinkHairGirlSprite),
            "Astronomer": NPCData.new(PinkHairGirlSprite),
            "Merchant": NPCData.new(WrestlerSprite),
            "Guardian": NPCData.new(GoblinVikingSprite),
            "Cordelia": NPCData.new(PinkHairGirlSprite)
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

    // Whether beating this NPC in a duel recruits them into the party (see CombatResolver.win + Party.wren).
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
        var physicsBody = PhysicsBody.create(_entity, bodyDef, playerShape)
        var shader = ShaderResource.load(null, "Shaders/SoulEffect_100.fs")
        var shaderComponent = ShaderComponent.create(_entity, shader)
        shader.unload()
        shaderComponent.setVec3Uniform("soulColor", Vec3.new(0.3, 0.7, 1.0))
        shaderComponent.setFloatUniform("glowIntensity", 0.05)
        shaderComponent.setFloatUniform("rippleSpeed", 1.0)
        shaderComponent.setFloatUniform("rippleFrequency", 15.0)
        shaderComponent.setFloatUniform("waveAmplitude", 0.05)
        shaderComponent.setFloatUniform("waveFrequency", 8.0)
        shaderComponent.setFloatUniform("waveSpeed", 3.0)
        shaderComponent.setVec3Uniform("waveDirection", Vec2.new(1.0, 0.3))
        shaderComponent.setFloatUniform("scanlineIntensity", 0.1)
        // ~1.5px colour split and one holographic band per figure, whatever the atlas size.
        shaderComponent.setFloatUniform("chromaticAberration", 1.5 / npcData.textureWidth)
        shaderComponent.setFloatUniform("glitchFrequency", 0.05)
        shaderComponent.setFloatUniform("holographicShift", 10.0 * npcData.rows)
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

    getEntity() {
        return _entity
    }

    getInteractId() {
        return "Entrance Door"
    }
}
