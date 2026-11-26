# Missing ride-world art

The procedural ride world ([RideWorld.md](RideWorld.md)) needs 33 art roles, listed in
`assets/World/manifest.json`. 29 of them have no real file yet, so the game draws a placeholder in code for each
(`RideWorld::GeneratePlaceholder`, named `placeholder_<role>` in logs). The game runs fine without them.

**To replace a placeholder:** save a PNG at the expected path below, or edit the role's `file` in the manifest to
point at an existing asset. Nothing else changes; sizes, frame layout and anchors are read from the manifest.

- **Size:** sizes are per frame. Frames (variants or crank positions) run left to right and rows top to bottom.
- **Alpha:** straight (not premultiplied).
- **Sampling:** nearest-neighbour, so pixel art stays crisp.
- **Props:** drawn as camera-facing billboards with the base at the bottom centre, except guardrails, buildings and
  the banner, which face along / across the road.
- **Placement:** world size comes from the biome's `height` range, and the width follows the frame's aspect ratio.
- **Rider frames per revolution:** 8 is the minimum (45 degree steps), 12 is recommended (and what the placeholder
  uses), and more is smoother. The legs can't strobe backwards at any real cadence (that needs more than half a
  revolution per displayed frame, over 1800 rpm at 60 fps), but a spoked side-on wheel can. See RideWorld.md.
- **Already mapped to existing assets** (not listed below): `rider_shadow` -> `Actors/Character/Shadow.png`,
  `sky_clouds` -> `FX/Particle/Clouds.png`, `particle_rain` -> `FX/Particle/Rain.png`, `particle_snow` ->
  `FX/Particle/Snow.png`.
- **Optional:** `rider_sheet` is only used if `assets/World/Rider/rider.json` switches to `"kind": "sheet"`. With the
  four rider layers in place it isn't needed.

This list is also logged once at startup, and `Struktur.exe --list-missing-assets` prints it (this exact table) and
quits.

INFO: [DialogueManager.cpp:12 in Struktur::Dialogue::DialogueManager::DialogueManager] DialogueManager initialized
INFO: [Input.cpp:555 in Struktur::Input::Input::RefreshGamepads] Gamepad 'Xbox Gaming Device' connected
INFO: [Input.cpp:592 in Struktur::Input::Input::SetActiveGamepad] Active gamepad is now 'Xbox Gaming Device'
INFO: [Mixer.cpp:35 in Struktur::Audio::Mixer::Mixer] SDL3_mixer mixer device created
INFO: [WrenScriptEngine.cpp:30 in Struktur::Wren::WrenScriptEngine::Initialise] Wren VM initialised
INFO: [WrenScriptEngine.cpp:31 in Struktur::Wren::WrenScriptEngine::Initialise] Registered 818 method bindings
INFO: [WrenScriptEngine.cpp:32 in Struktur::Wren::WrenScriptEngine::Initialise] Registered 53 class bindings
INFO: [WrenScriptEngine.cpp:33 in Struktur::Wren::WrenScriptEngine::Initialise] Registered 9 enum bindings
INFO: [WrenScriptEngine.cpp:34 in Struktur::Wren::WrenScriptEngine::Initialise] Registered 1 constant bindings
INFO: [WrenStateManager.cpp:23 in Struktur::Wren::WrenStateManager::Initialise] [WrenStateManager] Initializing Main.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: app from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/app.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: input from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/input.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: ui from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/ui.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: localization from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/localization.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: flags from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/flags.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: fileSystem from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/fileSystem.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: physics from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/physics.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/StateManager from Scripts/States/StateManager.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: debug from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/debug.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: serialisation from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/serialisation.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/GameWorldState from Scripts/States/GameWorldState.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: gameObject from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/gameObject.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: gameObjectComponents from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/gameObjectComponents.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: math from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/math.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: resourceManager from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/resourceManager.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/AreaMusic from Scripts/World/AreaMusic.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/BaseState from Scripts/States/BaseState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/PlayState from Scripts/States/PlayState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/InventoryState from Scripts/States/InventoryState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Inventory/Inventory from Scripts/Inventory/Inventory.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Inventory/ItemDatabase from Scripts/Inventory/ItemDatabase.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Party/Party from Scripts/Party/Party.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Party/Member from Scripts/Party/Member.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Party/PartyTrail from Scripts/Party/PartyTrail.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/BattleCritter from Scripts/Combat/Config/BattleCritter.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Combatant from Scripts/Combat/Combatant.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Stats from Scripts/Combat/Stats.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Gear from Scripts/Combat/Gear.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/GearTypes from Scripts/Combat/Config/GearTypes.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Move from Scripts/Combat/Move.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/EnemyAI from Scripts/Combat/EnemyAI.wren
WARNING: [WrenScriptEngine.cpp:255 in Struktur::Wren::WrenScriptEngine::OnBindForeignMethod] Wren binding not found: random.Random.seed_() (static=0)
WARNING: [WrenScriptEngine.cpp:255 in Struktur::Wren::WrenScriptEngine::OnBindForeignMethod] Wren binding not found: random.Random.seed_(_) (static=0)
WARNING: [WrenScriptEngine.cpp:255 in Struktur::Wren::WrenScriptEngine::OnBindForeignMethod] Wren binding not found: random.Random.seed_(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_) (static=0)
WARNING: [WrenScriptEngine.cpp:255 in Struktur::Wren::WrenScriptEngine::OnBindForeignMethod] Wren binding not found: random.Random.float() (static=0)
WARNING: [WrenScriptEngine.cpp:255 in Struktur::Wren::WrenScriptEngine::OnBindForeignMethod] Wren binding not found: random.Random.int() (static=0)
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Chain from Scripts/Combat/Chain.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/BattlePlan from Scripts/Combat/BattlePlan.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/Moves from Scripts/Combat/Config/Moves.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/SheetAnims from Scripts/Combat/Config/SheetAnims.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/NPC from Scripts/GameObjects/NPC.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Util/Shadow from Scripts/Util/Shadow.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: renderer from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/renderer.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Util/CharacterCollider from Scripts/Util/CharacterCollider.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/StoryGates from Scripts/World/StoryGates.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/FinnSprite from Scripts/Sprites/FinnSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/SpriteSheet from Scripts/Sprites/SpriteSheet.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: animation from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/animation.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Util/Colors from Scripts/Util/Colors.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/FinnBattleSprite from Scripts/Sprites/FinnBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/ClaireSprite from Scripts/Sprites/ClaireSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/ClaireBattleSprite from Scripts/Sprites/ClaireBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/GoblinVikingSprite from Scripts/Sprites/GoblinVikingSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/GoblinVikingBattleSprite from Scripts/Sprites/GoblinVikingBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/PeasantGirlSprite from Scripts/Sprites/PeasantGirlSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/PeasantGirlBattleSprite from Scripts/Sprites/PeasantGirlBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/PinkHairGirlSprite from Scripts/Sprites/PinkHairGirlSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/PinkHairGirlBattleSprite from Scripts/Sprites/PinkHairGirlBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/WrestlerSprite from Scripts/Sprites/WrestlerSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/WrestlerBattleSprite from Scripts/Sprites/WrestlerBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/MutantBattleSprite from Scripts/Sprites/MutantBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/YellowPantsGirlSprite from Scripts/Sprites/YellowPantsGirlSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/YellowPantsGirlBattleSprite from Scripts/Sprites/YellowPantsGirlBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/YellowPantsGirlBattleWeaponSprite from Scripts/Sprites/YellowPantsGirlBattleWeaponSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Events/GameEvents from Scripts/Events/GameEvents.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: events from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/events.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/BattlePlayer from Scripts/Combat/Config/BattlePlayer.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/ExplorerBattleSprite from Scripts/Sprites/ExplorerBattleSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/ExplorerBattleWeaponSprite from Scripts/Sprites/ExplorerBattleWeaponSprite.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/MoveBook from Scripts/Combat/MoveBook.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Inventory/Equipment from Scripts/Inventory/Equipment.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Localization/DisplayNames from Scripts/Localization/DisplayNames.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Story/QuestLog from Scripts/Story/QuestLog.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/InventoryUI from Scripts/UI/Inventory/InventoryUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Theme/UIKit from Scripts/UI/Theme/UIKit.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Theme/AlertPopup from Scripts/UI/Theme/AlertPopup.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/InventoryTheme from Scripts/UI/Inventory/InventoryTheme.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/ItemGridPage from Scripts/UI/Inventory/ItemGridPage.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/MovesPage from Scripts/UI/Inventory/MovesPage.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/MoveCells from Scripts/UI/Inventory/MoveCells.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/PartyPage from Scripts/UI/Inventory/PartyPage.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/LevelsPage from Scripts/UI/Inventory/LevelsPage.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/QuestsPage from Scripts/UI/Inventory/QuestsPage.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Inventory/MapPage from Scripts/UI/Inventory/MapPage.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/MapLog from Scripts/World/MapLog.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/Rooms from Scripts/World/Rooms.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/InteractState from Scripts/States/InteractState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/DialogueInterpreter from Scripts/Dialogue/DialogueInterpreter.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: dialogue from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/dialogue.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/DialogueEntryPoints from Scripts/Dialogue/DialogueEntryPoints.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/ChoiceRecord from Scripts/Dialogue/ChoiceRecord.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Dialogue/DialogueBoxUI from Scripts/UI/Dialogue/DialogueBoxUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Dialogue/ChoiceListUI from Scripts/UI/Dialogue/ChoiceListUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Dialogue/CloseUpUI from Scripts/UI/Dialogue/CloseUpUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/Portraits from Scripts/Dialogue/Portraits.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/CloseUp from Scripts/Dialogue/CloseUp.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/StageDirector from Scripts/World/StageDirector.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/Npcs from Scripts/World/Npcs.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/Paths from Scripts/World/Paths.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/GameOverState from Scripts/States/GameOverState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/GameOver/GameOverUI from Scripts/UI/GameOver/GameOverUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Shared/ButtonUI from Scripts/UI/Shared/ButtonUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/CombatState from Scripts/States/CombatState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/BattleEngine from Scripts/Combat/BattleEngine.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Rules from Scripts/Combat/Rules.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/BattleStage from Scripts/Combat/BattleStage.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Battler from Scripts/Combat/Battler.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/MoveParticles from Scripts/Combat/Config/MoveParticles.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Util/SoulFx from Scripts/Util/SoulFx.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/PlanGhost from Scripts/Combat/PlanGhost.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/BattleProjectile from Scripts/Combat/BattleProjectile.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/BattleAudio from Scripts/Combat/BattleAudio.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/CombatOutcome from Scripts/Combat/CombatOutcome.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Defeated from Scripts/Combat/Defeated.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Encounter from Scripts/Combat/Encounter.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Boss/Warden from Scripts/Combat/Boss/Warden.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/BattleItems from Scripts/Combat/Config/BattleItems.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/Critter from Scripts/GameObjects/Critter.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/StateMachine from Scripts/GameObjects/StateMachine.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Combat/CombatUI from Scripts/UI/Combat/CombatUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Combat/CombatantUI from Scripts/UI/Combat/CombatantUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Combat/HealthBarUI from Scripts/UI/Combat/HealthBarUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Theme/KitBar from Scripts/UI/Theme/KitBar.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Combat/StaminaBarUI from Scripts/UI/Combat/StaminaBarUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Combat/TimelineUI from Scripts/UI/Combat/TimelineUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Combat/MoveIcons from Scripts/UI/Combat/MoveIcons.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Combat/MoveButtonUI from Scripts/UI/Combat/MoveButtonUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/SoulSight from Scripts/World/SoulSight.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/ImpactState from Scripts/States/ImpactState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Gameplay/FightPromptUI from Scripts/UI/Gameplay/FightPromptUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Gameplay/InteractPromptUI from Scripts/UI/Gameplay/InteractPromptUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Party/DuelRequest from Scripts/Party/DuelRequest.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Camera/RoomCamera from Scripts/Camera/RoomCamera.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/Signals from Scripts/World/Signals.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/PressurePlates from Scripts/World/PressurePlates.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/ImpactEvents from Scripts/World/ImpactEvents.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/Warps from Scripts/World/Warps.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/StoryTriggers from Scripts/World/StoryTriggers.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/SoulVeil from Scripts/World/SoulVeil.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Gameplay/SoulSightOverlay from Scripts/UI/Gameplay/SoulSightOverlay.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Gameplay/ScreenFade from Scripts/UI/Gameplay/ScreenFade.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/SpawnPoint from Scripts/World/SpawnPoint.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/PathWalkers from Scripts/World/PathWalkers.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Notifications/Notifications from Scripts/UI/Notifications/Notifications.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Story/ArcEnd from Scripts/Story/ArcEnd.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/MainMenuState from Scripts/States/MainMenuState.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: ride from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/ride.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/MainMenu/MainMenuUI from Scripts/UI/MainMenu/MainMenuUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/DevicePairingState from Scripts/States/DevicePairingState.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: fitness from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/fitness.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Pairing/DevicePairingUI from Scripts/UI/Pairing/DevicePairingUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Theme/KitButton from Scripts/UI/Theme/KitButton.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/RiderSelectState from Scripts/States/RiderSelectState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Rider/RiderSelectUI from Scripts/UI/Rider/RiderSelectUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Rider/RiderTheme from Scripts/UI/Rider/RiderTheme.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Rider/RiderProfiles from Scripts/Rider/RiderProfiles.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/RiderEditState from Scripts/States/RiderEditState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Rider/RiderEditUI from Scripts/UI/Rider/RiderEditUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/RideState from Scripts/States/RideState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Ride/RideHudUI from Scripts/UI/Ride/RideHudUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Ride/RideTheme from Scripts/UI/Ride/RideTheme.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Ride/RideFormat from Scripts/UI/Ride/RideFormat.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Ride/RideLayout from Scripts/UI/Ride/RideLayout.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/WorkoutBrowserState from Scripts/States/WorkoutBrowserState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Ride/WorkoutBrowserUI from Scripts/UI/Ride/WorkoutBrowserUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/RideSettingsState from Scripts/States/RideSettingsState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/SettingsState from Scripts/States/SettingsState.wren
INFO: [WrenScriptEngine.cpp:175 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: audio from C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/audio.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/Settings/SettingsUI from Scripts/UI/Settings/SettingsUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: States/BattleTestState from Scripts/States/BattleTestState.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Combat/Config/BattleTestRoster from Scripts/Combat/Config/BattleTestRoster.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: UI/BattleTest/BattleSetupUI from Scripts/UI/BattleTest/BattleSetupUI.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/DialogueLoader from Scripts/Dialogue/DialogueLoader.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/RegisteredFunctions/Commands from Scripts/Dialogue/RegisteredFunctions/Commands.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: World/StoryEvents from Scripts/World/StoryEvents.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/Door from Scripts/GameObjects/Door.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/RegisteredFunctions/Conditions from Scripts/Dialogue/RegisteredFunctions/Conditions.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/RegisteredFunctions/Operators from Scripts/Dialogue/RegisteredFunctions/Operators.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Dialogue/RegisteredFunctions/Variables from Scripts/Dialogue/RegisteredFunctions/Variables.wren
INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [StateManager] created
Call stack:
  [0] init new() in States/StateManager (line 16)
  [1]  in States/StateManager (line 17)
  [2] init new() in Main (line 80)
  [3]  in Main (line 87)
  [4] new() in <unknown> (line 0)

INFO: [WrenStateManager.cpp:80 in Struktur::Wren::WrenStateManager::Initialise] [WrenStateManager] Initialised successfully
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Chinlin.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/States/IdleMixin from Scripts/GameObjects/States/IdleMixin.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/States/WanderMixin from Scripts/GameObjects/States/WanderMixin.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/States/ChaseMixin from Scripts/GameObjects/States/ChaseMixin.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/Steering/Separation from Scripts/GameObjects/Steering/Separation.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Chinlin (Chinlin)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Chicken.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Chicken (Chicken)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Door.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Door (Door)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/DoorWorp.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/DoorWorp (DoorWorp)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/PartyMember.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Party/TrailFollow from Scripts/Party/TrailFollow.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Party/PartySeparation from Scripts/Party/PartySeparation.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Party/WarpRecovery from Scripts/Party/WarpRecovery.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/PartyMember (PartyMember)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Item.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Item (Item)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/NPC.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/NPC (NPC)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/PressurePlate.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/PressurePlate (PressurePlate)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Trigger.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Trigger (Trigger)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/PressurePad.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/PressurePlate from Scripts/GameObjects/PressurePlate.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/PressurePad (PressurePad)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Sign.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Sign (Sign)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/SoulFragment.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/SoulFragment (SoulFragment)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/ArrowShot.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/ArrowShot (ArrowShot)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/PerimeterWolf.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: GameObjects/Chinlin from Scripts/GameObjects/Chinlin.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/PerimeterWolf (PerimeterWolf)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Player.wren
INFO: [WrenScriptEngine.cpp:188 in Struktur::Wren::WrenScriptEngine::OnLoadModule] Loaded Wren module: Sprites/ExplorerSprite from Scripts/Sprites/ExplorerSprite.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Player (Player)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/ParticleEffect.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/ParticleEffect (ParticleEffect)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Room.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Room (Room)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Interactable.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Interactable (Interactable)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/RestSpot.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/RestSpot (RestSpot)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Level/Critter.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Level/Critter (Critter)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Spawn.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Spawn (Spawn)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Marker.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Marker (Marker)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/Path.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/Path (Path)
INFO: [WrenScriptComponentRegistry.cpp:54 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] [WrenScriptComponentRegistry] Initializing GameObjects/WayPoint.wren
INFO: [WrenScriptComponentRegistry.cpp:123 in Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents] Initialised Wren script: GameObjects/WayPoint (WayPoint)
INFO: [Window.cpp:21 in Struktur::Platform::Window::Initialise] Created SDL3 window (bgfx renders into it - no GL context requested).
INFO: [GraphicsDevice.cpp:63 in Struktur::Renderer::GraphicsDevice::Initialise] bgfx initialised, renderer: Direct3D 11
INFO: [RideService.cpp:168 in Struktur::RideIntegration::RideService::LoadWorldData] Ride world: 29 of 33 art roles use placeholders:
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing rider_frame: expected assets/World/Rider/rider_frame.png (64 x 96, PNG, RGBA (straight alpha), frames 1, anchor 32, 94) - using placeholder_rider_frame
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing rider_wheel: expected assets/World/Rider/rider_wheel.png (6 x 32, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_rider_wheel
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing rider_legs: expected assets/World/Rider/rider_legs.png (64 x 96 per frame (sheet 768 x 288), PNG, RGBA (straight alpha), frames 12 x 3 rows (12 per crank revolution), anchor 32, 94) - using placeholder_rider_legs
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing rider_torso: expected assets/World/Rider/rider_torso.png (64 x 96, PNG, RGBA (straight alpha), frames 1, anchor 32, 94) - using placeholder_rider_torso
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing rider_sheet: expected assets/World/Rider/rider_sheet.png (64 x 96 per frame (sheet 768 x 288), PNG, RGBA (straight alpha), frames 12 x 3 rows (12 per crank revolution), anchor 32, 94) - using placeholder_rider_sheet
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing tree_pine: expected assets/World/Props/tree_pine.png (64 x 128 per frame (sheet 192 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_tree_pine
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing tree_broadleaf: expected assets/World/Props/tree_broadleaf.png (96 x 128 per frame (sheet 288 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_tree_broadleaf
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing tree_palm: expected assets/World/Props/tree_palm.png (96 x 128 per frame (sheet 288 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_tree_palm
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing cactus: expected assets/World/Props/cactus.png (48 x 96 per frame (sheet 96 x 96), PNG, RGBA (straight alpha), frames 2, anchor bottom centre) - using placeholder_cactus
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing bush: expected assets/World/Props/bush.png (64 x 40 per frame (sheet 192 x 40), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_bush
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing rock: expected assets/World/Props/rock.png (64 x 40 per frame (sheet 192 x 40), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_rock
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing grass: expected assets/World/Props/grass.png (32 x 24 per frame (sheet 96 x 24), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_grass
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing sign_post: expected assets/World/Props/sign_post.png (48 x 64 per frame (sheet 96 x 64), PNG, RGBA (straight alpha), frames 2, anchor bottom centre) - using placeholder_sign_post
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing guardrail: expected assets/World/Props/guardrail.png (128 x 24, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_guardrail
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing lamp_post: expected assets/World/Props/lamp_post.png (32 x 128, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_lamp_post
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing building: expected assets/World/Props/building.png (128 x 128 per frame (sheet 384 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_building
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing finish_banner: expected assets/World/Props/finish_banner.png (256 x 96, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_finish_banner
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing sky_sun: expected assets/World/Sky/sun.png (64 x 64, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_sky_sun
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing sky_moon: expected assets/World/Sky/moon.png (64 x 64, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_sky_moon
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing sky_stars: expected assets/World/Sky/stars.png (256 x 128, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_sky_stars
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing mountains_far: expected assets/World/Sky/mountains_far.png (512 x 128, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_mountains_far
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing mountains_near: expected assets/World/Sky/mountains_near.png (512 x 128, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_mountains_near
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing terrain_grass: expected assets/World/Terrain/grass.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_grass
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing terrain_rock: expected assets/World/Terrain/rock.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_rock
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing terrain_snow: expected assets/World/Terrain/snow.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_snow
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing terrain_sand: expected assets/World/Terrain/sand.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_sand
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing water: expected assets/World/Terrain/water.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_water
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing road_asphalt: expected assets/World/Road/asphalt.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_road_asphalt
INFO: [RideService.cpp:171 in Struktur::RideIntegration::RideService::LoadWorldData]   missing road_markings: expected assets/World/Road/markings.png (16 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_road_markings
| Role | Category | Expected file | Size and format | Frames | Anchor | Placeholder used | Notes |
|---|---|---|---|---|---|---|---|
| `rider_frame` | rider | `assets/World/Rider/rider_frame.png` | 64 x 96; PNG, RGBA (straight alpha) | 1 | 32, 94 | `placeholder_rider_frame` (rider_frame) | Bike seen from behind (seat post, saddle, handlebar, derailleur) without wheels or rider. Layer 2 of 4, drawn over the wheel. |
| `rider_wheel` | rider | `assets/World/Rider/rider_wheel.png` | 6 x 32, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_rider_wheel` (rider_wheel) | Rear tyre tread strip, tileable vertically - scrolled with the wheel angle (rear view). For a side-on wheel instead, set motion 'rotate' and 'spokes' in rider.json and draw the whole wheel. |
| `rider_legs` | rider | `assets/World/Rider/rider_legs.png` | 64 x 96 per frame (sheet 768 x 288); PNG, RGBA (straight alpha) | 12 x 3 rows (12 per crank revolution) | 32, 94 | `placeholder_rider_legs` (rider_legs) | Legs from behind. Row 0: seated pedalling, frame 0 = right crank at 12 o'clock, then clockwise (seen from the right) in equal steps. Row 1: out of the saddle, same phases. Row 2: column 0 coasting (cranks level), column 1 idle (one foot down). Minimum 8 frames per revolution, 12 recommended. |
| `rider_torso` | rider | `assets/World/Rider/rider_torso.png` | 64 x 96; PNG, RGBA (straight alpha) | 1 | 32, 94 | `placeholder_rider_torso` (rider_torso) | Back, arms to the bars, helmet. Drawn last; shifted up 7 px out of the saddle (rider.json outOfSaddleOffsetY). |
| `rider_sheet` | rider | `assets/World/Rider/rider_sheet.png` | 64 x 96 per frame (sheet 768 x 288); PNG, RGBA (straight alpha) | 12 x 3 rows (12 per crank revolution) | 32, 94 | `placeholder_rider_sheet` (rider_legs) | OPTIONAL alternative to the four layers: one flat sheet (whole bike + rider per frame), same rows as rider_legs. Only used when assets/World/Rider/rider.json has kind 'sheet'. |
| `tree_pine` | props | `assets/World/Props/tree_pine.png` | 64 x 128 per frame (sheet 192 x 128); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_tree_pine` (tree_pine) | Conifer, 3 variants. Camera-facing billboard, base at the bottom centre. |
| `tree_broadleaf` | props | `assets/World/Props/tree_broadleaf.png` | 96 x 128 per frame (sheet 288 x 128); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_tree_broadleaf` (tree_broadleaf) | Deciduous tree, 3 variants. |
| `tree_palm` | props | `assets/World/Props/tree_palm.png` | 96 x 128 per frame (sheet 288 x 128); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_tree_palm` (tree_palm) | Palm, 3 variants (coastal). |
| `cactus` | props | `assets/World/Props/cactus.png` | 48 x 96 per frame (sheet 96 x 96); PNG, RGBA (straight alpha) | 2 | bottom centre | `placeholder_cactus` (cactus) | Desert cactus, 2 variants. |
| `bush` | props | `assets/World/Props/bush.png` | 64 x 40 per frame (sheet 192 x 40); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_bush` (bush) | Low shrub, 3 variants. |
| `rock` | props | `assets/World/Props/rock.png` | 64 x 40 per frame (sheet 192 x 40); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_rock` (rock) | Boulder, 3 variants. |
| `grass` | props | `assets/World/Props/grass.png` | 32 x 24 per frame (sheet 96 x 24); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_grass` (grass) | Grass tuft, 3 variants; drawn only near the road. |
| `sign_post` | props | `assets/World/Props/sign_post.png` | 48 x 64 per frame (sheet 96 x 64); PNG, RGBA (straight alpha) | 2 | bottom centre | `placeholder_sign_post` (sign) | Roadside sign on a post, 2 variants (no real place names or brands). |
| `guardrail` | props | `assets/World/Props/guardrail.png` | 128 x 24; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_guardrail` (guardrail) | One 4 m guardrail section with posts at both ends. Drawn as a quad along the road on the outside of bends. |
| `lamp_post` | props | `assets/World/Props/lamp_post.png` | 32 x 128; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_lamp_post` (lamp) | Street lamp; the lamp head overhangs to the left (towards the road on the right-hand side). |
| `building` | props | `assets/World/Props/building.png` | 128 x 128 per frame (sheet 384 x 128); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_building` (building) | House / small building front, 3 variants; faces the road. |
| `finish_banner` | props | `assets/World/Props/finish_banner.png` | 256 x 96; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_finish_banner` (banner) | Banner spanning the road on two poles (workout complete). Poles at the far left and right edges. |
| `sky_sun` | sky | `assets/World/Sky/sun.png` | 64 x 64; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_sky_sun` (sun) | Sun disc with glow, additive. |
| `sky_moon` | sky | `assets/World/Sky/moon.png` | 64 x 64; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_sky_moon` (moon) | Moon disc, alpha blended. |
| `sky_stars` | sky | `assets/World/Sky/stars.png` | 256 x 128, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_sky_stars` (stars) | Star field, tileable both ways; mapped around the sky by heading and elevation. Alpha = brightness. |
| `mountains_far` | sky | `assets/World/Sky/mountains_far.png` | 512 x 128, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_mountains_far` (mountains) | Far mountain silhouette, white on transparent (tinted per biome), tileable horizontally. Parallax layer 1. |
| `mountains_near` | sky | `assets/World/Sky/mountains_near.png` | 512 x 128, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_mountains_near` (mountains) | Nearer hills silhouette, white on transparent, tileable horizontally. Parallax layer 2. |
| `terrain_grass` | terrain | `assets/World/Terrain/grass.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_grass` (noise) | Detail texture multiplied over the vertex colour; keep it near-grey/neutral. |
| `terrain_rock` | terrain | `assets/World/Terrain/rock.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_rock` (noise) | Detail texture for steep slopes. |
| `terrain_snow` | terrain | `assets/World/Terrain/snow.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_snow` (noise) | Detail texture for snow. |
| `terrain_sand` | terrain | `assets/World/Terrain/sand.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_sand` (noise) | Detail texture for sand / beaches. |
| `water` | terrain | `assets/World/Terrain/water.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_water` (water) | Water surface detail, scrolled slowly; multiplied with the biome's water colour. |
| `road_asphalt` | road | `assets/World/Road/asphalt.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_road_asphalt` (noise) | Asphalt detail, multiplied with the biome's road colour; tiled every 4 m. |
| `road_markings` | road | `assets/World/Road/markings.png` | 16 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_road_markings` (markings) | Paint texture inside the lines (the line pattern itself is computed in the shader). |
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing rider_frame: expected assets/World/Rider/rider_frame.png (64 x 96, PNG, RGBA (straight alpha), frames 1, anchor 32, 94) - using placeholder_rider_frame
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing rider_wheel: expected assets/World/Rider/rider_wheel.png (6 x 32, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_rider_wheel
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing rider_legs: expected assets/World/Rider/rider_legs.png (64 x 96 per frame (sheet 768 x 288), PNG, RGBA (straight alpha), frames 12 x 3 rows (12 per crank revolution), anchor 32, 94) - using placeholder_rider_legs
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing rider_torso: expected assets/World/Rider/rider_torso.png (64 x 96, PNG, RGBA (straight alpha), frames 1, anchor 32, 94) - using placeholder_rider_torso
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing rider_sheet: expected assets/World/Rider/rider_sheet.png (64 x 96 per frame (sheet 768 x 288), PNG, RGBA (straight alpha), frames 12 x 3 rows (12 per crank revolution), anchor 32, 94) - using placeholder_rider_sheet
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing tree_pine: expected assets/World/Props/tree_pine.png (64 x 128 per frame (sheet 192 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_tree_pine
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing tree_broadleaf: expected assets/World/Props/tree_broadleaf.png (96 x 128 per frame (sheet 288 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_tree_broadleaf
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing tree_palm: expected assets/World/Props/tree_palm.png (96 x 128 per frame (sheet 288 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_tree_palm
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing cactus: expected assets/World/Props/cactus.png (48 x 96 per frame (sheet 96 x 96), PNG, RGBA (straight alpha), frames 2, anchor bottom centre) - using placeholder_cactus
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing bush: expected assets/World/Props/bush.png (64 x 40 per frame (sheet 192 x 40), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_bush
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing rock: expected assets/World/Props/rock.png (64 x 40 per frame (sheet 192 x 40), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_rock
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing grass: expected assets/World/Props/grass.png (32 x 24 per frame (sheet 96 x 24), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_grass
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing sign_post: expected assets/World/Props/sign_post.png (48 x 64 per frame (sheet 96 x 64), PNG, RGBA (straight alpha), frames 2, anchor bottom centre) - using placeholder_sign_post
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing guardrail: expected assets/World/Props/guardrail.png (128 x 24, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_guardrail
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing lamp_post: expected assets/World/Props/lamp_post.png (32 x 128, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_lamp_post
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing building: expected assets/World/Props/building.png (128 x 128 per frame (sheet 384 x 128), PNG, RGBA (straight alpha), frames 3, anchor bottom centre) - using placeholder_building
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing finish_banner: expected assets/World/Props/finish_banner.png (256 x 96, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_finish_banner
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing sky_sun: expected assets/World/Sky/sun.png (64 x 64, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_sky_sun
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing sky_moon: expected assets/World/Sky/moon.png (64 x 64, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_sky_moon
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing sky_stars: expected assets/World/Sky/stars.png (256 x 128, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_sky_stars
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing mountains_far: expected assets/World/Sky/mountains_far.png (512 x 128, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_mountains_far
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing mountains_near: expected assets/World/Sky/mountains_near.png (512 x 128, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_mountains_near
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing terrain_grass: expected assets/World/Terrain/grass.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_grass
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing terrain_rock: expected assets/World/Terrain/rock.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_rock
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing terrain_snow: expected assets/World/Terrain/snow.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_snow
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing terrain_sand: expected assets/World/Terrain/sand.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_terrain_sand
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing water: expected assets/World/Terrain/water.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_water
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing road_asphalt: expected assets/World/Road/asphalt.png (64 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_road_asphalt
INFO: [Game.cpp:208 in Struktur::InitialiseGame] missing road_markings: expected assets/World/Road/markings.png (16 x 64, tileable, PNG, RGBA (straight alpha), frames 1, anchor bottom centre) - using placeholder_road_markings
INFO: [EngineSettings.cpp:15 in Struktur::Core::EngineSettings::Load] ENGINESETTINGS: No settings found at engine_settings.json, using defaults
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/animation.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/events.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/gameObject.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/resourceManager.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/gameObjectComponents.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/math.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/physics.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/dialogue.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/ui.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/callback.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/fileSystem.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/flags.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/app.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/input.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/debug.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/serialisation.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/audio.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/localization.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/fitness.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/ride.wren
INFO: [WrenCodeGenerator.cpp:325 in Struktur::Wren::CodeGenerator::GenerateModuleFile] Generated: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/renderer.wren
INFO: [WrenCodeGenerator.cpp:65 in Struktur::Wren::CodeGenerator::GenerateBindingFiles] Generated 21 Wren binding file(s) in: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings
INFO: [WrenCodeGenerator.cpp:428 in Struktur::Wren::CodeGenerator::GenerateAllHeaders] Generated combined header with 21 module(s): C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/Generated/WrenBindingSources.h
INFO: [WrenCodeGenerator.cpp:69 in Struktur::Wren::CodeGenerator::GenerateBindingFiles] Generated release headers in: C:\Users\H1Ber\Documents\Game Creation Projects\Struktur/../src/WrenBindings/Bindings/Generated
INFO: [Game.cpp:279 in Struktur::InitialiseGame] Game Data Loaded
INFO: [InputConfigLoader.cpp:52 in Struktur::Input::InputConfigLoader::LoadFromFile] Set deadzone to 0.2
INFO: [InputConfigLoader.cpp:122 in Struktur::Input::InputConfigLoader::LoadFromFile] Successfully loaded input config from: Settings/InputBindings/InputConfig.json
INFO: [LocalizationManager.cpp:119 in Struktur::Localization::LocalizationManager::LoadLanguage] LOCALIZATION: Loaded 708 strings for 'en' from Localization/en.json
INFO: [LocalizationManager.cpp:119 in Struktur::Localization::LocalizationManager::LoadLanguage] LOCALIZATION: Loaded 2 strings for 'fr' from Localization/fr.json
INFO: [FlagManager.cpp:271 in Struktur::Flag::FlagManager::Load] FLAGMANAGER: Loaded flags from flags.sav
INFO: [WrenDebug.cpp:36 in wren_DebugInfo] Loading all dialogue...
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 14)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Finn.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Thomas.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Kira.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Elena.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Garrett.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Child.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Guide.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/Warden.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [Dialogue Loading] succesfuly loaded Dialogue/Arc1/World.json
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 40)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] All dialogue loaded
Call stack:
  [0] loadAllDialogue() in Dialogue/DialogueLoader (line 48)
  [1] start() in Main (line 110)
  [2] start() in <unknown> (line 0)

INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: giveItem
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: removeItem
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: giveGold
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: takeGold
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: unlockMove
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: setIntFlag
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: setFlag
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: cameraShake
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: pickupEntity
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: joinParty
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: leaveParty
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: setSpawnPoint
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: endArc
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: restParty
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: partyStay
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: partyFollow
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: closeChoice
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: startDuel
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: startBattle
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: openDoor
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: closeDoor
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: moveTo
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: moveToPlayer
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: soulFragment
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: shootArrow
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: chargePlayer
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: teleport
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: face
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: playAnimation
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: expression
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: closeUp
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: closeUpEnd
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: wait
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: startQuest
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: completeObjective
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: completeQuest
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: triggerEvent
INFO: [DialogueRegistry.cpp:80 in Struktur::Dialogue::DialogueRegistry::RegisterCommandType] Registered command type: putdownEntity
INFO: [DialogueRegistry.cpp:65 in Struktur::Dialogue::DialogueRegistry::RegisterConditionType] Registered condition type: hasIntFlag
INFO: [DialogueRegistry.cpp:65 in Struktur::Dialogue::DialogueRegistry::RegisterConditionType] Registered condition type: hasItem
INFO: [DialogueRegistry.cpp:65 in Struktur::Dialogue::DialogueRegistry::RegisterConditionType] Registered condition type: hasGold
INFO: [DialogueRegistry.cpp:65 in Struktur::Dialogue::DialogueRegistry::RegisterConditionType] Registered condition type: inParty
INFO: [DialogueRegistry.cpp:65 in Struktur::Dialogue::DialogueRegistry::RegisterConditionType] Registered condition type: notInParty
INFO: [DialogueRegistry.cpp:65 in Struktur::Dialogue::DialogueRegistry::RegisterConditionType] Registered condition type: notHasItem
INFO: [DialogueRegistry.cpp:95 in Struktur::Dialogue::DialogueRegistry::RegisterVariableType] Registered variable type: item_count
INFO: [DialogueRegistry.cpp:95 in Struktur::Dialogue::DialogueRegistry::RegisterVariableType] Registered variable type: contains_item
INFO: [DialogueRegistry.cpp:95 in Struktur::Dialogue::DialogueRegistry::RegisterVariableType] Registered variable type: keys_collected
INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [StateManager] created
Call stack:
  [0] init new() in States/StateManager (line 16)
  [1]  in States/StateManager (line 17)
  [2] init new() in States/GameWorldState (line 229)
  [3]  in States/GameWorldState (line 254)
  [4] insertState(_,_) in States/StateManager (line 97)
  [5] start() in Main (line 125)
  [6] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] [StateManager] created
Call stack:
  [0] init new() in States/StateManager (line 16)
  [1]  in States/StateManager (line 17)
  [2] init new() in States/BattleTestState (line 73)
  [3]  in States/BattleTestState (line 75)
  [4] insertState(_,_) in States/StateManager (line 97)
  [5] start() in Main (line 138)
  [6] start() in <unknown> (line 0)

INFO: [WrenDebug.cpp:36 in wren_DebugInfo] DevicePairingState entered
Call stack:
  [0] enter(_,_) in States/BaseState (line 14)
  [1] enter(_,_) in States/DevicePairingState (line 35)
  [2] changeState(_,_) in States/StateManager (line 74)
  [3] changeState(_) in States/StateManager (line 56)
  [4] start() in Main (line 142)
  [5] start() in <unknown> (line 0)

INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'UI/Font/NormalFont.ttf (9pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: UI/Font/NormalFont.ttf (size: 9, glyphs: 95)
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::GetResource] Loading resource 'UI/Theme/Wip/ThemeMetal/nine_path_panel.png'
INFO: [TextureResource.cpp:76 in Struktur::Resource::TextureResource::LoadFromDisk] Loaded texture from disk: UI/Theme/Wip/ThemeMetal/nine_path_panel.png (16x16)
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::GetResource] Loading resource 'UI/Theme/Theme Wood/unchecked.png'
INFO: [TextureResource.cpp:76 in Struktur::Resource::TextureResource::LoadFromDisk] Loaded texture from disk: UI/Theme/Theme Wood/unchecked.png (16x16)
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::GetResource] Loading resource 'UI/Theme/Theme Wood/nine_path_focus.png'
INFO: [TextureResource.cpp:76 in Struktur::Resource::TextureResource::LoadFromDisk] Loaded texture from disk: UI/Theme/Theme Wood/nine_path_focus.png (8x8)
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::GetResource] Loading resource 'UI/Theme/Theme Wood/button_normal.png'
INFO: [TextureResource.cpp:76 in Struktur::Resource::TextureResource::LoadFromDisk] Loaded texture from disk: UI/Theme/Theme Wood/button_normal.png (16x8)
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Loading resource 'default (32pt)'
INFO: [FontResource.cpp:214 in Struktur::Resource::FontResource::LoadFromDisk] Loaded font from disk: Fonts/machine-std/machine-std-regular.ttf (size: 32, glyphs: 95)
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::GetResource] Loading resource 'UI/Input/Keyboard/KeySpace.png'
INFO: [TextureResource.cpp:76 in Struktur::Resource::TextureResource::LoadFromDisk] Loaded texture from disk: UI/Input/Keyboard/KeySpace.png (33x13)
INFO: [ResourcePool.h:144 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::GetResource] Resource 'UI/Font/NormalFont.ttf (9pt)' found in cache
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'default (32pt)'
INFO: [ResourcePool.h:150 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::GetResource] Loading resource 'UI/Theme/Theme Wood/button_disabled.png'
INFO: [TextureResource.cpp:76 in Struktur::Resource::TextureResource::LoadFromDisk] Loaded texture from disk: UI/Theme/Theme Wood/button_disabled.png (16x8)
INFO: [Game.cpp:294 in Struktur::ExitGame] [Clean Up] Wren State Manager
INFO: [WrenDebug.cpp:36 in wren_DebugInfo] Quit requested
Call stack:
  [0] quit() in Main (line 168)
  [1] quit() in <unknown> (line 0)

INFO: [WrenStateManager.cpp:270 in Struktur::Wren::WrenStateManager::Shutdown] [WrenStateManager] Shutdown complete
INFO: [Game.cpp:298 in Struktur::ExitGame] [Clean Up] UI Manager
INFO: [Game.cpp:302 in Struktur::ExitGame] [Clean Up] Registry
INFO: [Game.cpp:306 in Struktur::ExitGame] [Clean Up] Game Object Manager
INFO: [Game.cpp:310 in Struktur::ExitGame] [Clean Up] Physics World
INFO: [Game.cpp:314 in Struktur::ExitGame] [Clean Up] Dialogue Registry
INFO: [DialogueRegistry.cpp:20 in Struktur::Dialogue::DialogueRegistry::Clear] DialogueRegistry destroyed, releasing 6 condition callbacks and 38 command callbacks
INFO: [Game.cpp:318 in Struktur::ExitGame] [Clean Up] Wren Script Component Registry
INFO: [Game.cpp:322 in Struktur::ExitGame] [Clean Up] Wren Script Engine
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::EvictIfUnused] Unloading unreferenced resource 'UI/Theme/Theme Wood/button_disabled.png'
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::EvictIfUnused] Unloading unreferenced resource 'UI/Theme/Theme Wood/button_normal.png'
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::EvictIfUnused] Unloading unreferenced resource 'UI/Theme/Theme Wood/nine_path_focus.png'
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::EvictIfUnused] Unloading unreferenced resource 'UI/Theme/Theme Wood/unchecked.png'
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::TextureResource,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >::EvictIfUnused] Unloading unreferenced resource 'UI/Theme/Wip/ThemeMetal/nine_path_panel.png'
INFO: [ResourcePool.h:80 in Struktur::Resource::ResourcePool<class Struktur::Resource::FontResource,struct Struktur::Resource::FontKey>::EvictIfUnused] Unloading unreferenced resource 'UI/Font/NormalFont.ttf (9pt)'
INFO: [Game.cpp:326 in Struktur::ExitGame] [Clean Up] DialogueManager
INFO: [DialogueManager.cpp:30 in Struktur::Dialogue::DialogueManager::Clear] Cleared all dialogue nodes
INFO: [Game.cpp:330 in Struktur::ExitGame] [Clean Up] Resource Manager
