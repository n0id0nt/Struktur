// states/InventoryState.wren
// The inventory screen. Fetches the item list from the Inventory model, owns the menu music/focus
// sound, and reacts to the "close inventory" input. UI/Inventory/InventoryUI.wren is the pure view
// this owns and refreshes. Game time is paused while it's open - the world freezes behind the screen.
import "app" for Time
import "resourceManager" for Music, Sound
import "input" for Input
import "States/BaseState" for BaseState
import "Inventory/Inventory" for Inventory
import "UI/Inventory/InventoryUI" for InventoryUI
import "debug" for Debug

class InventoryState is BaseState {
    construct new() {
        super()
        name = "InventoryState"
        _view = null
        _menuMusic = null
        _itemFocusSound = null
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        Debug.info("Entering Inventory")
        Time.setTimeScale(0)

        _menuMusic = Music.load("Sounds/menuMusic.wav")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _itemFocusSound = Sound.load("Sounds/scroll.wav")

        var items = Inventory.getItems()
        _view = InventoryUI.new(items, Fn.new { |item| _itemFocusSound.play() })
    }

    update(stateManager) {
        if (Input.isInputJustReleased("Inventory")) {
            stateManager.clearCurrentState()
        }
    }

    exit() {
        super.exit()
        Debug.info("Unloading Inventory...")
        Time.setTimeScale(1)

        if (_view != null) {
            _view.teardown()
            _view = null
        }

        _menuMusic.stop()
        _menuMusic.unload()
        _menuMusic = null
        _itemFocusSound.unload()
        _itemFocusSound = null
        Debug.info("Inventory unloaded")
    }
}
