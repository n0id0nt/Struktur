// states/GameOverState.wren
// Main gameplay state - handles player movement, interaction checks
// This is the default state when playing the game

import "States/BaseState" for BaseState
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application

var WHITE = Vec4.new(255, 255, 255, 255)
var BLANK = Vec4.new(0, 0, 0, 0)

class GameOverState is BaseState {
    construct new() {
        super()
        _name = "GameOverState"

        _screenPanel = null
    }
    
    enter() {
        super.enter()
        
        var font = Font.load("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_120")

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(Vec4.new(0, 0, 0, 200))
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var textBackgroundPanel = UILabel.new(Vec2.new(-20, -20), Vec2.new(0.5, 0.5), "Game Complete", 60)
        textBackgroundPanel.setAnchorPoint(Vec2.new(0.5, 0.5))
        textBackgroundPanel.setTextColor(WHITE)
        textBackgroundPanel.setFont(font)
        _screenPanel.addChild(textBackgroundPanel)
    }
    
    update() {

    }
    
    exit() {
        super.exit()
        
        UIManager.removeUIElement(_screenPanel)
    }
}
