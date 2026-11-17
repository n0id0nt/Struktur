// Combat/Config/BattleTestRoster.wren
// What the battle test (States/BattleTestState.wren) can put on either side: every battle def by its menu label.
// Humanoids carry real gear and human moves, so the test's pickers can re-equip them and they can fight on your
// side; monsters and props bring their own natural weapon and combo menu and only fight on the other side. Add a
// fighter here and it shows up in the setup menu's pickers.
import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Config/BattlePlayer" for BattlePlayer
import "Combat/Boss/Warden" for Warden

class BattleTestRoster {
    // [label, def, humanoid] - a def is BattlePlayer (a class with static getters) or a BattleCritter.
    static fighters {
        return [
            ["You", BattlePlayer, true],
            ["Kira", BattleCritter.kira, true],
            ["Thomas", BattleCritter.thomas, true],
            ["Villager", BattleCritter.villager, true],
            ["Duelist", BattleCritter.duelist, true],
            ["Master Finn", BattleCritter.finn, true],
            ["Desperate Raider", BattleCritter.raider, true],
            ["Training Dummy", BattleCritter.trainingDummy, false],
            ["Practice Stump", BattleCritter.practiceStump, false],
            ["Swing-post", BattleCritter.swingPost, false],
            ["Chicken", BattleCritter.chicken, false],
            ["Chinlin", BattleCritter.chinlin, false],
            ["Starving Thornwolf", BattleCritter.starvingThornwolf, false],
            ["Corrupted Thornwolf", BattleCritter.thornwolf, false],
            ["Twisted Sprite", BattleCritter.twistedSprite, false],
            ["Runic Construct", BattleCritter.runicConstruct(110), false],
            ["Stone Guardian", BattleCritter.stoneGuardian, false],
            ["The Warden (boss)", Warden.def, false]
        ]
    }
}
