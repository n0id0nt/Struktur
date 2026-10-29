// UI/BattleTest/BattleReadoutUI.wren
// The battle test's live overlay while a test battle runs (States/BattleTestState.wren): a "Fighters" panel top-left
// with each fighter's exact HP / stamina and what it's charging, and a "Rolls" panel top-right with the Arbiter's
// most recent rolls (Combat/Arbiter.recentRolls). Both sit in the gaps CombatUI leaves - under the enemy stacks,
// above the party stacks and the move menu. Pure view: the state pushes the text every frame.
import "math" for Vec2, Vec4
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application
import "Colors" for BLANK, WHITE, LIGHTGRAY

var PANEL_TOP    = 140
var LINE_HEIGHT  = 18
var TEXT_SIZE    = 14.0
var PADDING      = 8
var FIGHTERS_W   = 400
var ROLLS_W      = 470
var PANEL_TINT   = Vec4.new(0, 0, 0, 150)

class BattleReadoutUI {
    // fighterLines / rollLines: how many lines each panel holds.
    construct new(fighterLines, rollLines) {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)
        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _root.setBackgroundColor(BLANK)
        _root.setBorderColor(BLANK)
        UIManager.addUIElement(_root)

        _fighterLabels = panel_(font, "Fighters", Vec2.new(16, PANEL_TOP), Vec2.new(0, 0), Vec2.new(0, 0), FIGHTERS_W,
                                fighterLines)
        _rollLabels = panel_(font, "Rolls (newest last)", Vec2.new(-16, PANEL_TOP), Vec2.new(1, 0), Vec2.new(1, 0),
                             ROLLS_W, rollLines)
        font.unload()
        _shown = {}   // label key -> text currently on it, so unchanged lines aren't re-laid-out every frame
    }

    // lines: [[text, isParty], ...] - one per fighter, party first.
    setFighters(lines) {
        for (i in 0..._fighterLabels.count) {
            var label = _fighterLabels[i]
            if (show_(label, "f%(i)", i < lines.count ? lines[i][0] : "")) {
                label.setTextColor(i < lines.count && lines[i][1] ? WHITE : LIGHTGRAY)
            }
        }
    }

    // rolls: short roll lines, oldest first - the newest fill the bottom of the panel.
    setRolls(rolls) {
        var n = _rollLabels.count
        var start = rolls.count > n ? rolls.count - n : 0
        for (i in 0...n) {
            var r = start + i
            show_(_rollLabels[i], "r%(i)", r < rolls.count ? rolls[r] : "")
        }
    }

    // Puts `text` on `label` (keyed `key`) if it isn't already there; true when it changed.
    show_(label, key, text) {
        if (_shown[key] == text) {
            return false
        }
        _shown[key] = text
        label.setText(text)
        label.setBoundingBoxToText()
        return true
    }

    teardown() {
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
    }

    // A tinted panel with a title and `lines` empty labels; returns the labels.
    panel_(font, title, absPos, relPos, anchor, width, lines) {
        var panel = UIPanel.new(absPos, relPos, Vec2.new(width, PADDING * 2 + LINE_HEIGHT * (lines + 1)), Vec2.new(0, 0))
        panel.setAnchorPoint(anchor)
        panel.setBackgroundColor(PANEL_TINT)
        panel.setBorderColor(BLANK)
        _root.addChild(panel)

        label_(panel, font, title, PADDING, WHITE)
        var labels = []
        for (i in 0...lines) {
            labels.add(label_(panel, font, "", PADDING + LINE_HEIGHT * (i + 1), LIGHTGRAY))
        }
        return labels
    }

    label_(panel, font, text, y, color) {
        var label = UILabel.new(Vec2.new(PADDING, y), Vec2.new(0, 0), text, TEXT_SIZE)
        label.setFont(font)
        label.setTextColor(color)
        label.setBoundingBoxToText()
        panel.addChild(label)
        return label
    }
}
