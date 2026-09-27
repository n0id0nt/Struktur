// UI/Inventory/InventoryUI.wren
// The inventory screen's view: background panel + grid of focusable item tiles, and a focused-item
// detail panel (name + big icon). items is the raw item-name list States/InventoryState.wren
// already fetched from Inventory.getItems() - this view never reads that model directly.
// onItemFocused is called with the item name whenever a tile gains focus, so the State can react
// (playing a focus sound) without this view needing to know about audio.
import "resourceManager" for Font, Texture
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application
import "math" for Vec2, Vec4
import "Colors" for BLANK, WHITE
import "Localization/DisplayNames" for DisplayNames

class InventoryUI {
    construct new(items, onItemFocused) {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 120)
        var inventoryBackgroundPanelTexture = Texture.load("Tiles/InventoryBackgroundPanel.png")
        var focusedItemBackgroundPanelTexture = Texture.load("Tiles/FocusedItemBackgroundPanel.png")

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
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

        _focusedItemNameLabel = UILabel.new(Vec2.new(0, 20), Vec2.new(0.5, 0.55), "", 30.0)
        _focusedItemNameLabel.setTextColor(WHITE)
        _focusedItemNameLabel.setBoundingBoxToText()
        _focusedItemNameLabel.setAnchorPoint(Vec2.new(0.5, 0))
        _focusedItemNameLabel.setFont(font)
        font.unload()
        focusedBackgroundPanel.addChild(_focusedItemNameLabel)

        _focusedItemPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.25), Vec2.new(250, 250), Vec2.new(0, 0))
        _focusedItemPanel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _focusedItemPanel.setBorderColor(BLANK)
        focusedBackgroundPanel.addChild(_focusedItemPanel)

        var index = 0
        var curX = 25
        var curY = 35
        for (item in items) {
            var tile = UIPanel.new(Vec2.new(curX, curY), Vec2.new(0, 0), Vec2.new(64, 64), Vec2.new(0, 0))
            inventoryBackgroundPanel.addChild(tile)
            tile.setBackgroundColor(BLANK)
            tile.setBorderColor(BLANK)
            tile.setFocusable(true)

            var texture = Texture.load(iconPathFor_(item))
            tile.setBackgroundTexture(texture)

            tile.setOnFocus { |sender|
                _focusedItemNameLabel.setText(DisplayNames.itemDisplay(item))
                _focusedItemNameLabel.setBoundingBoxToText()
                _focusedItemPanel.setBackgroundTexture(texture)
                onItemFocused.call(item)
            }

            if (index == 0) {
                UIManager.setFocus(tile)
            }

            curX = curX + 90
            index = index + 1
            if (index % 4 == 0) {
                curX = 25
                curY = curY + 90
            }
        }
    }

    // Receipt/note items share a generic icon rather than needing per-item art.
    iconPathFor_(item) {
        if (item.endsWith(" Note") || item.endsWith(" Recipt")) {
            return "Tiles/Items/Recipt.png"
        }
        return "Tiles/Items/%(item).png"
    }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
    }
}
