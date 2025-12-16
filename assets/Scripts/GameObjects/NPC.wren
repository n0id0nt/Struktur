import "game" for Transform, Vec2, Vec3, Vec4, PhysicsBody, SpriteAnimation, Math, GameObject, Sprite, Script, SpriteAnimationDefinition, ResourceManager, Camera, BodyDefinition, PhysicsCircleShape

var WHITE = Vec4.new(255, 255, 255, 255)

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

    construct new(entity, name) {
        _entity = entity
        _name = name
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
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    create(entity) {
        var texture = ResourceManager.getTextureResource("assets/Tiles/NPCs.png")
        var ncpData = _spriteDataMap[_name]
        Sprite.create(_entity, texture, WHITE, npcData.getOffset(), 9, 1, false, npcData.getSpriteIndex(), 2)
        var bodyDef = BodyDefinition.createStaticBody()
        var playerShape = PhysicsCircleShape.new(0.25)
        var physicsBody = PhysicsBody.create(entity, bodyDef, playerShape)
        physicsBody.syncFromPhysics = true
        physicsBody.syncToPhysics = true
        var shader = ResourceManager.getShaderResource(null, "assets/Shaders/SoulEffect_100.fs")
        var shaderComponent = Shader.create(_entity, shader)
        shaderComponent.setUniform("soulColor", Vec3.new(0.3, 0.7, 1.0))
        shaderComponent.setUniform("glowIntensity", 0.05)
        shaderComponent.setUniform("rippleSpeed", 1.0)
        shaderComponent.setUniform("rippleFrequency", 15.0)
        shaderComponent.setUniform("waveAmplitude", 0.05)
        shaderComponent.setUniform("waveFrequency", 8.0)
        shaderComponent.setUniform("waveSpeed", 3.0)
        shaderComponent.setUniform("waveDirection", Vec2.new(1.0, 0.3))
        shaderComponent.setUniform("scanlineIntensity", 0.1)
        shaderComponent.setUniform("chromaticAberration", 0.005)
        shaderComponent.setUniform("glitchFrequency", 0.05)
        shaderComponent.setUniform("holographicShift", 10.0)
    }
    
    update(dt) {

    }
    
    onDestroy() {
    }
    
    onEvent(event) {
        
    }
}
