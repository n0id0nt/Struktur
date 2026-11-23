# Arc 1 dialogue

Generated from `assets/Dialogue/Arc1Dialogue.json` and `assets/Localization/en.json` by
`python tools/dialogue_to_markup.py` - regenerate it after the dialogue changes.

## How to read and mark this

- Each `###` heading is one dialogue node (its id in the JSON).
- **Speaker** *(expression)*: the line. The expression shows on the speaker's portrait; with no art for it
  yet, the portrait shows its default face with the expression written underneath.
- *Staging* lines are what plays while the line shows (or, for a node with no line, before moving on).
- *Effects* are what the node changes in the game (items, flags, fights).
- **Missing** notes are things for you to add (art, animations, markers, sounds).
- *Kept* marks text read in the world (signs, tablets, puzzles, tutorials) - not narration, so it stays.
- To ask for a change, add a line starting with `> EDIT:` under the node, e.g.
  `> EDIT: Kira should laugh here` - anything marked that way can then be applied.

## Still to add

### Expression art

Every portrait (assets/Portraits, rendered by 3dModelsToSprites `job.portrait.json`) has only its default
face - the rigs can't change faces. These expressions are asked for and show as captions for now:

- **Child**: eager, excited, wide-eyed
- **Elena**: fussing, out of breath
- **Finn**: approving, caught out, chuckling, evasive, grave, knowing, mysterious, proud, sad smile, searching, sly, squinting, straining, warm, wistful
- **Garrett**: glaring
- **Guide**: delighted, sheepish, stronger
- **Kira**: amazed, awed, breathless, cheerful, chewing, concentrating, confident, content, delighted, determined, eager, firm, flustered, focused, nodding, overwhelmed, pensive, pleased, proud, quiet, quietly pleased, realising, sad, smiling, squinting, startled, sulky, thoughtful, troubled, uneasy
- **Thomas**: chuckling, earnest, firm, focused, impressed, laughing, nodding, pleased, quiet, rueful, serious, urgent, winded
- **Warden**: awakening, menacing, reverent

When a face exists for one, list it in `Dialogue/Portraits.wren` `EXPRESSIONS` (and wire its image).

### Per scene

- `rest_spot` - A rest-spot sitting pose that fits the spot (a bench / log) - SittingIdle plays where the player stands.
- `finn_s1` - Horseshoe prop in Finn's hand (he plays Inspecting).
- `finn_s1_hammer` - Animation: catching (a hammer slides off the bench and Finn catches it without looking) - PickingUp stands in. Hammer prop.
- `finn_s1_a2` - Anvil prop for Finn's Lifting.
- `finn_s1_end_2` - Animation: handing over (Finn gives the practice sword and three herbs) - the player's Lifting stands in.
- `elena_bread` - Animation: handing over (Elena piles rolls into your arms) - the player's Lifting stands in.
- `elena_rations` - She was written as running up - she's already being talked to, so nothing moves. A Path for her to run in on would restore it.
- `garrett_ask` - He should face the well while he grumbles - the well isn't a named actor or Marker yet (a Marker called e.g. VillageWell would do).
- `child_s5` - Animation: tugging your cloak.
- `child_s5_end` - She should run off - needs a Path / Marker to run to (e.g. ChildRunsOff), then moveTo with speed=run.
- `kira_early_shot` - Sounds: the bow's twang and the thunk as the arrow hits (the arrow is a particle - GameObjects/ArrowShot.wren - that vanishes in a puff of dust; it doesn't stay stuck in the target).
- `kira_s3_shot` - Sounds, as above. Animation: eating an apple (she's on her third).
- `kira_s3_a1` - Animation: patting the book on her belt.
- `kira_s3_a2` - Close-up art: the Big Book held upside down (placeholder CloseUps/kira_book_upside_down.png).
- `thomas_early` - Animation: sharpening a knife - Working stands in. Knife prop.
- `thomas_heal_before` - Animation: handing over the potion.
- `thomas_leaves_cabin` - He walks to the cabin door, then is simply at the training grounds when you get there - staging can't walk anyone between rooms.
- `thomas_gone_to_gate` - He disappears from the training grounds as the conversation closes - a Path to the grounds' exit would let him walk off first.
- `thomas_gate_noticed` - Wolf art (it wears the Chinlin's sheet) and a growl as it charges. The fight is the player's alone (GameObjects/PerimeterWolf.wren fightsLeaderAlone).
- `thomas_gate_done` - He disappears from the gate as the conversation closes, and is back in his cabin.
- `thomas_after_2` - Animation: handing over (a better bow for Kira, leather armour for you) - Kira's Lifting stands in.
- `thomas_after_kira` - Animation: slinging a bow over the shoulder.
- `thomas_after_2_2` - Animation: holding out a small horn. Horn prop.
- `finn_map` - Animation: holding out / handing over the map.
- `big_book` - Kira should sit on a rock and draw with charcoal - a sitting-and-drawing clip (Inspecting stands in, standing). A rock to sit on near the fight.
- `big_book_a1` - Close-up art: Kira's drawing of the wolf fact (placeholder CloseUps/kira_book_wolf_fact.png).
- `stone_pillars` - Close-up art: the pillar carvings (placeholder CloseUps/shrine_pillar_carvings.png).
- `tablet_1` - Close-up art for all five tablets (placeholders CloseUps/tablet_circle/triangle/square/diamond/star.png).
- `shifting_floor` - A rumble sound with the camera shake.
- `kira_split_2` - Sounds: the plate's click and the slab slamming down.
- `split_wall` - Animation: knocking on the wall.
- `heart_memory` - Close-up art: the long table memory (placeholder CloseUps/memory_long_table.png).
- `hidden_door_3` - An effect for the dark wall folding itself away (the door just opens).
- `central_mural_find_2` - Close-up art: the six figures and the city of light (placeholder CloseUps/memory_six_figures.png).
- `warden_p3` - Animation: the Warden kneeling (it has no overworld sheet of its own - it wears the wrestler's).
- `warden_peace_2` - The hidden mechanism in the throne (only a camera shake plays).
- `warden_violent` - The final blow - nothing shows the blade finding the crack now.
- `viewer_s17_2` - Close-up art for the three memory beats (placeholders CloseUps/memory_city_height.png, memory_war.png, memory_equalitas.png).
- `viewer_s17_5` - The "go home and talk to Finn" hint is gone with the narration - the quest log has to carry it.
- `wolf_heal` - A healing effect from your hands, and the wolf licking your hand and padding away.
- `finn_spar_after_2` - Animation: handing over Finn's blade.
- `finn_rev_c_3` - Animation: opening a hidden compartment in the forge - PickingUp stands in.
- `farewell_1` - Animation: Finn standing to address the gathering.
- `farewell_2` - Animation: a hand on your shoulder.
- `road_end_1` - The narrated ending is gone - the end-of-arc card follows the choice straight away. An epilogue close-up could take its place.

### Other

- The Guide is a voice, but speaks with Claire's portrait (the Guide NPC wears Claire's sheet).
- The player has a portrait (`Portraits/explorer_portrait.png`) but never speaks a line yet.
- `[PH]` lines are placeholder writing from the original script.

## The script

## Resting

### `rest_spot`

*Staging:* Player plays **SittingIdle** (the line waits for it)

*Effects:* restParty()

→ `rest_spot_up`

**Missing:** A rest-spot sitting pose that fits the spot (a bench / log) - SittingIdle plays where the player stands.

### `rest_spot_up`

*Staging:* Player plays **GettingUp** (the line waits for it), then Idle

### `rest_bed`

- if `bed_intro_seen` == 0 → `bed_sign`
- if otherwise → `rest_bed_sleep`

### `rest_bed_sleep`

*Staging:* Player plays **LyingDown** (the line waits for it)

*Effects:* restParty()

→ `rest_bed_sleep_up`

### `rest_bed_sleep_up`

*Staging:* Player plays **GettingUp** (the line waits for it), then Idle

### `bed_sign`

*Staging:* Player plays **GettingUp** (the line waits for it), then Idle

*Effects:* `bed_intro_seen` = 1

→ `bed_wake`

### `bed_wake`

## The opening

### `opening`

- if otherwise → `bed_sign`

## Finn's workshop - the first talk

### `workshop_stairs`

*Staging:* Finn is placed at FinnAtAnvil; Finn turns to face down; Finn plays **Working**

→ `workshop_walk`

### `workshop_walk`

*Staging:* Player walks to PlayerOutOfBed

→ `finn_s1`

### `finn`

- if `arc1_stage` >= 11 → `finn_farewell`
- if `arc1_stage` == 10 → `finn_reveal_done`
- if `arc1_stage` == 9 and `duel_won_Finn` == 1 and `finn_spar` < 2 → `finn_spar_after`
- if `arc1_stage` == 9 and `finn_spar` == 1 → `finn_spar_again`
- if `arc1_stage` == 9 and `finn_spar` == 0 → `finn_spar`
- if `arc1_stage` == 9 → `finn_reveal`
- if `arc1_stage` >= 4 → `finn_away`
- if `arc1_stage` == 3 → `finn_map_greet`
- if `arc1_stage` == 2 → `finn_reminder_thomas`
- if `arc1_stage` >= 1 → `finn_reminder`
- if otherwise → `finn_s1`

### `finn_s1`

**Finn** *(squinting)*: Hm. Hmmm. It's a horseshoe. I'll give you that. A horse might even agree.

*Staging:* Player turns to face Finn; Finn turns to face Player; Finn plays **Inspecting**

→ `finn_s1_2`

**Missing:** Horseshoe prop in Finn's hand (he plays Inspecting).

### `finn_s1_2`

**Finn**: No, it's good. Better than mine were at your age. Don't tell anyone I said that - I've a reputation for grumpiness to keep up.

→ `finn_s1_hammer`

### `finn_s1_hammer`

*Staging:* Finn plays **PickingUp** (the line waits for it), then Idle

→ `finn_s1_hammer_2`

**Missing:** Animation: catching (a hammer slides off the bench and Finn catches it without looking) - PickingUp stands in. Hammer prop.

### `finn_s1_hammer_2`

**Finn** *(caught out)*: ...Reflexes. From all the, ah, dropping. Ow. My back.

→ `finn_s1_task`

### `finn_s1_task`

**Finn**: Now. An errand. There's an old shrine past the stones in [b]Whisperwood[/b], and in it there's some old metalwork I've wanted for years. Bring it back and I'll stop pretending your horseshoes are bad.

→ `finn_s1_menu`

### `finn_s1_menu`

**Finn**: Well?

1. What kind of metalwork? → `finn_s1_a1`
2. Why don't you go? → `finn_s1_a2`
3. Past the stones? Nobody goes past the stones. → `finn_s1_a3`

### `finn_s1_a1`

**Finn**: Old. Older than me, which is saying something. You'll know it when you see it. It'll be the only thing down there worth carrying.

→ `finn_s1_end`

### `finn_s1_a2`

**Finn** *(straining)*: My back. Terrible back.

*Staging:* Finn plays **Lifting** (the line waits for it), then Idle

→ `finn_s1_end`

**Missing:** Anvil prop for Finn's Lifting.

### `finn_s1_a3`

**Finn**: Which is why the metalwork's still there.

→ `finn_s1_end`

### `finn_s1_end`

**Finn**: Take [b]Kira[/b]. She'll know the way, or she'll claim to, which is nearly as good. She'll be at the [b]training grounds[/b].

→ `finn_fetch`

### `finn_fetch`

*Staging:* Finn walks to FinnToWorkBench

→ `finn_fetch_2`

### `finn_fetch_2`

*Staging:* Finn plays **PickingUp** (the line waits for it), then Idle

→ `finn_fetch_3`

### `finn_fetch_3`

*Staging:* Finn walks to FinnBackToPlayer

→ `finn_fetch_4`

### `finn_fetch_4`

*Staging:* Player turns to face down; Finn walks up to the player

→ `finn_s1_end_2`

### `finn_s1_end_2`

**Finn**: Pointy end away from you. Herbs for when it isn't.

*Staging:* Player plays **Lifting**, then Idle

*Effects:* gives Healing Herb x3; `arc1_stage` = 1

→ `finn_to_anvil`

**Missing:** Animation: handing over (Finn gives the practice sword and three herbs) - the player's Lifting stands in.

### `finn_to_anvil`

*Staging:* Finn walks to FinnToAnvil

→ `finn_working`

### `finn_working`

*Staging:* Finn turns to face down; Finn plays **Working**

### `finn_reminder`

**Finn**: Kira's at the [b]training grounds[/b]. And see Thomas before you go anywhere near the stones - he'll want to show you which end to hold.

*Staging:* Finn turns to face Player

→ `finn_working`

### `finn_reminder_thomas`

**Finn**: Good, you found Kira. Now go and see Thomas at his cabin before you go anywhere near the stones - he'll want to show you which end to hold.

*Staging:* Finn turns to face Player

→ `finn_working`

## Haven - Elena, Garrett, the well and the child

### `elena`

- if `arc1_stage` >= 9 → `elena_evening`
- if `arc1_stage` >= 3 and `elena_rations` == 0 → `elena_rations`
- if `elena_bread` == 0 → `elena_bread`
- if otherwise → `elena_idle`

### `elena_bread`

**Elena** *(fussing)*: Off past the stones? Take rolls. No, more rolls. You'll come back thin, everyone does.

*Staging:* Player plays **Lifting**, then Idle

*Effects:* gives Bread Roll x5; `elena_bread` = 1

**Missing:** Animation: handing over (Elena piles rolls into your arms) - the player's Lifting stands in.

### `elena_rations`

**Elena** *(out of breath)*: Wait! I almost forgot. Special travel rations - they'll keep you going even when regular food won't.

*Effects:* gives Special Ration x3; `elena_rations` = 1

**Missing:** She was written as running up - she's already being talked to, so nothing moves. A Path for her to run in on would restore it.

### `elena_idle`

**Elena**: Mind the oven - and mind yourselves in those woods!

### `elena_evening`

**Elena**: You're back! Come by the gathering later - the whole village is sharing supper tonight.

### `garrett`

- if `well_fixed` == 1 and `garrett_key` == 0 → `garrett_reward`
- if `well_fixed` == 1 → `garrett_idle`
- if otherwise → `garrett_ask`

### `garrett_ask`

**Garrett** *(glaring)*: Blasted rope's tangled again. Sixty years I've pulled this bucket and it's never once been grateful. Have a look at it, would you? My eyes aren't what they were.

*Effects:* `garrett_asked` = 1

**Missing:** He should face the well while he grumbles - the well isn't a named actor or Marker yet (a Marker called e.g. VillageWell would do).

### `garrett_reward`

**Garrett**: By the gods! You've got quite the eye, young one. Here, take this - it opens most of the storage buildings if you ever need supplies.

*Effects:* gives Village Key x1; `garrett_key` = 1

### `garrett_idle`

**Garrett**: Water flowing sweet as ever, thanks to you.

### `village_well`

- if `well_fixed` == 1 → `well_fixed`
- if otherwise → `well_puzzle`

### `well_puzzle_fail`

The rope snags. Start again from the first pull.  *(kept - read in the world)*

### `well_puzzle_done`

The rope slips free of a loose stone, and water gushes up from below.  *(kept - read in the world)*

*Staging:* Player plays **Lifting** (the line waits for it), then Idle

*Effects:* `well_fixed` = 1

### `well_puzzle_step1`

First pull?  *(kept - read in the world)*

1. Left → `well_puzzle_step2`
2. Right → `well_puzzle_fail`
3. Up → `well_puzzle_fail`
4. Down → `well_puzzle_fail`

### `well_puzzle_step2`

Second pull?  *(kept - read in the world)*

1. Left → `well_puzzle_fail`
2. Right → `well_puzzle_fail`
3. Up → `well_puzzle_fail`
4. Down → `well_puzzle_step3`

### `well_puzzle_step3`

Third pull?  *(kept - read in the world)*

1. Left → `well_puzzle_fail`
2. Right → `well_puzzle_done`
3. Up → `well_puzzle_fail`
4. Down → `well_puzzle_fail`

### `well_puzzle`

[PH] The rope is tangled around the pulley. You'll need to work it loose, one pull at a time.  *(kept - read in the world)*

→ `well_puzzle_step1`

### `well_fixed`

Clear, cold water. The rope runs smoothly now.  *(kept - read in the world)*

### `child`

- if `arc1_stage` >= 9 → `child_return`
- if `arc1_stage` >= 3 → `child_s5`
- if otherwise → `child_idle`

### `child_idle`

**Child**: Are you going on an adventure? Can I come? ...Mum says no.

### `child_s5`

**Child** *(eager)*: Are you really going to fight monsters?

1. I hope we won't have to fight anything. → `child_s5_end`
2. Only if they threaten innocent people. → `child_s5_end`
3. We're explorers, not fighters. → `child_s5_end`

**Missing:** Animation: tugging your cloak.

### `child_s5_end`

**Child** *(excited)*: Woah! I'm telling everyone!

**Missing:** She should run off - needs a Path / Marker to run to (e.g. ChildRunsOff), then moveTo with speed=run.

### `child_return`

**Child**: You're back! Did you find any treasure? Did you fight monsters?

1. We found something better than treasure. → `child_return_end`
2. We learned most monsters are just scared or hurt. → `child_return_end`
3. The real treasure was the friends we made along the way. → `child_return_end`

### `child_return_end`

**Child** *(wide-eyed)*: That's the best story ever. Tell me more tomorrow!

## Kira at the training grounds

### `kira`

- if `arc1_stage` >= 1 → `kira_s3_walk`
- if otherwise → `kira_early_walk`

### `kira_early_walk`

**Kira** *(cheerful)*: Oh - morning! I'm in the middle of training.

*Staging:* Kira turns to face Player; Player turns to face Kira

→ `kira_early_spot`

### `kira_early_spot`

*Staging:* Kira walks to KiraToTrainingSpot; Player walks to PlayerWatchesKira

→ `kira_early_face`

### `kira_early_face`

*Staging:* Kira turns to face right; Player turns to face right

→ `kira_early_watch`

### `kira_early_watch`

**Kira** *(focused)*: Watch this.

→ `kira_early_shot`

### `kira_early_shot`

*Staging:* Kira plays **Shoot** (the line waits for it), then Idle; Kira looses an arrow east, 70 px, after 0.75s; pause 0.5s

→ `kira_early`

**Missing:** Sounds: the bow's twang and the thunk as the arrow hits (the arrow is a particle - GameObjects/ArrowShot.wren - that vanishes in a puff of dust; it doesn't stay stuck in the target).

### `kira_s3_walk`

**Kira** *(cheerful)*: There you are! Give me a moment - I'm training.

*Staging:* Kira turns to face Player; Player turns to face Kira

→ `kira_s3_spot`

### `kira_s3_spot`

*Staging:* Kira walks to KiraToTrainingSpot; Player walks to PlayerWatchesKira

→ `kira_s3_face`

### `kira_s3_face`

*Staging:* Kira turns to face right; Player turns to face right

→ `kira_s3_watch`

### `kira_s3_watch`

**Kira** *(focused)*: Watch this one.

→ `kira_s3_shot`

### `kira_s3_shot`

*Staging:* Kira plays **Shoot** (the line waits for it), then Idle; Kira looses an arrow east, 70 px, after 0.75s; pause 0.5s

→ `kira_s3`

**Missing:** Sounds, as above. Animation: eating an apple (she's on her third).

### `kira_early`

**Kira** *(pleased)*: Dead centre! Anyway - Finn was looking for you, you know.

*Staging:* Kira turns to face Player; Player turns to face Kira

### `kira_s3`

**Kira** *(chewing)*: You're going past the stones? I'm going past the stones. Finn sent word. I've been waiting all morning, which is why I'm on my third apple.

*Staging:* Kira turns to face Player; Player turns to face Kira

→ `kira_s3_facts`

### `kira_s3_facts`

**Kira**: Here's what you need to know about [b]Whisperwood[/b]. Fact one: the trees can hear you, so don't insult them. Fact two: thornwolves can smell lies, which is why they always bite the liar first. Fact three: if you get lost, follow the moss. It always points to the nearest pie.

→ `kira_s3_menu`

### `kira_s3_menu`

**Kira**: Questions?

1. Where are these facts from? → `kira_s3_a1`
2. Right. Let's go. → `kira_s3_end`

### `kira_s3_a1`

**Kira** *(proud)*: The [b]Big Book of Facts[/b]. A priest gave it to Dad. It's full of them.

→ `kira_s3_menu2`

**Missing:** Animation: patting the book on her belt.

### `kira_s3_menu2`

**Kira**: Anything else?

1. Have you read it? → `kira_s3_a2`
2. Right. Let's go. → `kira_s3_end`

### `kira_s3_a2`

**Kira** *(confident)*: I don't need to read it, I know what's in it. Dad told me.

*Staging:* Kira plays **Inspecting**; close-up: `CloseUps/kira_book_upside_down.png`

→ `kira_s3_end`

**Missing:** Close-up art: the Big Book held upside down (placeholder CloseUps/kira_book_upside_down.png).

### `kira_s3_end`

**Kira**: Dad first, though. He'll want to see us before we go anywhere near the stones. He says that a lot.

*Staging:* the close-up closes; Kira plays **Idle**

→ `kira_comes_over`

### `kira_comes_over`

*Staging:* Kira walks up to the player

→ `kira_s3_end_2`

### `kira_s3_end_2`

**Kira**: Come on - his cabin's just down the path.

*Effects:* joinParty(name=Kira); `arc1_stage` = 2

## Kira in the party

### `kira_party_stay`

- if `arc1_stage` == 10 → `kira_s21`
- if otherwise → `kira_banter`

### `kira_party_follow`

- if otherwise → `kira_follow_menu`

### `kira_banter`

- if `arc1_stage` >= 9 → `kira_banter_home`
- if `arc1_stage` >= 7 → `kira_banter_shrine`
- if `arc1_stage` >= 4 → `kira_banter_forest`
- if otherwise → `kira_banter_village`

### `kira_banter_village`

**Kira**: Dad's cabin is just down the path. He'll want to tell you about the moss.

→ `kira_stay_menu`

### `kira_banter_forest`

**Kira**: Stay close. And don't insult the trees - they can hear you. Fact.

→ `kira_stay_menu`

### `kira_banter_shrine`

**Kira**: You've been quiet since that chamber. Whatever happened in there - I'm with you.

→ `kira_stay_menu`

### `kira_banter_home`

**Kira**: Has the village always been like this? Everyone sharing, helping each other... it's just like those murals.

→ `kira_stay_menu`

### `kira_stay_menu`

**Kira**: Want me to hold position here?

1. Wait here. → `kira_stay_yes`
2. No, stay with me. → `kira_stay_no`

### `kira_stay_yes`

**Kira**: I'll keep watch.

*Effects:* partyStay(name=Kira)

### `kira_stay_no`

**Kira**: Right behind you.

### `kira_follow_menu`

**Kira**: Ready to move on?

1. Let's go. → `kira_follow_yes`
2. Stay a bit longer. → `kira_follow_no`

### `kira_follow_yes`

**Kira**: Finally!

*Effects:* partyFollow(name=Kira)

### `kira_follow_no`

**Kira**: I'll be here.

## Thomas - the cabin, the training grounds and the north gate

### `thomas`

- if `arc1_stage` >= 9 and `warden_violent` == 1 → `thomas_violent`
- if `arc1_stage` >= 3 → `thomas_idle`
- if `arc1_stage` == 2 and `thomas_lesson` >= 3 and `duel_won_Thomas` == 1 → `thomas_heal_test`
- if `arc1_stage` == 2 and `thomas_lesson` >= 3 → `thomas_test_again`
- if `arc1_stage` == 2 and `thomas_place` == 2 and `defeated_StarvingThornwolf` >= 1 → `thomas_gate_done`
- if `arc1_stage` == 2 and `thomas_place` == 2 → `thomas_gate`
- if `arc1_stage` == 2 and `thomas_lesson` == 1 and `defeated_StarvingThornwolf` >= 1 → `thomas_wolf_back`
- if `arc1_stage` == 2 and `thomas_lesson` == 1 → `thomas_wolf_wait`
- if `arc1_stage` == 2 and `thomas_place` == 1 and `duel_won_Thomas` == 1 → `thomas_heal_after`
- if `arc1_stage` == 2 and `thomas_place` == 1 → `thomas_grounds`
- if `arc1_stage` == 2 → `thomas_s4`
- if otherwise → `thomas_early`

### `thomas_early`

**Thomas** *(focused)*: Kira's off at the training grounds, if you're looking for her.

*Staging:* Thomas plays **Working**

**Missing:** Animation: sharpening a knife - Working stands in. Knife prop.

### `thomas_s4`

**Thomas** *(pleased)*: Ah, there's my favourite pair of troublemakers. Let me guess - Finn's got you heading into the deep woods again?

*Staging:* Thomas turns to face Player

→ `thomas_s4_menu`

### `thomas_s4_menu`

**Thomas**: Well?

1. We'll be careful, Thomas. → `thomas_s4_a1`
2. Kira says you know the area well. → `thomas_s4_a2`
3. Why don't you come with us? → `thomas_s4_a3`

### `thomas_s4_a1`

**Thomas** *(chuckling)*: You always are. But 'careful' and 'safe' aren't the same thing in those woods.

→ `thomas_s4_end`

### `thomas_s4_a2`

**Thomas** *(nodding)*: Used to. These old joints don't handle the underbrush like they used to.

→ `thomas_s4_end`

### `thomas_s4_a3`

**Thomas** *(rueful)*: I wish I could! But my knees creak louder than rusty hinges. I'd only slow you young ones down.

→ `thomas_s4_end`

### `thomas_s4_end`

**Thomas** *(serious)*: Something's been stirring up the wildlife lately - animals acting more aggressive than natural.

→ `thomas_s4_end_2`

### `thomas_heal_before`

**Thomas**: Here - drink this first. I'd rather you start the lesson whole.

*Staging:* Thomas turns to face Player

*Effects:* gives Healing Potion x1

→ `thomas_heal_before_drink`

**Missing:** Animation: handing over the potion.

### `thomas_heal_before_drink`

*Staging:* Player plays **Drinking** (the line waits for it), then Idle

*Effects:* takes Healing Potion x1; restParty()

→ `thomas_grounds_start`

### `thomas_grounds_start`

**Thomas**: Now. Feet, sword, breath. Show me.

*Effects:* startDuel(name=Thomas)

### `thomas_s4_end_2`

**Thomas**: If you're both set on this, you'd better know how to defend yourselves. Come with me to the training grounds - we'll start with the basics.

→ `thomas_leaves_cabin`

### `thomas_leaves_cabin`

*Staging:* Thomas walks to ThomasLeavesCabin

→ `thomas_gone_to_grounds`

**Missing:** He walks to the cabin door, then is simply at the training grounds when you get there - staging can't walk anyone between rooms.

### `thomas_gone_to_grounds`

*Effects:* `thomas_place` = 1

### `thomas_grounds`

**Thomas** *(pleased)*: Right, here we are. Kira, you watch - this one's for your friend.

*Staging:* Thomas turns to face Player

→ `thomas_heal_before`

### `thomas_heal_after`

**Thomas**: Easy - you took a few knocks. Drink this.

*Staging:* Thomas turns to face Player

*Effects:* gives Healing Potion x1

→ `thomas_heal_after_drink`

### `thomas_heal_after_drink`

*Staging:* Player plays **Drinking** (the line waits for it), then Idle

*Effects:* takes Healing Potion x1; restParty()

→ `thomas_lesson_done`

### `thomas_lesson_done`

**Thomas** *(winded)*: Not bad for a first go. Now listen, because this next bit matters.

→ `thomas_lesson_done_2`

### `thomas_lesson_done_2`

**Thomas**: There's a thornwolf been skulking about the north edge of the village, out by the Whisperwood gate. Starving, by the look of it - and the hungry ones don't back off.

→ `thomas_lesson_done_5`

### `thomas_lesson_done_5`

**Thomas**: Meet me at the north gate. I'll show you how to deal with it.

*Effects:* `duel_won_Thomas` = 0; `thomas_lesson` = 1

→ `thomas_gone_to_gate`

### `thomas_gone_to_gate`

*Effects:* `thomas_place` = 2

**Missing:** He disappears from the training grounds as the conversation closes - a Path to the grounds' exit would let him walk off first.

### `thomas_gate`

**Thomas** *(serious)*: There it is. See how it keeps to the edge? Starving. A thornwolf waits till you're close, and the moment you move, it bites. You won't beat it to the first hit, so don't try.

*Staging:* Thomas turns to face Player; Player turns to face Thomas

→ `thomas_gate_brace`

### `thomas_gate_brace`

**Thomas**: Instead you [b]Brace[/b]. Tuck in and take the bite on your guard, then hit it while it's pulling back. Catch your breath when it backs off, never when it's on you.

*Effects:* unlockMove(name=Brace)

→ `thomas_gate_move`

### `thomas_gate_move`

[b]New move: Brace[/b] - it blocks hits that land while you're tucked in, best right in the middle.

→ `thomas_gate_kira`

### `thomas_gate_kira`

**Kira** *(eager)*: And I'll shoot it from the back while it's busy chewing on you. Fact.

→ `thomas_gate_alone`

### `thomas_gate_alone`

**Thomas** *(firm)*: Not this time, you won't. This one's theirs - you stay with me.

→ `thomas_gate_noticed`

### `thomas_gate_noticed`

**Thomas** *(urgent)*: Steady - it's seen you. Here it comes! Brace!

*Effects:* chargePlayer()

**Missing:** Wolf art (it wears the Chinlin's sheet) and a growl as it charges. The fight is the player's alone (GameObjects/PerimeterWolf.wren fightsLeaderAlone).

### `thomas_gate_done`

**Thomas** *(impressed)*: Hardly a scratch on you. You braced it, just like I said! Back to the cabin, the pair of you - one last test.

*Staging:* Thomas turns to face Player

→ `thomas_gone_home`

**Missing:** He disappears from the gate as the conversation closes, and is back in his cabin.

### `thomas_gone_home`

*Effects:* `thomas_place` = 0

### `thomas_wolf_wait`

**Thomas**: That wolf's still out by the [b]north gate[/b]. Brace the bite, then hit back. Off you go.

### `thomas_wolf_back`

**Thomas** *(impressed)*: Back already? Good. You did well out there.

→ `thomas_wolf_back_2`

### `thomas_wolf_back_2`

**Kira** *(sulky)*: He braced, just like you said. I'd have shot it, but you made me stand there.

→ `thomas_wolf_back_3`

### `thomas_wolf_back_3`

**Thomas** *(laughing)*: I'm impressed. Truly. But a wolf's one thing - out there you'll meet things that think.

→ `thomas_wolf_back_4`

### `thomas_wolf_back_4`

**Thomas**: So, one last test. The both of you, against me. Don't hold back - I won't.

*Effects:* `thomas_lesson` = 3; startDuel(name=Thomas)

### `thomas_test_again`

**Thomas**: Back for another go? Good. Same as before - the both of you, against me.

*Effects:* startDuel(name=Thomas)

### `thomas_heal_test`

**Thomas**: That's the hardest I've been hit in years. Here - one for you, before anything else.

*Staging:* Thomas turns to face Player

*Effects:* gives Healing Potion x1

→ `thomas_heal_test_drink`

### `thomas_heal_test_drink`

*Staging:* Player plays **Drinking** (the line waits for it), then Idle

*Effects:* takes Healing Potion x1; restParty()

→ `thomas_after`

### `thomas_after`

**Thomas**: Not bad! You two might actually survive out there. Now, about equipment...

→ `thomas_rack`

### `thomas_rack`

*Staging:* Thomas walks to ThomasToRack

→ `thomas_rack_2`

### `thomas_rack_2`

*Staging:* pause 0.6s

→ `thomas_rack_3`

### `thomas_rack_3`

*Staging:* Thomas walks up to the player

→ `thomas_after_2`

### `thomas_after_2`

**Thomas**: Nothing shiny. Shiny's for people who want to be looked at.

*Staging:* Thomas turns to face Kira; Kira turns to face Thomas; Kira plays **Lifting**, then Idle

*Effects:* gives Leather Armor x1

→ `thomas_after_kira`

**Missing:** Animation: handing over (a better bow for Kira, leather armour for you) - Kira's Lifting stands in.

### `thomas_after_kira`

**Kira** *(delighted)*: Oh, she's lovely. Plain wood and everything. Thanks, Dad.

*Staging:* Thomas turns to face Player; Kira plays **Inspecting**

→ `thomas_after_2_2`

**Missing:** Animation: slinging a bow over the shoulder.

### `thomas_after_2_2`

**Thomas** *(earnest)*: One more thing... If you get in real trouble, blow this. It won't bring help, but it might scare off whatever's chasing you.

*Staging:* Kira plays **Idle**

→ `thomas_after_2_3`

**Missing:** Animation: holding out a small horn. Horn prop.

### `thomas_after_2_3`

**Thomas**: Go and see Finn before you leave. He was heading for the [b]village gate[/b].

*Effects:* gives Emergency Horn x1; `arc1_stage` = 3

### `thomas_idle`

**Thomas**: Keep your guard up and your feet moving. And bring my daughter home in one piece.

### `thomas_violent`

**Thomas** *(quiet)*: Kira told me about the guardian. I taught you to defend yourself, not to choose violence first.

### `finn_map_greet`

**Finn** *(proud)*: There you are. I heard from Thomas you passed his training with ease.

*Staging:* Finn turns to face Player; Player turns to face Finn

→ `finn_map_greet_2`

### `finn_map_greet_2`

**Finn** *(warm)*: Don't let it go to your head - a wolf at the edge of the village isn't the deep woods. Come on, I'll see you to the gate.

→ `finn_map_walk`

## The village gate

### `finn_map_walk`

*Staging:* Finn walks to FinnToParty; pause 0.7s

→ `finn_map_walk_2`

### `finn_map_walk_2`

*Staging:* Player walks to PartyToGate

→ `finn_map`

### `finn_map`

**Finn** *(sly)*: Old trader paths. Mostly overgrown. [i]Mostly.[/i]

*Staging:* Player turns to face Finn; Finn turns to face Player

→ `finn_map_kira`

**Missing:** Animation: holding out / handing over the map.

### `finn_map_kira`

*Staging:* Kira walks to KiraToMap

→ `finn_map_2`

### `finn_map_2`

**Kira**: These paths go places I've never seen. How do you know them?

*Staging:* Kira turns to face Finn; Finn turns to face Kira; Kira plays **Inspecting**

→ `finn_map_3`

### `finn_map_3`

**Finn**: I'm old. Old people know paths. It's most of what we're for.

*Staging:* Kira plays **Idle**; Finn turns to face Player

*Effects:* gives Old Map x1

→ `finn_map_doors`

### `finn_map_doors`

*Staging:* Finn walks to FinToDoors

→ `finn_map_doors_2`

### `finn_map_doors_2`

*Staging:* Finn turns to face up; Finn plays **Working** (the line waits for it), then Idle

→ `finn_map_doors_3`

### `finn_map_doors_3`

*Staging:* Finn walks to FinnStepAside

*Effects:* openDoor(name=WisperWoodsDoor); openDoor(name=WisperWoodsDoor2)

→ `finn_away`

### `wisperwood_door`

Brambles choke the old trail into Whisperwood.  *(kept - read in the world)*

### `finn_away`

**Finn**: Follow the map. And come home safe - both of you.

*Staging:* Finn turns to face Player

→ `finn_leaves`

### `finn_leaves`

*Staging:* Finn walks to FinnHome

→ `finn_gone`

### `finn_gone`

*Effects:* `arc1_stage` = 4

## Whisperwood

### `kira_stones`

**Kira** *(uneasy)*: It's always like this past the stones. Dad says the trees out here are just sulking.

*Staging:* Kira turns to face Player

### `claw_marks_spotted`

**Kira**: Wait - over there. Look at that tree.

→ `claw_marks_walk`

### `claw_marks_walk`

*Staging:* Kira walks to KiraToClawMarks; Player turns to face Kira

→ `claw_marks_look`

### `claw_marks_look`

**Kira**: Wait. See that? Scratch marks - fresh, and deep. Dad says the wolves have been getting bolder. Fact. Probably.

*Staging:* Kira plays **Inspecting**; Kira turns to face Player

## The Big Book of Facts

### `big_book`

*Staging:* Kira turns to face Player; Kira plays **Inspecting**

→ `big_book_menu`

**Missing:** Kira should sit on a rock and draw with charcoal - a sitting-and-drawing clip (Inspecting stands in, standing). A rock to sit on near the fight.

### `big_book_menu`

**Kira** *(concentrating)*: Hold still. I'm getting this down while it's fresh.

1. What are you doing? → `big_book_a1`
2. You can't write? → `big_book_a2`

### `big_book_a1`

**Kira**: Adding a fact. Wolves bite first. Block, then bonk. That's the wolf. That's you blocking. That's the bonk.

*Staging:* close-up: `CloseUps/kira_book_wolf_fact.png`

→ `big_book_2`

**Missing:** Close-up art: Kira's drawing of the wolf fact (placeholder CloseUps/kira_book_wolf_fact.png).

### `big_book_a2`

**Kira**: I can draw. Drawing's better. Writing's just drawing for people with no imagination.

*Staging:* close-up: `CloseUps/kira_book_wolf_fact.png`

→ `big_book_2`

### `big_book_2`

**Kira** *(quietly pleased)*: That's the first one in here that I found out myself. All the others are Dad's, from the priest.

→ `big_book_menu2`

### `big_book_menu2`

**Kira** *(content)*: There.

1. It's a good fact. → `big_book_b1`
2. (Say nothing.) → `big_book_b2`

### `big_book_b1`

**Kira**: It's a great fact. It's probably the best one in the book.

*Staging:* the close-up closes; Kira plays **Idle**

### `big_book_b2`

*Staging:* the close-up closes; Kira plays **Idle**

### `claw_marks`

**Kira**: Wait. See that? Scratch marks - fresh, and deep. Dad says the wolves have been getting bolder. Fact. Probably.

*Staging:* Kira plays **Inspecting**

## The road to the shrine

### `blue_lights`

**Kira** *(quiet)*: Those lights... they're not fireflies. Fireflies don't hum. ...I'm not drawing that in the book until I know what it is.

### `stone_pillars`

*Staging:* close-up: `CloseUps/shrine_pillar_carvings.png`; pause 1.5s

→ `stone_pillars_2`

**Missing:** Close-up art: the pillar carvings (placeholder CloseUps/shrine_pillar_carvings.png).

### `stone_pillars_2`

**Kira**: Look. Nobody's kneeling. Nobody's carrying a whip. They just look... normal. The priest said that without lords watching, people starve. These ones don't look starved.

→ `stone_pillars_3`

### `stone_pillars_3`

**Kira**: Huh. Maybe the priest never came this far either.

### `tablet_1`

"In the beginning, all stood as one, / United beneath the equal sun..."  *(kept - read in the world)*

*Staging:* close-up: `CloseUps/tablet_circle.png`

**Missing:** Close-up art for all five tablets (placeholders CloseUps/tablet_circle/triangle/square/diamond/star.png).

### `tablet_2`

"From unity comes the strength to grow, / Triangle's peak shall higher go..."  *(kept - read in the world)*

*Staging:* close-up: `CloseUps/tablet_triangle.png`

### `tablet_3`

"Upon this base, four corners strong, / Square foundations shall last long..."  *(kept - read in the world)*

*Staging:* close-up: `CloseUps/tablet_square.png`

### `tablet_4`

"Precious gem of varied face, / Diamond holds each person's place..."  *(kept - read in the world)*

*Staging:* close-up: `CloseUps/tablet_diamond.png`

### `tablet_5`

"Above all else, hope's light shall shine, / Star eternal, truth divine..."  *(kept - read in the world)*

*Staging:* close-up: `CloseUps/tablet_star.png`

## The shrine door

### `shrine_door`

- if `arc1_stage` >= 5 → `shrine_door_open`
- if otherwise → `shrine_pedestals`

### `shrine_pedestals_fail`

The symbols fade. Try again.  *(kept - read in the world)*

### `shrine_pedestals_done`

*Staging:* the camera shakes

*Effects:* openDoor(name=ShrineEntranceDoor); `arc1_stage` = 5

### `shrine_pedestals_step1`

Press which pedestal first?  *(kept - read in the world)*

1. Circle → `shrine_pedestals_step2`
2. Triangle → `shrine_pedestals_fail`
3. Square → `shrine_pedestals_fail`
4. Diamond → `shrine_pedestals_fail`
5. Star → `shrine_pedestals_fail`

### `shrine_pedestals_step2`

Then?  *(kept - read in the world)*

1. Circle → `shrine_pedestals_fail`
2. Triangle → `shrine_pedestals_step3`
3. Square → `shrine_pedestals_fail`
4. Diamond → `shrine_pedestals_fail`
5. Star → `shrine_pedestals_fail`

### `shrine_pedestals_step3`

Then?  *(kept - read in the world)*

1. Circle → `shrine_pedestals_fail`
2. Triangle → `shrine_pedestals_fail`
3. Square → `shrine_pedestals_step4`
4. Diamond → `shrine_pedestals_fail`
5. Star → `shrine_pedestals_fail`

### `shrine_pedestals_step4`

Then?  *(kept - read in the world)*

1. Circle → `shrine_pedestals_fail`
2. Triangle → `shrine_pedestals_fail`
3. Square → `shrine_pedestals_fail`
4. Diamond → `shrine_pedestals_step5`
5. Star → `shrine_pedestals_fail`

### `shrine_pedestals_step5`

And last?  *(kept - read in the world)*

1. Circle → `shrine_pedestals_fail`
2. Triangle → `shrine_pedestals_fail`
3. Square → `shrine_pedestals_fail`
4. Diamond → `shrine_pedestals_fail`
5. Star → `shrine_pedestals_done`

### `shrine_pedestals`

[PH] Five stone pedestals ring the vine-covered door: Circle, Triangle, Square, Diamond and Star. The tablets around the courtyard might tell you the order.  *(kept - read in the world)*

→ `shrine_pedestals_step1`

### `shrine_door_open`

The shrine door stands open.  *(kept - read in the world)*

## Inside the shrine

### `shrine_entry_kira`

**Kira** *(awed)*: By the gods... it's massive. And old. Older than anything I've ever seen.

→ `shrine_entry_kira_2`

### `shrine_entry_kira_2`

**Kira**: The poem was telling a story about how communities grow and thrive. It's nothing like the priests' stories about divine mandate.

### `shifting_floor`

**Kira** *(startled)*: [PH] This passage looks like- ...That was close. The floor almost split us up! Let's stick together.

*Staging:* the camera shakes

**Missing:** A rumble sound with the camera shake.

### `kira_split`

*Staging:* Kira is placed at KiraBehindTheWall; the camera shakes

*Effects:* closeDoor(name=ShrineSplitWall)

→ `kira_split_2`

### `kira_split_2`

*Staging:* Player turns to face down

→ `kira_split_3`

**Missing:** Sounds: the plate's click and the slab slamming down.

### `kira_split_3`

**Kira** *(flustered)*: I'm fine! I'm fine. I've got two rolls and a strong will. Meet you in the middle!

*Effects:* leaveParty(name=Kira); `kira_separated` = 1; setSpawnPoint(iid=fb8791e3-e3c1-11f1-9dc9-28d043bb8e9f, level=ShrineChamber)

### `split_wall`

Solid stone. No answer from the other side.  *(kept - read in the world)*

**Missing:** Animation: knocking on the wall.

### `kira_reunion`

*Staging:* pause 0.2s

*Effects:* joinParty(name=Kira, at=KiraTumblesIn); `kira_separated` = 0; openDoor(name=ShrineSplitWall)

→ `kira_reunion_walk`

### `kira_reunion_walk`

*Staging:* Player turns to face right; Kira walks up to the player (run)

→ `kira_reunion_2`

### `kira_reunion_2`

**Kira** *(breathless)*: There you are! You look weird. Glowy weird.

*Staging:* Player turns to face Kira

→ `kira_reunion_menu`

### `kira_reunion_menu`

**Kira** *(squinting)*: Hold still. What's that glow?

1. There's a voice in my head. → `kira_reunion_a1`
2. Where did you go? → `kira_reunion_a2`

### `kira_reunion_a1`

**Kira**: Oh, that's normal in shrines. Fact.

### `kira_reunion_a2`

**Kira**: The long way round. That wall went back up as soon as I got off the plate. ...I ate one of the rolls. For strength.

## The runic constructs

### `construct_1`

- if `duel_won_RunicConstuct1` == 1 → `construct_1_after`
- if otherwise → `construct_1_wake`

### `construct_1_wake`

*Staging:* the camera shakes

*Effects:* startDuel(name=RunicConstuct1)

### `construct_1_after`

*Staging:* the camera shakes

*Effects:* triggerEvent(event=Defeat RunicConstuct1)

### `construct_2`

- if `duel_won_RunicConstruct2` == 1 → `construct_2_after`
- if otherwise → `construct_2_wake`

### `construct_2_wake`

*Staging:* the camera shakes

*Effects:* startDuel(name=RunicConstruct2)

### `construct_2_after`

*Staging:* the camera shakes

*Effects:* triggerEvent(event=Defeat RunicConstuct2)

### `construct_3`

- if `duel_won_RunicConstruct3` == 1 → `construct_3_after`
- if otherwise → `construct_3_wake`

### `construct_3_wake`

*Staging:* the camera shakes

*Effects:* startDuel(name=RunicConstruct3)

### `construct_3_after`

*Staging:* the camera shakes

*Effects:* triggerEvent(event=Defeat RunicConstuct3); `arc1_stage` = 6

## The Heart

### `heart`

*Staging:* Player turns to face ShrineCrystal; a soul fragment drifts from ShrineCrystal into the player

→ `heart_2`

### `heart_2`

*Staging:* the camera shakes

→ `heart_3`

### `heart_3`

**Guide**: Oh! Oh, that's... hello. You're warm. Are you warm? I think you're warm. I don't actually know what warm is.

→ `heart_menu`

### `heart_menu`

**Guide** *(delighted)*: So... hello?

1. Who are you? → `heart_a1`
2. Get out of my head. → `heart_a2`

### `heart_a1`

**Guide**: I was hoping you'd know. I've got... a long table. People laughing. Someone passing bread. That's all of me there is.

→ `heart_memory`

### `heart_a2`

**Guide**: Fair, completely fair, but I don't think I can. I'll be quiet. ...What did you have for breakfast?

→ `heart_memory`

### `heart_memory`

*Staging:* close-up: `CloseUps/memory_long_table.png`; pause 3s

→ `heart_4`

**Missing:** Close-up art: the long table memory (placeholder CloseUps/memory_long_table.png).

### `heart_4`

**Guide**: There's another piece of... something, nearby. Like a word on the tip of a tongue. If I had a tongue. Can we go and get it? Call me the Guide. That sounds helpful.

*Staging:* the close-up closes

*Effects:* `ability_soul_sight` = 1; gives Soul Fragment x1; `arc1_stage` = 7

→ `heart_sight`

### `heart_sight`

[b]Soul Sight[/b] - press [input=SoulSight] to see hidden doors and mechanisms. In a fight, it shows what your foes mean to do.  *(kept - read in the world)*

→ `heart_5`

### `heart_5`

**Guide**: Oh, and I think I can help you move. Like a step, but... slippier. Try it next time something big swings at you.

*Effects:* unlockMove(name=Soul Step)

→ `heart_step`

### `heart_step`

[b]New move: Soul Step[/b] - a quick step in any direction that nothing can hit for a moment.  *(kept - read in the world)*

### `hidden_door`

**Guide**: Oh - sorry. Sorry! I didn't mean to stare.

→ `hidden_door_2`

### `hidden_door_2`

**Guide** *(sheepish)*: You were doing a very good job of being a wall. Honestly. Very convincing.

→ `hidden_door_3`

### `hidden_door_3`

*Effects:* openDoor(name=ShrineHiddenDoor)

**Missing:** An effect for the dark wall folding itself away (the door just opens).

### `memory_door`

**Guide**: It's stuck. There's something about this floor, though... Try looking at it the other way. The soul way. [input=SoulSight]

### `shrine_crystal`

- if `arc1_stage` >= 7 → `shrine_crystal_dim`
- if otherwise → `heart`

### `shrine_crystal_glow`

A great crystal hums at the heart of the chamber. Its light feels strangely familiar.  *(kept - read in the world)*

### `shrine_crystal_dim`

The crystal is quiet now, its light gone into you.  *(kept - read in the world)*

## The murals

### `memory_murals`

- if `told_kira` >= 1 → `murals_again`
- if otherwise → `murals_s13`

### `murals_s13`

**Kira**: You look... different. There's something in your eyes. What happened in there?

→ `murals_s13_menu`

### `murals_s13_menu`

**Kira**: Tell me.

1. Tell her about the soul fragment. → `murals_told`
2. Keep it to yourself for now. → `murals_secret`

### `murals_told`

**Kira** *(amazed)*: A divine spirit? But the priests say the gods only speak through official channels...

*Effects:* `told_kira` = 1

→ `murals_paint`

### `murals_secret`

**Kira**: Well, whatever happened, I'm glad you're safe.

*Effects:* `told_kira` = 2

→ `murals_paint`

### `murals_paint`

**Kira**: Look at these paintings! These people had no kings, no nobles. How did they organize anything?

→ `murals_paint_menu`

### `murals_paint_menu`

**Kira**: What do you think?

1. Maybe they didn't need kings. → `murals_end`
2. They seem to have managed just fine. → `murals_end`
3. What if the stories we've been told are wrong? → `murals_wrong`

### `murals_wrong`

**Kira** *(thoughtful)*: That's a dangerous thought. But looking at this place... maybe dangerous thoughts are what we need.

→ `murals_end`

### `murals_end`

**Kira**: Whatever this place is, it's not finished with us yet.

### `murals_again`

**Kira**: Leaders chosen by merit, and rotated... schools for everyone... I keep reading them over.

### `central_mural`

- if `fragment_2` == 1 → `central_mural_empty`
- if otherwise → `central_mural_find`

### `central_mural_find`

*Staging:* Player plays **Inspecting** (the line waits for it), then Idle

→ `central_mural_find_2`

### `central_mural_find_2`

*Staging:* close-up: `CloseUps/memory_six_figures.png`; pause 3.5s

→ `central_mural_find_3`

**Missing:** Close-up art: the six figures and the city of light (placeholder CloseUps/memory_six_figures.png).

### `central_mural_find_3`

**Guide** *(stronger)*: Yes... I remember more. There were six others. They feared what we represented. They feared people would see there was another way.

*Staging:* the close-up closes

*Effects:* gives Soul Fragment x1; `fragment_2` = 1

### `central_mural_empty`

The compartment is empty now.  *(kept - read in the world)*

## The Warden

### `warden`

- if `arc1_stage` >= 8 → `warden_rest`
- if `duel_won_Warden` == 1 → `warden_s16`
- if `arc1_stage` >= 7 → `warden_s14`
- if otherwise → `warden_dormant`

### `warden_dormant`

Three great stone guardians stand motionless before the throne.  *(kept - read in the world)*

### `warden_rest`

The three wardens stand still, their long watch finally over.  *(kept - read in the world)*

### `warden_s14`

**Warden** *(menacing)*: Who... dares... disturb... the... sanctuary?

→ `warden_s14_2`

### `warden_s14_2`

**Kira**: [PH] All three of them! Aim for the joints - that's how father taught me to hunt large game!

*Effects:* startBattle(name=Warden)

### `warden_s16`

**Warden** *(awakening)*: [PH] The light... I remember the light...

→ `warden_s16_2`

### `warden_s16_2`

**Warden**: Bearer of the Light... why do you carry the Master's essence? I was meant to guard this place from those who would exploit its knowledge. But you...

→ `warden_s16_menu`

### `warden_s16_menu`

**Warden**: Speak.

1. I want to restore what was lost. → `warden_p1`
2. I seek to understand the truth. → `warden_p2`
3. I carry the soul of the one you served. → `warden_p3`
4. [Strike it down while it is weak] → `warden_violent`

### `warden_p1`

**Warden**: Restoration... yes. The Master spoke of this possibility before the end came.

→ `warden_peace`

### `warden_p2`

**Warden**: Truth. The rarest treasure of all. The six who came before sought only power.

→ `warden_peace`

### `warden_p3`

**Warden** *(reverent)*: Master... even in fragments, your light shines true.

→ `warden_peace`

**Missing:** Animation: the Warden kneeling (it has no overworld sheet of its own - it wears the wrestler's).

### `warden_peace`

**Warden**: I have failed in my duty. Much of the knowledge I was meant to protect has been lost. But not all.

→ `warden_peace_2`

### `warden_peace_2`

**Warden**: Take these. They contain not just power, but memory. Memory of how things could be.

*Staging:* the camera shakes

*Effects:* gives Soul Fragment x1; gives Ancient Knowledge Codex x1; gives Guardian's Key x1; `warden_peaceful` = 1; `arc1_stage` = 8; triggerEvent(event=Defeat Warden)

**Missing:** The hidden mechanism in the throne (only a camera shake plays).

### `warden_violent`

→ `warden_violent_2`

**Missing:** The final blow - nothing shows the blade finding the crack now.

### `warden_violent_2`

**Kira** *(troubled)*: We... we could have talked to it. I heard it trying to speak near the end. Why did we have to...?

*Effects:* gives Soul Fragment x1; gives Guardian's Key x1; `warden_violent` = 1; `arc1_stage` = 8; triggerEvent(event=Defeat Warden)

## The memory viewer

### `memory_viewer`

- if `arc1_stage` >= 9 → `viewer_done`
- if otherwise → `viewer_s17`

### `viewer_s17`

*Staging:* Player plays **Inspecting** (the line waits for it), then Idle

→ `viewer_s17_2`

### `viewer_s17_2`

*Staging:* close-up: `CloseUps/memory_city_height.png`; pause 3s

→ `viewer_s17_2b`

**Missing:** Close-up art for the three memory beats (placeholders CloseUps/memory_city_height.png, memory_war.png, memory_equalitas.png).

### `viewer_s17_2b`

*Staging:* close-up: `CloseUps/memory_war.png`; pause 3s

→ `viewer_s17_2c`

### `viewer_s17_2c`

*Staging:* close-up: `CloseUps/memory_equalitas.png`; pause 3s

→ `viewer_s17_3`

### `viewer_s17_3`

**Guide**: Now you understand. The histories were rewritten. The truth was buried. But fragments remain, waiting for those brave enough to seek them.

*Staging:* the close-up closes

→ `viewer_s17_4`

### `viewer_s17_4`

**Kira** *(overwhelmed)*: Everything we were taught... the kingdoms, the gods we worship... they destroyed something beautiful.

→ `viewer_s17_5`

### `viewer_s17_5`

*Effects:* gives Fragment Map x1; gives Ancient Records x1; `arc1_stage` = 9

**Missing:** The "go home and talk to Finn" hint is gone with the narration - the quest log has to carry it.

### `viewer_done`

The device is quiet now.  *(kept - read in the world)*

## The wounded wolf

### `wounded_wolf`

[PH] A Thornwolf lies in the undergrowth, whimpering. With your new sight you can see the corruption burning through it - it isn't vicious. It's in pain.  *(kept - read in the world)*

1. Use Empathic Healing. → `wolf_heal`
2. Leave it be. → `wolf_leave`

### `wolf_heal`

→ `wolf_heal_2`

**Missing:** A healing effect from your hands, and the wolf licking your hand and padding away.

### `wolf_heal_2`

**Kira**: You can heal them? That's incredible! No wonder they were acting strange - they were suffering!

*Effects:* `healed_wolf` = 1

### `wolf_leave`

**Kira** *(sad)*: Poor thing.

## Home - Finn's spar and the truth

### `finn_spar`

**Finn** *(searching)*: You found it. And something else found you, I think.

*Staging:* Finn plays **Inspecting**; Finn turns to face Player

→ `finn_spar_2`

### `finn_spar_2`

**Finn**: Pick up that sword. Properly. Let's see what Thomas taught you.

*Staging:* Finn plays **Idle**

*Effects:* `finn_spar` = 1; startDuel(name=Finn)

### `finn_spar_again`

**Finn**: Again. Sword up. Hit, then block - you know this.

*Staging:* Finn turns to face Player

*Effects:* startDuel(name=Finn)

### `finn_spar_after`

**Finn**: Not bad. Not bad at all.

*Staging:* Finn turns to face Player

→ `finn_spar_after_2`

### `finn_spar_after_2`

**Finn** *(wistful)*: I made this a long time ago. I dabbled in fighting when I was young. Silly dreams. Every boy has them.

*Staging:* Player plays **Lifting**, then Idle

*Effects:* gives Finn's Blade x1

→ `finn_spar_after_3`

**Missing:** Animation: handing over Finn's blade.

### `finn_spar_after_3`

**Kira**: You caught my arrow with tongs.

→ `finn_spar_after_4`

### `finn_spar_after_4`

**Finn**: Lucky. Bad back.

→ `finn_spar_after_5`

### `finn_spar_after_5`

**Kira**: It's warm. Is it meant to be warm?

*Staging:* Kira plays **Inspecting**

→ `finn_spar_after_6`

### `finn_spar_after_6`

**Finn** *(evasive)*: It's been near the forge.

*Staging:* Kira plays **Idle**

*Effects:* `finn_spar` = 2

→ `finn_reveal_2`

### `finn_reveal`

**Finn** *(knowing)*: Welcome back. I can see from your eyes that you found more than metal fragments.

*Staging:* Finn turns to face Player

→ `finn_reveal_2`

### `finn_reveal_2`

**Finn**: I suppose it's time for truth. [color=#7fb6ff]I've been waiting seventy years for someone like you to appear.[/color]

→ `finn_reveal_3`

### `finn_reveal_3`

→ `finn_reveal_menu`

### `finn_reveal_menu`

**Finn**: ...

1. Seventy years? You're not that old. → `finn_r1`
2. You've been waiting for me specifically? → `finn_r2`
3. [Say nothing, but look skeptical] → `finn_r3`

### `finn_r1`

**Finn** *(chuckling)*: Ah, you caught that, did you? Sharp eyes... or perhaps something more than eyes now.

→ `finn_rev_b`

### `finn_r2`

**Finn** *(mysterious)*: Not you personally, but someone like you. Someone who could carry the burden.

→ `finn_rev_b`

### `finn_r3`

**Finn** *(approving)*: I see you're learning to read between the lines. Good.

→ `finn_rev_b`

### `finn_rev_b`

**Finn**: What I said wasn't entirely false, just... incomplete. What happened to you in that shrine was no accident. Use those abilities wisely - not everyone appreciates having their words examined.

→ `finn_reveal_menu2`

### `finn_reveal_menu2`

**Finn**: Ask.

1. You're one of them, aren't you? A follower of Equalitas. → `finn_q1`
2. How long have you known what I would find? → `finn_q2`
3. The whole village knows, don't they? → `finn_q3`

### `finn_q1`

**Finn** *(sad smile)*: The last, I believe. Though I pray there are others, hidden in distant places.

→ `finn_rev_c`

### `finn_q2`

**Finn**: I knew when I first saw you. There was something familiar about your spirit - something that reminded me of the old days.

→ `finn_rev_c`

### `finn_q3`

**Finn**: Some suspect. Most simply live the old ways without knowing why. Haven is... special. We've preserved what we could, even if we couldn't preserve the memory of why.

→ `finn_rev_c`

### `finn_rev_c`

**Kira** *(realising)*: The village... the way we share everything... this isn't normal, is it? Other places aren't like this.

→ `finn_rev_c_2`

### `finn_rev_c_2`

**Finn**: Your generation was raised in our ways without knowing their origin. Natural kindness, unforced by doctrine.

→ `finn_rev_c_3`

### `finn_rev_c_3`

**Finn**: [PH] This hammer was forged in the last days of the war. It can shape metal touched by divine essence. One day soon, it will strengthen your blade.

*Staging:* Finn plays **PickingUp** (the line waits for it), then Idle

→ `finn_rev_c_4`

**Missing:** Animation: opening a hidden compartment in the forge - PickingUp stands in.

### `finn_rev_c_4`

**Finn**: But know this - the six who rule now have agents everywhere. They watch for the old power returning. You will be hunted.

→ `finn_reveal_menu3`

### `finn_reveal_menu3`

**Finn**: What will you do?

1. Then we'll have to be stronger than they are. → `finn_h1`
2. Maybe it's time people knew the truth. → `finn_h2`
3. Are you saying I should hide this power? → `finn_h3`

### `finn_h1`

**Finn**: Strength isn't always about power, young one. Sometimes it's about knowing when to stand and when to move wisely.

→ `finn_rev_d`

### `finn_h2`

**Finn**: The truth is dangerous. But lies are poison that spreads with time. Perhaps you're right.

→ `finn_rev_d`

### `finn_h3`

**Finn**: I'm saying you should be careful. Not that you should do nothing.

→ `finn_rev_d`

### `finn_rev_d`

**Finn**: The soul of Equalitas was shattered into seven pieces. You carry three of them now. The other four lie hidden in the domains of the six false gods.

→ `finn_rev_d_2`

### `finn_rev_d_2`

**Kira**: These locations... they're in the heart of enemy territory. The Iron Theocracy, the Merchant Republic...

*Staging:* Kira plays **Inspecting**

→ `finn_path`

### `finn_path`

**Finn**: It will not be easy.

*Staging:* Kira plays **Idle**

1. We have to try. People deserve to know the truth. → `finn_p1`
2. Maybe we should gather allies first. → `finn_p2`
3. This is bigger than I thought. I need time to think. → `finn_p3`

### `finn_p1`

**Finn**: Brave, but dangerous. The direct path often leads to martyrdom.

*Effects:* `path_forward` = 1

→ `finn_reveal_end`

### `finn_p2`

**Finn**: Wisdom. There are others who preserve fragments of the old ways. Finding them will be crucial.

*Effects:* `path_forward` = 2

→ `finn_reveal_end`

### `finn_p3`

**Finn**: Time is a luxury, but wisdom... wisdom is a necessity.

*Effects:* `path_forward` = 3

→ `finn_reveal_end`

### `finn_reveal_end`

**Finn**: Whatever path you choose, you won't walk it alone. Rest now. Talk to Kira in the morning - she has a lot to think about too.

*Effects:* `arc1_stage` = 10

### `finn_reveal_done`

**Finn**: Kira was out by the well, last I saw. Go to her.

## Kira at the well

### `kira_s21`

**Kira** *(pensive)*: I've been thinking all night about what we saw. My whole life, I believed the priests - that some people were born to rule and others to serve. But those murals...

→ `kira_s21_menu`

### `kira_s21_menu`

**Kira**: ...

1. What do you believe now? → `kira_21a`
2. It's hard when everything you've been taught is wrong. → `kira_21b`
3. You don't have to decide everything right now. → `kira_21c`

### `kira_21a`

**Kira** *(firm)*: I believe people should be free to help each other without someone above them taking credit. I believe knowledge should be shared, not hoarded.

→ `kira_s21_end`

### `kira_21b`

**Kira** *(nodding)*: It is. But maybe it's also liberating? Like I can finally ask the questions I was afraid to ask before.

→ `kira_s21_end`

### `kira_21c`

**Kira** *(smiling)*: You're right. But I know enough to know I want to help. Whatever comes next, I'm with you.

→ `kira_s21_end`

### `kira_s21_end`

**Kira** *(determined)*: I want to see more of those places. And if there are people suffering under lies... well, I'm a pretty good shot. Come on - Finn's gathering everyone for supper.

*Staging:* Kira plays **Shoot**, then Idle

*Effects:* `arc1_stage` = 11

## The farewell

### `finn_farewell`

- if `warden_violent` == 1 → `farewell_violent`
- if otherwise → `farewell_1`

### `farewell_violent`

**Finn** *(grave)*: Some of the villagers say they can feel something dark about you since you came back. Remember who you want to be.

→ `farewell_1`

### `farewell_1`

**Finn** *(proud)*: Our young friends will be leaving us soon. They go to seek truth in a world that has forgotten it.

→ `farewell_1_2`

**Missing:** Animation: Finn standing to address the gathering.

### `farewell_1_2`

**Finn** *(warm)*: [PH] Take these - supplies for the road, and a crystal you can use to reach me. Remember: you carry more than power. You carry hope.

*Effects:* gives Communication Crystal x1; gives Healing Herb x3

→ `farewell_menu`

### `farewell_menu`

**Finn**: Go well.

1. I won't let you down. → `farewell_2`
2. I'll find the other fragments. → `farewell_2`
3. I'll come back when this is over. → `farewell_2`

### `farewell_2`

**Finn** *(proud)*: You already make me proud. Now go, and show the world what's possible.

→ `road_1`

**Missing:** Animation: a hand on your shoulder.

### `road_1`

**Kira** *(eager)*: So... where do we go first?

→ `road_choice`

### `road_choice`

**Kira**: [PH] The world map would open here.

1. The Merchant Republic - investigate economic oppression. → `road_end_1`
2. The Scholar Kingdom - seek hidden knowledge. → `road_end_2`
3. The Warrior Clans - face direct conflict. → `road_end_3`
4. Follow the fragment resonance. → `road_end_4`

### `road_end_1`

*Effects:* `act2_path` = 1; endArc()

**Missing:** The narrated ending is gone - the end-of-arc card follows the choice straight away. An epilogue close-up could take its place.

### `road_end_2`

*Effects:* `act2_path` = 2; endArc()

### `road_end_3`

*Effects:* `act2_path` = 3; endArc()

### `road_end_4`

*Effects:* `act2_path` = 4; endArc()
