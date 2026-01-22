import "gameObjectComponents" for Sprite, PhysicsBody, Shader as ShaderComponent, WorldTransform
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Texture, Shader as ShaderResource
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "Colors" for WHITE

class NPCData {
    construct new(spriteIndex, xOffset, yOffset) {
        _spriteIndex = spriteIndex
        _xOffset = xOffset
        _yOffset = yOffset
    }

    getOffset() {
        return Vec2.new(_xOffset, _yOffset)
    }

    getSpriteIndex() {
        return _spriteIndex
    }
}

class NPC {

    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _spriteDataMap = {
            "Scholar": NPCData.new(0, 48, 64),
            "Gardener": NPCData.new(6, 48, 64),
            "Cook": NPCData.new(4, 48, 64),
            "Inventor": NPCData.new(2, 48, 64),
            "Dreamer": NPCData.new(5, 60, 48),
            "Astronomer": NPCData.new(8, 48, 64),
            "Merchant": NPCData.new(1, 48, 64),
            "Guardian": NPCData.new(7, 48, 64),
            "Cordelia": NPCData.new(3, 48, 64),
        }
    }
    
    name { _name }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        var npcData = _spriteDataMap[_name]
        var texture = Texture.load("assets/Tiles/NPCs.png")
        Sprite.create(_entity, texture, WHITE, npcData.getOffset(), 9, 1, false, npcData.getSpriteIndex(), WorldTransform.getPosition(_entity).y)
        texture.unload()
        var bodyDef = BodyDefinition.new(BodyType.STATIC_BODY)
        var playerShape = PhysicsCircleShape.new(0.25)
        var physicsBody = PhysicsBody.create(_entity, bodyDef, playerShape)
        physicsBody.syncFromPhysics = true
        physicsBody.syncToPhysics = true
        var shader = ShaderResource.load(null, "assets/Shaders/SoulEffect_100.fs")
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
        shaderComponent.setFloatUniform("chromaticAberration", 0.005)
        shaderComponent.setFloatUniform("glitchFrequency", 0.05)
        shaderComponent.setFloatUniform("holographicShift", 10.0)
    }
    
    update() {

    }
    
    onDestroy() {
    }
    
    onEvent(event) {
        
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
