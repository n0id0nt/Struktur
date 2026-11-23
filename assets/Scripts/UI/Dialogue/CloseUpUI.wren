// UI/Dialogue/CloseUpUI.wren
// The close-up view's picture (Dialogue/CloseUp.wren): framed and centred in the space above the dialogue box, at the
// largest whole-number scale that fits there. Created by States/InteractState.wren before its dialogue box, so the box
// draws over it; stays up while a staged node hides the box (a memory plays with no line of text).
import "ui" for UIManager, UIPanel, UITexture
import "app" for Application
import "math" for Vec2
import "Util/Colors" for BLANK
import "UI/Theme/UIKit" for UIKit

var BOX_SPACE = 260          // the dialogue box and its margin at the bottom of the screen (UI/Dialogue/DialogueBoxUI.wren)
var MARGIN = 24              // between the frame and the top of the screen / the box
var FRAME_PAD = 10           // the "plate" frame's rim around the picture

class CloseUpUI {
    construct new() {
        UIKit.open()
        _panel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _panel.setBackgroundColor(BLANK)
        _panel.setBorderColor(BLANK)
        UIManager.addUIElement(_panel)
        _frame = UIKit.frame(_panel, "plate", Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(16, 16), Vec2.new(0, 0), 0)
        _picture = UITexture.new(Vec2.new(FRAME_PAD, FRAME_PAD), Vec2.new(0, 0), Vec2.new(1, 1), Vec2.new(0, 0))
        _picture.setZIndex(1)
        _frame.addChild(_picture)
        _path = null
        _panel.setVisible(false)
    }

    // Shows the picture at `path` (null hides it).
    show(path) {
        if (path == null) {
            _path = null
            _panel.setVisible(false)
            return
        }
        if (path != _path) {
            _path = path
            var texture = UIKit.texture(path)
            var roomWidth = Application.gameWidth - 2 * MARGIN - 2 * FRAME_PAD
            var roomHeight = Application.gameHeight - BOX_SPACE - 2 * MARGIN - 2 * FRAME_PAD
            var scale = (roomWidth / texture.width).floor.min((roomHeight / texture.height).floor).max(1)
            var width = texture.width * scale
            var height = texture.height * scale
            _picture.setTexture(texture)
            _picture.setSize(Vec2.new(width, height), Vec2.new(0, 0))
            var frameWidth = width + 2 * FRAME_PAD
            var frameHeight = height + 2 * FRAME_PAD
            var top = MARGIN + ((roomHeight + 2 * FRAME_PAD - frameHeight) / 2).floor
            _frame.setSize(Vec2.new(frameWidth, frameHeight), Vec2.new(0, 0))
            _frame.setPosition(Vec2.new(((Application.gameWidth - frameWidth) / 2).floor, top), Vec2.new(0, 0))
        }
        _panel.setVisible(true)
    }

    teardown() {
        if (_panel != null) {
            UIManager.removeUIElement(_panel)
            _panel = null
            _frame = null
            _picture = null
            UIKit.close()
        }
    }
}
