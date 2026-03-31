// states/InventoryState.wren
// Main gameplay state - handles player movement, interaction checks
// This is the default state when playing the game

import "resourceManager" for Font, Texture, Music, Sound
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application
import "math" for Vec2, Vec3, Vec4
import "input" for Input

import "States/BaseState" for BaseState
import "Colors" for BLANK, WHITE
import "Inventory" for Inventory

class InventoryState is BaseState {
    construct new() {
        super()
        name = "InventoryState"
        _screenPanel = null
        _menuMusic = null
        _itemFocusSound = null
    }
    
    enter(stateManager, params) {
        super.enter(stateManager, params)

        System.print("Entering Inventory")
        
        _menuMusic = Music.load("Sounds/menuMusic.wav")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _itemFocusSound = Sound.load("Sounds/scroll.wav")
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 120)
        var inventoryBackgroundPanelTexture = Texture.load("Tiles/InventoryBackgroundPanel.png")
        var focusedItemBackgroundPanelTexture = Texture.load("Tiles/FocusedItemBackgroundPanel.png")

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(Vec4.new(0, 0, 0, 70))
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var inventoryBackgroundPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0.3, 0.5), Vec2.new(394, 500), Vec2.new(0, 0))
        inventoryBackgroundPanel.setAnchorPoint(Vec2.new(0.5, 0.5))
        inventoryBackgroundPanel.setBorderColor(BLANK)
        inventoryBackgroundPanel.setBackgroundTexture(inventoryBackgroundPanelTexture)
        inventoryBackgroundPanelTexture.unload()
        _screenPanel.addChild(inventoryBackgroundPanel)

        var focusedBackgroundPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0.7, 0.5), Vec2.new(400, 500), Vec2.new(0, 0))
        focusedBackgroundPanel.setAnchorPoint(Vec2.new(0.5, 0.5))
        focusedBackgroundPanel.setBorderColor(BLANK)
        focusedBackgroundPanel.setBackgroundTexture(focusedItemBackgroundPanelTexture)
        focusedItemBackgroundPanelTexture.unload()
        _screenPanel.addChild(focusedBackgroundPanel)

        var inventoryFocusedItemNameLabel = UILabel.new(Vec2.new(0, 20), Vec2.new(0.5, 0.55), "No Items\nTest\n\n  Test two", 30.0)
        inventoryFocusedItemNameLabel.setTextColor(WHITE)
        inventoryFocusedItemNameLabel.setBoundingBoxToText()
        inventoryFocusedItemNameLabel.setAnchorPoint(Vec2.new(0.5, 0))
        inventoryFocusedItemNameLabel.setFont(font)
        font.unload()
        focusedBackgroundPanel.addChild(inventoryFocusedItemNameLabel)

        var testPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0.0, 0.0), Vec2.new(0, 0), Vec2.new(1, 1))
        testPanel.setAnchorPoint(Vec2.new(0, 0))
        testPanel.setBorderColor(Vec4.new(0, 0, 0, 100))
        testPanel.setBackgroundColor(Vec4.new(0, 0, 0, 100))
        inventoryFocusedItemNameLabel.addChild(testPanel)

        var focusedItemPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.25), Vec2.new(250, 250), Vec2.new(0, 0))
        focusedItemPanel.setAnchorPoint(Vec2.new(0.5, 0.5))
        focusedItemPanel.setBorderColor(BLANK)
        focusedBackgroundPanel.addChild(focusedItemPanel)

        var index = 0
        var row = 0
        var column = 0
        var curX = 25
        var curY = 35
        var items = Inventory.getItems()
        for (item in items) {
            var inventoryItemPanel = UIPanel.new(Vec2.new(curX, curY), Vec2.new(0, 0), Vec2.new(64, 64), Vec2.new(0, 0))
            inventoryBackgroundPanel.addChild(inventoryItemPanel)
            inventoryItemPanel.setBackgroundColor(BLANK)
            inventoryItemPanel.setBorderColor(BLANK)
            inventoryItemPanel.setFocusable(true)

            var texture = null
            var suffix1 = " Note"
            var suffix2 = " Recipt"
            if (item.endsWith(suffix1) || item.endsWith(suffix2)) {
                texture = Texture.load("Tiles/Items/Recipt.png")
            } else {
                texture = Texture.load("Tiles/Items/%(item).png")
            }

            inventoryItemPanel.setBackgroundTexture(texture)

            inventoryItemPanel.setOnFocus { |sender|
                inventoryFocusedItemNameLabel.setText(item)
                inventoryFocusedItemNameLabel.setBoundingBoxToText()
                focusedItemPanel.setBackgroundTexture(texture)
                _itemFocusSound.play()
            }

            curX = curX + 90

            if (index == 0) {
                UIManager.setFocus(inventoryItemPanel)
            }
            index = index + 1
            if (index % 4 == 0) {
                curX = 25
                curY = curY + 90
            }
        }
    }
    
    update(stateManager) {
        var inventoryInteract = Input.isInputJustReleased("Inventory")

        if (inventoryInteract) {
            stateManager.clearCurrentState()
            return
        }
    }
    
    exit() {
        super.exit()
        
        System.print("Unloading Inventory...")
        
        if (_screenPanel) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }

        _menuMusic.stop()
        _menuMusic.unload()
        _menuMusic = null
        _itemFocusSound.unload()
        _itemFocusSound = null
        System.print("Inventory unloaded")
    }
}
