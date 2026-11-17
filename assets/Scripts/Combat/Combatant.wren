// Combat/Combatant.wren
// One participant in a battle. The persistent part - name, Stats (HP carries between fights for the party), Loadout
// (Combat/Gear.wren), the moves it brings (up to six, plus Hold and Use Item which everyone always has) and the
// overworld entity it came from - and the per-fight state the engine (Combat/BattleEngine.wren) runs on: where it
// stands, which way it faces, the chain it has committed to, how far into the current move it is, its statuses and
// the little counters gear effects keep. beginBattle() resets all of the latter.
import "Combat/Stats" for Stats
import "Combat/Gear" for Loadout

// One step of a chain: the move, and for a movement move the cell it was aimed at when the chain was built - either
// a resolved [lane, row] (the player's pick) or a hint the AI resolves when the move starts ("toward", "away",
// "side", "forward", "back", "heart").
class ChainEntry {
    construct new(move, dest) {
        _move = move
        _dest = dest
    }
    move { _move }
    dest { _dest }
    dest=(v) { _dest = v }
}

class Combatant {
    construct new(name, stats, loadout, moves, entity) {
        _name = name
        _stats = stats
        _loadout = loadout == null ? Loadout.bare : loadout
        _moves = moves
        _entity = entity
        _read = 0
        _traits = {}
        _def = null
        _brain = null
        _side = 0
        beginBattle(0, 0, 0, 1)
    }

    name { _name }
    name=(v) { _name = v }
    stats { _stats }
    stats=(v) { _stats = v }
    loadout { _loadout }
    loadout=(v) { _loadout = v == null ? Loadout.bare : v }
    moves { _moves }
    moves=(value) { _moves = value }
    entity { _entity }
    alive { _stats.alive }

    // How much of each enemy's chain this combatant's side gets to see: 0 the active move, 1 + the next, 2 all of it.
    read { _read }
    read=(v) { _read = v }

    // The battle def this combatant was minted from (art, combo menu) and the AI that picks its chains (null = a
    // human picks).
    def { _def }
    def=(v) { _def = v }
    brain { _brain }
    brain=(v) { _brain = v }

    // Free-form rules a def or boss hangs on a combatant:
    //   "passive"      never acts on its own (the practice stump); hits on it count as hits on a Hold
    //   "noStamina"    ignores stamina entirely (bosses)
    //   "granite"      [defence, defence while Exposed, share of a hit that gets through outside an Exposed window]
    //                  replacing its Defence stat (the Warden's Granite Skin)
    //   "pool"         the Combatant whose HP this part's damage also comes off (boss shards)
    //   "reactive"     a Fn(move id) -> Move it starts whenever it is hit while idle (the swing-post's arm)
    //   "natural"      [power, staggerMul] of a monster's natural weapon, used by every one of its moves
    //   "element"      a monster's attack element         "bodyElement"  a monster's body element (its "armour")
    //   "bleed"        its damaging hits add a Bleed stack
    //   "immovable"    can't be pushed or thrown
    traits { _traits }
    trait(key) { _traits.containsKey(key) ? _traits[key] : null }
    has(key) { _traits.containsKey(key) }

    // --- per-fight state ---------------------------------------------------------------

    // side: 0 the party (advances toward higher rows), 1 the opposition. facing: +1 / -1 along the rows.
    beginBattle(side, lane, row, facing) {
        _side = side
        _lane = lane
        _row = row
        _facing = facing
        _chain = []
        _chainIndex = 0
        _move = null
        _entry = null
        _progress = 0
        _stun = 0
        _impactDone = false
        _moved = false
        _claim = null
        _cells = []
        _lastWasHold = false
        _staggerTimes = []
        _animImmuneUntil = 0
        _cost = 0
        _startedAfterHold = false
        _hitLanded = false

        _burn = 0
        _burnTime = 0
        _burnSource = null
        _burnRate = 0
        _chill = 0
        _chillTime = 0
        _chillSource = null
        _chillStrength = 0
        _frozen = 0
        _bleed = 0
        _bleedTime = 0

        _shield = 0
        _shieldTime = 0
        _counters = {}
        _needsChain = true
        _removed = false
    }

    side { _side }
    lane { _lane }
    row { _row }
    lane=(v) { _lane = v }
    row=(v) { _row = v }
    facing { _facing }
    facing=(v) { _facing = v }
    at(lane, row) { _lane == lane && _row == row }

    // The committed chain and where in it the combatant is. move: the active Move (null between chains / passive).
    chain { _chain }
    chainIndex { _chainIndex }
    chainIndex=(v) { _chainIndex = v }
    move { _move }
    entry { _entry }
    progress { _progress }
    progress=(v) { _progress = v }
    // Seconds of damage animation left to play - the move doesn't advance while this runs.
    stun { _stun }
    stun=(v) { _stun = v }
    impactDone { _impactDone }
    impactDone=(v) { _impactDone = v }
    moved { _moved }
    moved=(v) { _moved = v }
    // The cell this combatant's movement move has claimed ([lane, row]) until it arrives.
    claim { _claim }
    claim=(v) { _claim = v }
    // The cells the active move will hit: [[lane, row, share, isAimCell], ...] - locked when the move starts.
    cells { _cells }
    cells=(v) { _cells = v }
    lastWasHold { _lastWasHold }
    lastWasHold=(v) { _lastWasHold = v }
    staggerTimes { _staggerTimes }
    animImmuneUntil { _animImmuneUntil }
    animImmuneUntil=(v) { _animImmuneUntil = v }
    // What the active move cost when it started (Tempo refunds a share of it).
    cost { _cost }
    cost=(v) { _cost = v }
    startedAfterHold { _startedAfterHold }
    startedAfterHold=(v) { _startedAfterHold = v }
    hitLanded { _hitLanded }
    hitLanded=(v) { _hitLanded = v }
    // True while this combatant has no chain left and must be given one.
    needsChain { _needsChain }
    needsChain=(v) { _needsChain = v }
    // Taken off the board without dying (a boss shard folded back into the statue).
    removed { _removed }
    removed=(v) { _removed = v }
    active { alive && !_removed }

    setChain(entries) {
        _chain = entries
        _chainIndex = 0
        _needsChain = false
    }

    // The engine starts / clears the active move through these.
    startMove(entry, move) {
        _entry = entry
        _move = move
        _progress = 0
        _impactDone = false
        _moved = false
        _hitLanded = false
        _cells = []
    }
    clearMove() {
        _entry = null
        _move = null
        _progress = 0
        _cells = []
        _claim = null
    }

    inHold { _move == null || _move.isHold }
    // Seconds left of the active move (0 with none).
    remaining { _move == null ? 0 : _move.completion - _progress + _stun }
    guardNow { _move == null ? 0 : _move.guardAt(_progress) }
    dodgingNow { _move != null && _stun <= 0 && _move.dodgingAt(_progress) }
    exposedNow { _move != null && _move.exposedAt(_progress) }
    // Past its impact, or past its defensive window: "in recovery".
    inRecovery {
        if (_move == null || _move.isHold) return false
        if (_move.impact != null) return _impactDone
        if (_move.window != null) return _progress > _move.window[2]
        return false
    }

    // --- statuses (only these persist on a target; everything else resolves on the hit) ---------------------

    burn { _burn }
    burn=(v) { _burn = v }
    burnTime { _burnTime }
    burnTime=(v) { _burnTime = v }
    burnSource { _burnSource }
    burnSource=(v) { _burnSource = v }
    burnRate { _burnRate }
    burnRate=(v) { _burnRate = v }
    chill { _chill }
    chill=(v) { _chill = v }
    chillTime { _chillTime }
    chillTime=(v) { _chillTime = v }
    chillSource { _chillSource }
    chillSource=(v) { _chillSource = v }
    chillStrength { _chillStrength }
    chillStrength=(v) { _chillStrength = v }
    // 0, or the multiplier the next set-back taken is scaled by.
    frozen { _frozen }
    frozen=(v) { _frozen = v }
    bleed { _bleed }
    bleed=(v) { _bleed = v }
    bleedTime { _bleedTime }
    bleedTime=(v) { _bleedTime = v }
    clearStatuses() {
        _burn = 0
        _burnTime = 0
        _chill = 0
        _chillTime = 0
        _frozen = 0
        _bleed = 0
        _bleedTime = 0
    }

    // An overheal shield (Light matched set, Bloodthirst capstone): soaks damage first, fades after `shieldTime`.
    shield { _shield }
    shield=(v) { _shield = v }
    shieldTime { _shieldTime }
    shieldTime=(v) { _shieldTime = v }

    // Gear-effect scratch, by name (Momentum streaks, Storm Charges, a pending Reprisal...). 0 when never set.
    counter(key) { _counters.containsKey(key) ? _counters[key] : 0 }
    setCounter(key, value) { _counters[key] = value }
    addCounter(key, amount, max) {
        var v = counter(key) + amount
        if (max != null && v > max) v = max
        _counters[key] = v
        return v
    }
}
