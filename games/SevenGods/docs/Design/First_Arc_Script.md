# Seven Gods RPG — First Arc Script

## About this script

The prologue in Haven, from waking up in Finn's workshop to crossing the stones into the grey world. It follows the beats of the original tutorial: the blacksmith's errand, Kira, the hunter's training, the shrine, the trap, the fusion, the boss room, and the hand-in. It matches the Revised Plot Outline and the boss doc.

What's new compared with the old first-arc script:

- All dialogue is new.

- The battle system is taught step by step. The player and Kira start with just two moves each. Thomas teaches a third, and three lessons build the basic rhythms: get close and hit, then block, and block, then punish.

- No blessings and no worship. Haven has no elemental items at all; Thomas has never owned one.

- The Guide isn't obviously the seventh god. After hearing Finn's story, he decides he must have been the seventh god's disciple, and says he'll never be as powerful as the great one. The player takes it as modesty.

## Battle tutorial plan

## Starting moves

Everyone also always has Hold (stand and catch your breath; the only way to recover stamina, never twice in a row) and Use Item. Chains follow the battle system: pick moves until the chain reaches about 4 seconds. The move that crosses 4 s is the last one, and nothing cancels.

| Who | Move | Type | Takes | Key time | Notes |
| --- | --- | --- | --- | --- | --- |
| Player | Step | Move | 0.6 s | moves at 0.3 s | 1 cell in any direction |
| Player | Slash (one- handed sword) | Attack | 1.0 s | hits at 0.5 s | Reach 1 (the cell in front) |
| Kira | Step | Move | 0.6 s | moves at 0.3 s | 1 cell in any direction |
| Kira | Quick Shot (bow) | Attack | 0.8 s | hits at 0.4 s | First: hits the first enemy down the lane |
| Both, from Thomas in lesson 2 | Brace | Defend | 1.1 s | blocks from 0.1 to 0.8 s, best at 0.5 s | Tuck in and take the hit |
| Player, from the fusion (scene 9) | Soul Step | Move + Dodge | 0.8 s | moves at 0.3 s, dodges 0.0 to 0.3 s | 1 cell in any direction; can't be hit while dodging |

## The three lessons

Lesson 1: get close, then hit (Thomas's practice stump, scene 4). The stump doesn't fight back. It teaches that attacks only reach the next cell, that a chain fills about 4 seconds, and what Hold is for.

- Player: Step (0.0) → Slash (0.6, hits 1.1) → Slash (1.6, hits 2.1) → Hold 0.8 s (2.6) → Slash (3.4, hits 3.9). 4.4 s.

- Kira: Quick Shot (0.0) → Quick Shot (0.8) → Hold 0.8 s (1.6) → Quick Shot (2.4) → Quick Shot (3.2). 4.0 s.

Lesson 2: hit, then block (the swing-post, scene 4). Every time it's hit, the swing-post's arm swings back at the cell in front of it 0.8 s later. Thomas teaches Brace here. The lesson: after you attack, block, in case they hit back.

- Player: Step (0.0) → Slash (0.6, hits 1.1) → Brace (1.6, blocks 1.7 to 2.4; the arm lands at 1.9) → Slash (2.7, hits 3.2) → Brace (3.7, blocks 3.8 to 4.5; the arm lands at 4.0). 4.8 s.

Lesson 3: block, then punish (the first wolves, scene 6). A starving thornwolf bites the instant it starts a chain next to you, then circles, then leaps back. The lesson: if a wolf is beside you, open with Brace, then hit it while it circles.

- Player, when a wolf is next to them: Brace (0.0, blocks 0.1 to 0.8; the bite lands at 0.25) → Slash (1.1, hits 1.6, while it circles) → Hold 1.2 s (2.1, it leaps away) → Brace (3.3, ready for it coming back). 4.4 s.

- Kira, from the back row: the lesson 1 chain, shooting the wolf while it circles.

## The tutorial wolf: Starving Thornwolf

A starving version of the Corrupted Thornwolf from the enemy doc: 30 HP, no pack bonus, and two combos.

| Combo | When | Chain (start times) | Read it, answer it |
| --- | --- | --- | --- |
| Stalk | Opener, and whenever no one is next to it | Lope (0.0) → Circle 1.2 s (0.6) → Lope (1.8) → Circle 1.2 s (2.4) → Lope (3.6). 4.2 s. | It closes in one cell at a time. Hit it while it circles, or wait for it to come to you. |
| Snap | Whenever someone is next to it when it starts a chain | Bite (0.0, lands 0.25) → Circle 1.2 s (0.5) → Leap Back (1.7) → Circle 1.2 s (2.5) → Lope (3.7). 4.3 s. | It bites straight away. Brace, then hit it in the Circle (0.5 to 1.7 s) before it leaps back. |

Lope is a Step, Bite a Stab, Leap Back a Roll, and Circle is its Hold. All chains follow the enemy rules: at least 2 moves, about 4 s, and no Hold after a Hold.

## Scene 1: The workshop

Morning. The player comes downstairs. Finn is turning the player's latest horseshoe over in his hands, humming four slow notes.

**FINN:** Hm. Hmmm. (holds it up to the light) It's a horseshoe. I'll give you that. A horse might even agree.

**FINN:** No, it's good. Better than mine were at your age. Don't tell anyone I said that, I've a reputation for grumpiness to keep up.

A hammer slides off the bench. Finn catches it without looking, then notices he's done it.

**FINN:** ...Reflexes. From all the, ah, dropping. Ow. My back.

**FINN:** Now. An errand. There's an old shrine past the stones in Whisperwood, and in it there's some old metalwork I've wanted for years. Bring it back and I'll stop pretending your horseshoes are bad.

- "What kind of metalwork?" FINN: Old. Older than me, which is saying something. You'll know it when you see it. It'll be the only thing down there worth carrying. "Why don't you go?" FINN: My back. (he says this while lifting an anvil) Terrible back. "Past the stones? Nobody goes past the stones." FINN: Which is why the metalwork's still there. Take Kira. She'll know the way, or she'll claim to, which is nearly as good.

He hands over a short practice sword and three healing herbs. Quest: "Old Metalwork".

## Scene 2: Haven

The village is absurdly green: flowers in every crack, fruit on every tree. Villagers say hello to the player by name.

MARTA (baker): Off past the stones? Take rolls. No, more rolls. You'll come back thin, everyone does.

GARRETT (at the well, optional puzzle): Blasted rope's tangled again. Sixty years I've pulled this bucket and it's never once been grateful.

## Scene 3: Kira

At the training ground, Kira hits a bullseye, then eats an apple she'd balanced on the target.

**KIRA:** You're going past the stones? I'm going past the stones. Finn sent word. I've been waiting all morning, which is why I'm on my third apple.

**KIRA:** Here's what you need to know about Whisperwood. Fact one: the trees can hear you, so don't insult them. Fact two: thornwolves can smell lies, which is why they always bite the liar first. Fact three: if you get lost, follow the moss. It always points to the nearest pie.

- "Where are these facts from?" KIRA: (pats a battered book on her belt) The Big Book of Facts. A priest gave it to Dad. It's full of them.

- "Have you read it?" KIRA: I don't need to read it, I know what's in it. Dad told me. (she's holding it upside down)

Kira joins the party.

## Scene 4: Thomas teaches you to fight

Thomas's yard: a rotten stump, a post with a swinging wooden arm, and Thomas sharpening an ordinary knife on an ordinary stone.

**THOMAS:** Past the stones, is it? Good. Brave. Probably fine. Did Kira tell you about the moss?

**THOMAS:** Right. Before you go, you'll learn to fight properly. Now, I've never owned one of those fancy glowing blades the lords carry. Never needed one. You don't need a magic sword if you know when to swing a plain one.

### Lesson 1: the stump

**THOMAS:** Everything you do in a fight takes time, and so does everything they do. Nobody can hurry that. Not you, not a wolf, not a lord. So you pick a few moves at a time, and then you watch them play out.

[PROMPT: Choose moves until the chain bar fills (about 4 seconds). The fight runs until someone's chain ends, then you choose again.]

**THOMAS:** That sword only reaches the spot right in front of you. So first you Step up, then you Slash. Go on. Get close, then hit.

[PROMPT: Step moves you one cell in any direction. Slash hits the cell in front of you.] Suggested chain: Step, Slash, Slash, Hold, Slash. The player's first chain plays out on the stump.

**THOMAS:** Good! See that bit where you stood there puffing? That's Hold. It's the only way you get your wind back. Use it when nobody's swinging at you. Never twice in a row, mind: you'll fall asleep.

**KIRA:** (shooting the stump from across the yard) And I don't have to Step at all. Fact.

**THOMAS:** Your arrows fly till they hit something, love. His sword doesn't. (to the player) So she shoots from the back, and you go to them.

[PROMPT: Quick Shot hits the first enemy in Kira's lane, however far away.]

### Lesson 2: the swing-post

Thomas pats the post with the wooden arm.

**THOMAS:** Now this one hits back. Whack it and the arm comes round a breath later. So here's a new trick. Tuck in and take it. (he hunches his shoulders and plants his feet) Brace. [NEW MOVE: Brace. Kira learns it too. Brace blocks hits that land while you're tucked in, best right in the middle.]

**THOMAS:** Get close. Hit it. Then Brace, before the arm comes round. Hit, then block. Hit, then block. That's how you stay standing.

Suggested chain: Step, Slash, Brace, Slash, Brace. The arm lands in the middle of each Brace.

If the player Slashes twice without Bracing, the arm catches them:

**THOMAS:** (wincing) Ooh. You swung, it swung back. Never just keep swinging. Hit, then block.

On success:

**THOMAS:** That's it! Big swing, little wait, tuck in. You'll be fine. (beat) Probably.

## Gear and the horn

**THOMAS:** (gives Kira a better bow, plain wood, and the player a leather vest) Nothing shiny. Shiny's for people who want to be looked at. And take this horn. If you're in trouble, blow it. It won't bring help. But the noise is very satisfying.

## Scene 5: The stones

Finn catches them at the edge of the village with a hand-drawn map.

**FINN:** Old trader paths. Mostly overgrown. Mostly.

**KIRA:** These paths go places I've never seen. How do you know them?

**FINN:** I'm old. Old people know paths. It's most of what we're for.

The party walks past the standing stones, and the colour drains out of the world: grey trees, thin grass, silence. The camera lingers on the line where the green stops.

**KIRA:** It's always like this past the stones. Dad says the trees out here are just sulking.

## Scene 6: Whisperwood and the first wolves (lesson 3)

Two thin, starving thornwolves slink out of the grey scrub. The first real fight.

**KIRA:** Thornwolves! They're so skinny... Remember the post! Get close, hit, block! The wolves Stalk in, one cell at a time. The first time a wolf ends up next to the player and starts a chain, it bites instantly. The player is most likely mid-Slash, and takes the hit.

**KIRA:** It bit you before you'd even moved! It's faster than the post!

[PROMPT: Watch the wolf's timeline. A starving wolf bites the moment it starts a chain next to you. Start your chain with Brace when a wolf is beside you, then Slash it while it circles.]

Suggested chain when a wolf is beside you: Brace, Slash, Hold, Brace. The bite lands in the Brace, and the Slash lands while the wolf circles, just before it leaps back. Kira's arrows hit whichever wolf is in her lane.

The first time the player blocks a bite and lands the punish, Kira whoops.

**KIRA:** That's it! It bites, you block, then you bonk it! Wait, wait, hold still. After the fight. Kira sits on a rock, pulls out the Big Book of Facts and a stub of charcoal, and draws very carefully: a wolf, a little shield, and an arrow pointing to a stick figure hitting the wolf.

- "What are you doing?" KIRA: Adding a fact. Wolves bite first. Block, then bonk. (holds up the drawing) That's the wolf. That's you blocking. That's the bonk.

- "You can't write?" KIRA: I can draw. Drawing's better. Writing's just drawing for people with no imagination.

**KIRA:** (looking at her drawing, quietly pleased) That's the first one in here that I found out myself. All the others are Dad's, from the priest.

The player can choose to say nothing, or:

- "It's a good fact." KIRA: It's a great fact. It's probably the best one in the book.

(It is. Pay this off at the Spire, when she learns to read and finds it's the only true fact in the book.)

## The murals

Optional: the corrupted sprites, as in the old script. On the way to the shrine, old murals show a town going about its day: people building, trading, eating together.

**KIRA:** Look. Nobody's kneeling. Nobody's carrying a whip. They just look... normal. The priest said that without lords watching, people starve. These ones don't look starved.

**KIRA:** Huh. Maybe the priest never came this far either.

## Scene 7: The shrine door

The pedestal and poem puzzle from the old script. When the door opens, it plays four slow notes: the same tune Finn hums.

**KIRA:** ...Huh. That's Finn's song.

## Scene 8: Separated

Kira steps on a plate, and a stone wall drops between them.

**KIRA:** (muffled) I'm fine! I'm fine. I've got two rolls and a strong will. Meet you in the middle!

Solo section: the water and crystal puzzles, then two Runic Constructs. The constructs are slow and heavy, with long marked wind-ups. Alone, the player puts both lessons together: Brace their big hit, then Slash while they recover, and Hold only when neither is swinging. [PROMPT: No one's covering you. Read their timelines, Brace the heavy hit, then punish.]

## Scene 9: The Heart

A chamber of soft blue light. A fragment drifts down to the player and sinks into their chest.

**THE GUIDE:** Oh! Oh, that's... hello. You're warm. Are you warm? I think you're warm. I don't actually know what warm is.

- "Who are you?" THE GUIDE: I was hoping you'd know. I've got... a long table. People laughing. Someone passing bread. That's all of me there is.

- "Get out of my head." THE GUIDE: Fair, completely fair, but I don't think I can. I'll be quiet. (beat) What did you have for breakfast?

Memory 1 plays: the table, the laughter, the bread.

**THE GUIDE:** There's another piece of... something, nearby. Like a word on the tip of a tongue. If I had a tongue. Can we go and get it? Call me the Guide. That sounds helpful. [NEW: Soul Sight (see hidden doors and mechanisms). NEW MOVE: Soul Step, a quick step in any direction that nothing can hit for a moment.]

**THE GUIDE:** Oh, and I think I can help you move. Like a step, but... slippier. Try it next time something big swings at you.

The Guide apologises to the first hidden door before it opens.

## Scene 10: The Warden

Soul Sight leads to the great chamber, and Kira tumbles in through another door.

**KIRA:** There you are! You look weird. Glowy weird.

- "There's a voice in my head." KIRA: Oh, that's normal in shrines. Fact.

The Warden wakes. This is the tutorial lead-up boss from the boss doc: five lessons, with the merged and split forms. The party's whole kit is Step, Slash, Brace, Hold and Soul Step for the player, and Step, Quick Shot, Brace and Hold for Kira. That's everything the fight needs: Brace or Soul Step the slow heavy hits, punish while it's stuck, and never swing into its guard.

Kira's call-outs, each the first time it happens:

- After Stone Fist: KIRA: It's stuck after the punch! Hit it now!

- Into Bastion: KIRA: Don't hit it while it's got its arms up! It's like Dad's swing-post, but grumpier.

- Ground Slam marks the row: KIRA: The floor's glowing! Get off the glowy bit! Sunder: KIRA: It's splitting into three! Watch all of them!

- Reform: KIRA: It's pulling itself back together, and the middle's wide open!

When it falls, it kneels toward the player's chest, toward the fragment.

**THE WARDEN:** (grinding) ...Bearer.

Behind it: the Ember and, wrapped in old cloth, a broken hammer head stamped with an anvil-and-mountain mark (Finn's "old metalwork"). Memory 2: an ordinary, busy city; people arriving with nothing and being given land; nobody bowing; the god carrying bricks.

**THE GUIDE:** I was there. I think I was... someone there. (quietly) Nobody was afraid. Isn't that strange?

## Scene 11: Home, and a sparring match

Back in the workshop, Finn turns the hammer head over in his hands for a long time.

**FINN:** You found it. (looks at the player, really looks) And something else found you, I think.

**FINN:** Pick up that sword. Properly. Let's see what Thomas taught you.

The Master Finn sparring fight (the tutorial's main boss, before Finn's reveal). Finn uses exactly the player's kit (Step, Slash, Brace) and holds back so obviously that the player can see him choosing not to hit them. He leaves big, slow openings, and Brace-punish timing gets rewarded with a nod. Mid-fight, distracted, he blocks one of Kira's arrows with a pair of tongs.

**FINN:** (during) Hit, then block. Good. Block, then hit. Good. Thomas taught you something after all.

**FINN:** (after) Not bad. Not bad at all. (hands over a real blade with the same anvil-and- mountain mark) I made this a long time ago. I dabbled in fighting when I was young. Silly dreams. Every boy has them.

**KIRA:** You caught my arrow with tongs.

**FINN:** Lucky. Bad back.

**KIRA:** (touching the new blade) It's warm. Is it meant to be warm?

**FINN:** (a beat too fast) It's been near the forge.

## Scene 12: The story the Guide is told

Night. The player sits on the workshop roof. Finn climbs up, with no trouble at all.

**FINN:** Your passenger. Can it hear me?

**THE GUIDE:** Tell him yes. Tell him he has a nice voice.

**FINN:** There's a story they don't tell in temples. Once there were seven gods, not six. The seventh never ruled anyone with fear. His people weren't owned, and they weren't afraid. They just... lived. Built things, argued, married, grew old. The six couldn't stand that their own people kept leaving to join him. So they turned on him. Before they could finish it, he broke his soul into pieces and hid them across the world. They've been hunting the pieces ever since.

**THE GUIDE:** (very quietly) ...Oh.

**THE GUIDE:** I think I knew him. I think I... served him. His disciple, maybe? That would explain the bread. (beat) I'll never be what he was, though. Not even close. To the player this plays as modesty. It's actually the most honest thing the Guide says all game: Isaren will never get his old power back. Finn hears it too, and looks at the player for a long moment.

**FINN:** ...Maybe you did know him. (gets up) If those pieces came back together, they say the world might remember how to grow. Just a story. Old men have lots.

Next morning, the village sees them off. Kira shows up with a pack the size of a sheep.

**KIRA:** Somebody has to make sure you eat. Also, Dad says the outside world has a fruit called a "lemon" that's sour on purpose, and I need to know if that's a fact.

**THE GUIDE:** Ask her what a lemon tastes like. No, wait. Find out, then tell me. They cross the stones into the grey. Title card.

## Notes for the other docs

- Step in any direction. The weapons doc's tier I Step only moves forward or back. The starter Step here moves one cell in any direction, which is closer to the tier II pattern. Either the tier I Step changes, or the starter gets its own name.

- Brace is taught by Thomas. The player and Kira start with two moves each, and Brace is their first learned move. That fits the rule that moves are learned from NPCs. Soul Step is the fusion's new move (the original design's "the soul fragment gives the player a new combat ability"). Mechanically it's a Roll that moves one cell in any direction.

- The Starving Thornwolf is a tutorial-only variant. If you'd rather keep a single wolf, give the enemy doc's Corrupted Thornwolf the Snap combo as a positional trigger ("someone next to it at chain start").

- No elemental items in Haven. Finn's warm blade is the first hint that something elemental exists without one of the six behind it.
