// states/InventoryState.wren
// Main gameplay state - handles player movement, interaction checks
// This is the default state when playing the game

import "States/BaseState" for BaseState
import "resourceManager" for Font, Texture
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application, Inventory
import "math" for Vec2, Vec3, Vec4
import "input" for Input

var WHITE = Vec4.new(255, 255, 255, 255)
var BLANK = Vec4.new(0, 0, 0, 0)

class InventoryState is BaseState {
    construct new() {
        super()
        _name = "InventoryState"
        _screenPanel = null
    }
    
    enter(stateManager, params) {
        super.enter(stateManager, params)

        System.print("Entering Inventory")
        
        var font = Font.load("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_120")
        var inventoryBackgroundPanelTexture = Texture.load("assets/Tiles/InventoryBackgroundPanel.png")
        var focusedItemBackgroundPanelTexture = Texture.load("assets/Tiles/FocusedItemBackgroundPanel.png")

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

        var inventoryFocusedItemNameLabel = UILabel.new(Vec2.new(0, 20), Vec2.new(0.5, 0.75), "No Items", 30.0)
        inventoryFocusedItemNameLabel.setTextColor(WHITE)
        inventoryFocusedItemNameLabel.setAnchorPoint(Vec2.new(0.5, 0))
        inventoryFocusedItemNameLabel.setFont(font)
        font.unload()
        focusedBackgroundPanel.addChild(inventoryFocusedItemNameLabel)

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
                texture = Texture.load("assets/Tiles/Items/Recipt.png")
            } else {
                texture = Texture.load("assets/Tiles/Items/%(item).png")
            }

            inventoryItemPanel.setBackgroundTexture(texture)

            inventoryItemPanel.setOnFocus { |sender|
                inventoryFocusedItemNameLabel.setText(item)
                focusedItemPanel.setBackgroundTexture(texture)
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

        System.print("Inventory unloaded")
    }
}
