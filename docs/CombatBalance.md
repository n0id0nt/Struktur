# Combat balance

Damage and health are whole numbers small enough to count hits: a quick tier I attack does **1**, a longer one
**2**, so a 3 HP thornwolf takes three Slashes and the player (5 HP) takes five bites. To ask for a change, add a
line starting with `> EDIT:` under the row or section, like the dialogue markup.

Where the numbers live:

- Move damage, healing and blocks: `assets/Scripts/Combat/Config/Moves.wren`
- Tier multipliers: `assets/Scripts/Combat/Move.wren` (`TIER_SCALE`)
- Health and creature moves: `Combat/Config/BattlePlayer.wren`, `Combat/Config/BattleCritter.wren`, `Combat/Boss/Warden.wren`
- Weapon power and shield guard: `Combat/Config/GearTypes.wren`
- Chain length: `Combat/BattleEngine.wren` (`CHAIN_BUDGET`)

## Rules

| Rule | Value |
|---|---|
| Chain length | **3 s** of moves per chain (was 4 s) |
| Tier I damage | under 1.2 s: **1** - 1.2 to 1.9 s: **2** - 2 s or more: **3** |
| Tier II / III | x2 / x3 the tier I number (was x1.5 / x2) |
| Blocks (Defend moves) | the same scale - subtracted from a hit at the guard's strength (full at the peak, ramping either side) |
| Weapon power (added to every hit) | **0** for every weapon lighter than a long sword; **+1** for long sword, claymore, spear, pike, longbow, warbow, crossbow, magic staff. Mid-quality gear x2, late x4 |
| Shield guard (added to Block / Shield Wall / Buckler Parry / Phalanx Step) | buckler 0, heater 1, tower 2 |
| Attack / mana / defence stats | **0** everywhere, so a move's number is what lands (they still exist - mid/late gear and effects can raise them) |
| Item healing | a share of max HP, rounded: Healing Herb 30% (2 of 5), Healing Potion 50% (3 of 5), Special Ration 40% (2) |
| Armour defence | still a percentage cut (`damage x 100 / (100 + defence)`); at these sizes it almost never changes a hit - see *Open questions* |

## Health

| Who | HP | Notes |
|---|---|---|
| Player | **5** | |
| Kira | **4** | |
| Starving Thornwolf | **3** | three Slashes / three Quick Shots |
| Corrupted Thornwolf | 5 | |
| Chicken | 2 | |
| Chinlin | 6 | |
| Twisted Sprite | 3 | |
| Runic Construct 1 / 2 / 3 | 5 / 6 / 7 | the shrine's three, tougher each time (7 elsewhere) |
| Stone Guardian | 10 | natural weapon +1 |
| Desperate Raider | 6 | |
| Thomas (the test, vs player + Kira) | 8 | |
| Finn (spar) | 10 | his Slash does 1 |
| Villager / Duelist | 5 / 6 | battle test roster |
| The Warden | **14** pool, side shards **4** each | Granite Skin: outside an Exposed window half a hit gets through (rounded - a 1 does 0, a 2 does 1); Exposed hits x1.5 |
| Lesson dummy (Thomas) | never drops | `endless` - hits land and show, health never goes down |
| Practice Stump / Swing-post / Snap-post | 8 / 6 / 30 | battle test only now - the snap-post is gone from the lesson |
| Battle-test Training Dummy | never drops | |
| Mid / late tier combatants (battle test `Stats.tier`) | 9 / 14 | |

Hits to kill at tier I (no blocks, no armour):

| Attacker → target | Thornwolf (3) | Kira (4) | Player (5) | Construct 1 (5) | Thomas (8) | Warden (14, Exposed) |
|---|---|---|---|---|---|---|
| Slash / Quick Shot / Bite (1) | 3 | 4 | 5 | 5 | 8 | 7 (each Exposed hit does 2) |
| Long attack - Overhead Cleave, Aimed Shot, Rune Slam (2) | 2 | 2 | 3 | 3 | 4 | 5 (3 each) |

## Moves

Damage I / II / III. "-" means the move doesn't exist at that tier (it unlocks later). Times never change between
tiers.

### Universal

| Move | Time | Damage | Block | Notes |
|---|---|---|---|---|
| Step | 0.6 s | - | - | movement |
| Roll / Soul Step | 0.8 s | - | - | movement + dodge |
| Brace | 1.1 s | - | 2 / 4 / 6 | also covers the ally behind - a Brace I soaks a bite (1) from half guard up, a long hit (2) at its peak |
| Shove | 0.8 s | 1 / 2 / 3 | - | heavy stagger; pushes at III |
| Back Kick | 0.9 s | 1 / 2 / 3 | - | hits behind |

### Blades

| Move | Time | Damage | Block | Notes |
|---|---|---|---|---|
| Stab | 0.5 s | 1 / 2 / 3 | - | |
| Backstab | 0.8 s | 1 / 2 / 3 | - | x2 from behind (x2.5 at III or with a stiletto) |
| Slip Strike | 0.9 s | 1 / 2 / 3 | - | diagonal step + dodge |
| Throwing Knife | 0.7 s | 1 / 2 / 3 | - | first in lane |
| Parry | 0.6 s | - | 3 / 6 / 9 | narrow window |
| **Slash** | 1.0 s | **1** / 2 / 3 | - | the starter attack; sides 25% at III |
| Lunge | 1.1 s | 1 / 2 / 3 | - | step in + dodge |
| Riposte | 0.8 s | - | 3 / 6 / 9 | |
| Pommel Strike | 0.7 s | 1 / 1 / 2 | - | little damage, hard stagger - the interrupt |
| Cross Cut | 1.3 s | 2 / 4 / 6 | - | sides 50% (75% at III) |
| Spin Slash | 1.5 s | - / 4 / 6 | - | all 8 cells at 60-70% |
| Overhead Cleave | 1.7 s | 2 / 4 / 6 | - | |
| Half-sword Thrust | 1.2 s | 2 / 4 / 6 | - | depth 40% |
| Hanging Guard | 1.4 s | - | 3 / 6 / 9 | |
| Wide Sweep | 1.9 s | 2 / 4 / 6 | - | sides 80-100% |
| Crushing Overhead | 2.4 s | 3 / 6 / 9 | - | |
| Whirlwind | 2.2 s | - / 6 / 9 | - | all 8 cells at 70% |
| Blade Block | 1.6 s | - | 3 / 6 / 9 | |

### Shields

| Move | Time | Damage | Block | Notes |
|---|---|---|---|---|
| Block | 1.2 s | - | 2 / 4 / 6 | + shield guard |
| Shield Bash | 1.1 s | 1 / 2 / 3 | 1 / 2 / 3 | |
| Shield Charge | 1.2 s | 2 / 4 / 6 | 1 / 2 / 3 | |
| Shield Wall | 2.0 s | - | - / 6 / 9 | + shield guard, covers the sides |
| Buckler Parry | 0.6 s | - | 3 at every tier | + shield guard |
| Guarded Thrust | 1.3 s | 2 / 4 / 6 | 2 / 4 / 6 | |
| Counter Cut | 1.4 s | 2 / 4 / 6 | 2 / 4 / 6 | |
| Phalanx Step | 0.9 s | - | 2 / 4 / 6 | + shield guard |

### Spears

| Move | Time | Damage | Block | Notes |
|---|---|---|---|---|
| Thrust | 1.2 s | 2 / 4 / 6 | - | depth 50-75% |
| Long Thrust | 1.3 s | 2 / 4 / 6 | - | reach 2 |
| Set Spear | 1.4 s | - | 2 / 4 / 6 | |
| Twin Cleave | 1.3 s | - / 4 / 6 | - | front and back |
| Vaulting Thrust | 1.2 s | - / 4 / 6 | - | |

### Bows and crossbow

| Move | Time | Damage | Block | Notes |
|---|---|---|---|---|
| **Quick Shot** | 0.8 s | **1** / 2 / 3 | - | Kira's starter |
| Aimed Shot | 1.6 s | 2 / 4 / 6 | - | |
| Retreating Shot | 1.0 s | 1 / 2 / 3 | - | steps back |
| Volley | 2.0 s | - / 6 / 9 | - | reach 2, depth + sides 60-75% |
| Point-blank Shot | 0.7 s | 1 / 2 / 3 | - | |
| Bolt | 0.6 s | 2 / 4 / 6 | - | needs a loaded bolt |
| Reload | 1.2 s | - | - / - / 2 | |

### Fists

| Move | Time | Damage | Block | Notes |
|---|---|---|---|---|
| Jab | 0.45 s | 1 / 2 / 3 | - | |
| Hook | 0.8 s | 1 / 2 / 3 | - | |
| Uppercut | 1.1 s | 2 / 4 / 6 | - | heavy-hitting despite the time (gauntlets) |
| Palm Strike | 0.9 s | 1 / 2 / 3 | - | pushes |
| Catch | 0.6 s | - | 2 / 4 / 6 | |
| Throw | 1.3 s | - / 2 / 3 | - | throws behind you |

### Magic

| Move | Time | Damage | Heal / Block | Notes |
|---|---|---|---|---|
| Spark | 0.7 s | 1 / 2 / 3 | - | |
| Arcane Bolt | 1.4 s | 2 / 4 / 6 | - | |
| Mend | 1.2 s | - | heal 1 / 2 / 3 | caster + the ally ahead |
| Ward | 1.2 s | - | block 2 / 4 / 6 | absorbs |
| Blast | 1.8 s | 2 / 4 / 6 | - | |
| Mending Circle | 1.8 s | - | heal 2 / 4 / 6 | all around |
| Barrier | 1.8 s | - | block - / 6 / 9 | everyone around |
| Staff Strike | 0.8 s | 1 / 2 / 3 | - | |
| Sweep | 1.2 s | 2 / 4 / 6 | - | sides 60% |

### Creature and boss moves

| Who | Move | Damage | Notes |
|---|---|---|---|
| Thornwolves | Bite (a Stab) | 1 | Pounce (a Lunge) 1 |
| Chicken | Peck (a Stab) | 1 | |
| Chinlin | Gore (a Lunge) 1, Butt (a Shove) 1, Stomp (an Overhead Cleave) 2 | | |
| Runic Construct | Rune Slam 2, Stone Sweep 2 | | |
| Stone Guardian | Stone Slam 3, Tremor 3 | natural weapon +1 | |
| Twisted Sprite | Pixie Bolt (a Spark) | 1 | |
| The Warden | Stone Fist **3**, Ground Slam 2, Shard Slam 2, Heart Pulse 2 (1 on its ring at 50%) | slow and marked - Brace or step out | |
| Swing-post / Snap-post | Arm Swing 2 / Snap 1 | battle test | |
| Fire Flask (item) | 2 + Burn | | |

## Open questions

- **Armour** is still a percentage cut, which barely touches 1-3 damage hits - leather (8) turns a 2 into 1.85, still
  2. Should armour become a flat point of reduction (with "never below 1" unless blocked), or stay out of the small
  numbers?
- **Statuses** (Burn, Bleed, Thermal Shock) are shares of max HP per second - they scale with the new health on their
  own, but haven't been looked at against 5 HP.
- **Stamina** costs and recovery are unchanged - with 3 s chains a full chain spends less, so fights may feel
  more breathless or less; worth a playtest.
