// Combat/Move.wren
// One move of the animation-timed battle system: a data record tied to one animation. Every time is in seconds from
// the start of the move and never changes - no weapon, armour, status or effect may scale a move's timing.
//
//   types       any mix of Attack / Defend / Dodge / Heal, plus optional movement
//   completion  the whole animation; impact - when damage / healing lands; damageAnim - how long this move's own
//               damage animation plays when its user is staggered
//   aim         "reach" (the cell N ahead in the lane), "first" (the nearest opponent ahead in the lane), "around"
//               (a stencil centred on the user) or "self"; the stencil lists the other cells covered and the share
//               of damage / healing / stagger each takes: [depth, side, share] - depth along the user's facing,
//               side across lanes - relative to the aim cell (reach / first) or to the user (around)
//   defend      a window (start, peak, end): defence ramps 0 -> 100% at the peak -> 0; a hit inside it skips the move
//               forward by `skip`; guardRange lists ally cells it also covers, [depth, side, share] from the user
//   dodge       a window (start, end): nothing can hit the user inside it
//   movement    at moveTime the user moves to the cell picked when the chain was built; `pattern` says which cells
//               may be picked, `blocked` what happens if the cell is taken by then ("stay" / "short")
//   reaction    how the move takes a hit: "default", "steadfast" (half set-back), "unflinching" (no set-back after
//               impact), "deflect" (a block at the peak sets the attacker back), "absorb" (no skip; stagger -> stamina)
//
// Moves are built by Combat/Config/Moves.wren with the fluent setters below (each returns the move). A move comes in
// up to three tiers (I early, II mid, III late): the tier scales base damage / heal / defence x1 / x1.5 / x2, adds
// 0.05 s / 0.10 s of stagger and costs x1 / x1.25 / x1.5 stamina - the animation and every time stay the same.
var TIER_SCALE = [1, 1, 1.5, 2]
var TIER_STAGGER = [0, 0, 0.05, 0.10]
var TIER_COST = [1, 1, 1.25, 1.5]
var TIER_SUFFIX = ["", "", " II", " III"]

class Move {
    // id: the move's catalog name ("Slash"); tier 1-3; completion in seconds; baseCost: tier I stamina before tax.
    construct new(id, tier, completion, baseCost) {
        _id = id
        _tier = tier
        _name = id + TIER_SUFFIX[tier]
        _completion = completion
        _cost = (baseCost * TIER_COST[tier]).round
        _damageAnim = completion >= 1.6 ? 0.4 : 0.3
        _weapons = null
        _scaling = "attack"
        _anim = "attack"
        _reaction = "default"
        _hold = false

        _attack = false
        _impact = null
        _damage = 0
        _stagger = 0
        _aim = "self"
        _reach = 0
        _stencil = []
        _push = 0
        _backstab = 1

        _heal = false
        _healAmount = 0

        _defend = false
        _defence = 0
        _addsShield = false
        _window = null
        _skip = 0
        _guardRange = []

        _dodge = null

        _moveTime = null
        _pattern = null
        _blocked = "stay"

        _exposed = null
        _special = null
        _heavy = false
        _item = null
    }

    id { _id }
    tier { _tier }
    name { _name }
    // What battle logs and tells call it - a monster's reskin keeps the base move's numbers under its own name.
    rename(name) {
        _name = name
        return this
    }
    completion { _completion }
    cost { _cost }
    damageAnim { _damageAnim }
    weapons { _weapons }
    scaling { _scaling }
    anim { _anim }
    reaction { _reaction }
    isHold { _hold }

    isAttack { _attack }
    isHeal { _heal }
    isDefend { _defend }
    isDodge { _dodge != null }
    isMovement { _moveTime != null }
    impact { _impact }
    damage { _damage }
    stagger { _stagger }
    aim { _aim }
    reach { _reach }
    stencil { _stencil }
    push { _push }
    backstab { _backstab }
    healAmount { _healAmount }
    defence { _defence }
    addsShield { _addsShield }
    window { _window }
    skip { _skip }
    guardRange { _guardRange }
    dodge { _dodge }
    moveTime { _moveTime }
    pattern { _pattern }
    blocked { _blocked }
    exposed { _exposed }
    special { _special }
    heavy { _heavy }
    item { _item }

    // --- builders (each returns this) --------------------------------------------------

    weapons(tags) {
        _weapons = tags
        return this
    }
    mana() {
        _scaling = "mana"
        return this
    }
    anim(role) {
        _anim = role
        return this
    }
    damageAnim(seconds) {
        _damageAnim = seconds
        return this
    }
    costFixed(cost) {
        _cost = cost
        return this
    }
    react(reaction) {
        _reaction = reaction
        return this
    }

    // impact time, tier I damage and stagger (scaled by the tier).
    attack(impact, baseDamage, baseStagger) {
        _attack = true
        _impact = impact
        _damage = baseDamage * TIER_SCALE[_tier]
        _stagger = baseStagger + TIER_STAGGER[_tier]
        return this
    }
    // The same without tier scaling - boss and item moves, authored with their final numbers.
    attackFlat(impact, damage, stagger) {
        _attack = true
        _impact = impact
        _damage = damage
        _stagger = stagger
        return this
    }
    heal(impact, baseHeal) {
        _heal = true
        _impact = impact
        _healAmount = baseHeal * TIER_SCALE[_tier]
        return this
    }
    healFlat(impact, amount) {
        _heal = true
        _impact = impact
        _healAmount = amount
        return this
    }
    reach(n) {
        _aim = "reach"
        _reach = n
        return this
    }
    first() {
        _aim = "first"
        return this
    }
    self() {
        _aim = "self"
        return this
    }
    // cells: [[depth, side, share], ...] around the user (depth -1 is the cell behind).
    around(cells) {
        _aim = "around"
        _stencil = cells
        return this
    }
    around8(share) {
        var cells = []
        for (d in -1..1) {
            for (s in -1..1) {
                if (d != 0 || s != 0) cells.add([d, s, share])
            }
        }
        return around(cells)
    }
    // Extra cells past the aim cell (reach / first): the range-modifier rule - the higher share per cell wins.
    cell(depth, side, share) {
        for (c in _stencil) {
            if (c[0] == depth && c[1] == side) {
                if (share > c[2]) c[2] = share
                return this
            }
        }
        _stencil.add([depth, side, share])
        return this
    }
    depth(share) { cell(1, 0, share) }
    sides(share) {
        cell(0, -1, share)
        return cell(0, 1, share)
    }
    diagonals(share) {
        cell(1, -1, share)
        return cell(1, 1, share)
    }
    push(cells) {
        _push = cells
        return this
    }
    backstab(multiplier) {
        _backstab = multiplier
        return this
    }

    // window start / peak / end, tier I defence at the peak (scaled by the tier), skip time.
    defend(start, peak, end, baseDefence, skip) {
        _defend = true
        _window = [start, peak, end]
        _defence = baseDefence * TIER_SCALE[_tier]
        _skip = skip
        return this
    }
    defendFlat(start, peak, end, defence, skip) {
        _defend = true
        _window = [start, peak, end]
        _defence = defence
        _skip = skip
        return this
    }
    skip(seconds) {
        _skip = seconds
        return this
    }
    // The shield's guard bonus is added to this move's defence, and its guard range to this move's.
    withShield() {
        _addsShield = true
        return this
    }
    guard(depth, side, share) {
        _guardRange.add([depth, side, share])
        return this
    }
    guardSides(share) {
        guard(0, -1, share)
        return guard(0, 1, share)
    }

    dodge(start, end) {
        _dodge = [start, end]
        return this
    }

    // pattern: "ortho" (4 neighbours), "any" (8), "diag", "fwdback", "forward", "back", "forward2", "blink" (any free
    // cell within 2), "toHeart" (boss shards). blocked: "stay" or "short" (stop at the last free cell on the way).
    movement(moveTime, pattern, blocked) {
        _moveTime = moveTime
        _pattern = pattern
        _blocked = blocked
        return this
    }

    // A boss move's Exposed window (seconds from the move's start): the part takes x1.5 and drops its guard.
    exposed(from, to) {
        _exposed = [from, to]
        return this
    }
    special(tag) {
        _special = tag
        return this
    }
    // When a move with no attack or heal does its thing (a special's effect time).
    at(time) {
        _impact = time
        return this
    }
    // A slow, marked hit - its target cells are shaded from the moment the chain is committed.
    heavy() {
        _heavy = true
        return this
    }
    // The inventory item id an item move consumes at its effect time.
    item(id) {
        _item = id
        return this
    }

    // Hold: no types, no cost, no tax; stamina only recovers here. One per length.
    static hold(seconds) {
        var m = Move.new("Hold", 1, seconds, 0)
        m.holdSetup_(seconds)
        return m
    }
    holdSetup_(seconds) {
        _hold = true
        _anim = "idle"
        _name = "Hold"
        _damageAnim = 0.3
    }

    // A Falter: the 0.5 s stumble a move turns into when its stamina can't be paid.
    static falter() { Move.new("Falter", 1, 0.5, 0).anim("hurt") }

    // --- queries -------------------------------------------------------------------------

    // The defensive curve at `t` seconds into the move: 0 outside the window, 1 at the peak, linear between.
    guardAt(t) {
        if (_window == null || t < _window[0] || t > _window[2]) return 0
        if (t <= _window[1]) {
            var span = _window[1] - _window[0]
            return span <= 0 ? 1 : (t - _window[0]) / span
        }
        var tail = _window[2] - _window[1]
        return tail <= 0 ? 1 : (_window[2] - t) / tail
    }

    dodgingAt(t) { _dodge != null && t >= _dodge[0] && t <= _dodge[1] }
    exposedAt(t) { _exposed != null && t >= _exposed[0] && t <= _exposed[1] }

    // "Attack + Dodge + Move" - for menus and tells.
    typeLine {
        var parts = []
        if (_attack) parts.add("Attack")
        if (_defend) parts.add("Defend")
        if (_dodge != null) parts.add("Dodge")
        if (_heal) parts.add("Heal")
        if (_moveTime != null) parts.add("Move")
        if (_hold) parts.add("Hold")
        return parts.count == 0 ? "-" : parts.join(" + ")
    }

    static fmt(seconds) {
        var tenths = (seconds * 10).round
        return "%((tenths / 10).floor).%(tenths % 10)"
    }

    // "Slash  1.0s" - the move-menu button (the cost is the user's, with tax - the menu adds it).
    menuLabel { "%(_name)  %(Move.fmt(_completion))s" }
}
