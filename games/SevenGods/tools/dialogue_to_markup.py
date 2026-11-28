"""Writes docs/Arc1Dialogue.md - the whole Arc 1 dialogue (every file in assets/Dialogue/Arc1, one per character) as a readable script: every line with its speaker and
expression, the actions and close-ups that play with it, the choices and where they lead, and what's still missing.

    python tools/dialogue_to_markup.py

The game reads assets/Dialogue/Arc1/*.json (the graph, one file per character) and assets/Localization/en.json (the text); this only
reads them. Edit the markdown to mark changes you want, then have them applied to the JSON and re-run this.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DIALOGUE_DIR = ROOT / "assets" / "Dialogue" / "Arc1"
# The files in the order the script lists them (the game's load order - Scripts/Dialogue/DialogueLoader.wren).
DIALOGUE_FILES = ["Finn", "Thomas", "Kira", "Elena", "Garrett", "Child", "Guide", "Warden", "World"]
STRINGS = ROOT / "assets" / "Localization" / "en.json"
OUT = ROOT / "docs" / "Arc1Dialogue.md"

# Where each scene starts (the first node of it in the file).
SCENES = [
    ("rest_spot", "Resting"),
    ("opening", "The opening"),
    ("workshop_stairs", "Finn's workshop - the first talk"),
    ("elena", "Haven - Elena, Garrett, the well and the child"),
    ("kira", "Kira at the training grounds"),
    ("kira_party_stay", "Kira in the party"),
    ("thomas", "Thomas - the cabin, the training grounds and the north gate"),
    ("finn_map_walk", "The village gate"),
    ("kira_stones", "Whisperwood"),
    ("big_book", "The Big Book of Facts"),
    ("blue_lights", "The road to the shrine"),
    ("shrine_door", "The shrine door"),
    ("shrine_entry_kira", "Inside the shrine"),
    ("construct_1", "The runic constructs"),
    ("heart", "The Heart"),
    ("memory_murals", "The murals"),
    ("warden", "The Warden"),
    ("memory_viewer", "The memory viewer"),
    ("wounded_wolf", "The wounded wolf"),
    ("finn_spar", "Home - Finn's spar and the truth"),
    ("kira_s21", "Kira at the well"),
    ("finn_farewell", "The farewell"),
]

# Things a node still needs (art, animation, a marker...) - for you to add.
MISSING = {
    "rest_spot": "A rest-spot sitting pose that fits the spot (a bench / log) - SittingIdle plays where the player stands.",
    "finn_s1": "Horseshoe prop in Finn's hand (he plays Inspecting).",
    "finn_s1_hammer": "Animation: catching (a hammer slides off the bench and Finn catches it without looking) - PickingUp stands in. Hammer prop.",
    "finn_s1_a2": "Anvil prop for Finn's Lifting.",
    "finn_s1_end_2": "Animation: handing over (Finn gives the practice sword and three herbs) - the player's Lifting stands in.",
    "elena_bread": "Animation: handing over (Elena piles rolls into your arms) - the player's Lifting stands in.",
    "elena_rations": "She was written as running up - she's already being talked to, so nothing moves. A Path for her to run in on would restore it.",
    "garrett_ask": "He should face the well while he grumbles - the well isn't a named actor or Marker yet (a Marker called e.g. VillageWell would do).",
    "child_s5": "Animation: tugging your cloak.",
    "child_s5_end": "She should run off - needs a Path / Marker to run to (e.g. ChildRunsOff), then moveTo with speed=run.",
    "kira_early_shot": "Sounds: the bow's twang and the thunk as the arrow hits (the arrow is a particle - GameObjects/ArrowShot.wren - that vanishes in a puff of dust; it doesn't stay stuck in the target).",
    "kira_s3_shot": "Sounds, as above. Animation: eating an apple (she's on her third).",
    "kira_s3_a1": "Animation: patting the book on her belt.",
    "kira_s3_a2": "Close-up art: the Big Book held upside down (placeholder CloseUps/kira_book_upside_down.png).",
    "thomas_early": "Animation: sharpening a knife - Working stands in. Knife prop.",
    "thomas_heal_before": "Animation: handing over the potion.",
    "thomas_leaves_cabin": "He walks to the cabin door, then is simply at the training grounds when you get there - staging can't walk anyone between rooms.",
    "thomas_gone_to_gate": "He disappears from the training grounds as the conversation closes - a Path to the grounds' exit would let him walk off first.",
    "thomas_gate_noticed": "Wolf art (it wears the Chinlin's sheet) and a growl as it charges. The fight is the player's alone (GameObjects/PerimeterWolf.wren fightsLeaderAlone).",
    "thomas_gate_done": "He disappears from the gate as the conversation closes, and is back in his cabin.",
    "thomas_after_2": "Animation: handing over (a better bow for Kira, leather armour for you) - Kira's Lifting stands in.",
    "thomas_after_kira": "Animation: slinging a bow over the shoulder.",
    "thomas_after_2_2": "Animation: holding out a small horn. Horn prop.",
    "finn_map": "Animation: holding out / handing over the map.",
    "big_book": "Kira should sit on a rock and draw with charcoal - a sitting-and-drawing clip (Inspecting stands in, standing). A rock to sit on near the fight.",
    "big_book_a1": "Close-up art: Kira's drawing of the wolf fact (placeholder CloseUps/kira_book_wolf_fact.png).",
    "stone_pillars": "Close-up art: the pillar carvings (placeholder CloseUps/shrine_pillar_carvings.png).",
    "tablet_1": "Close-up art for all five tablets (placeholders CloseUps/tablet_circle/triangle/square/diamond/star.png).",
    "shifting_floor": "A rumble sound with the camera shake.",
    "kira_split_2": "Sounds: the plate's click and the slab slamming down.",
    "split_wall": "Animation: knocking on the wall.",
    "heart_memory": "Close-up art: the long table memory (placeholder CloseUps/memory_long_table.png).",
    "hidden_door_3": "An effect for the dark wall folding itself away (the door just opens).",
    "central_mural_find_2": "Close-up art: the six figures and the city of light (placeholder CloseUps/memory_six_figures.png).",
    "warden_p3": "Animation: the Warden kneeling (it has no overworld sheet of its own - it wears the wrestler's).",
    "warden_peace_2": "The hidden mechanism in the throne (only a camera shake plays).",
    "warden_violent": "The final blow - nothing shows the blade finding the crack now.",
    "viewer_s17_2": "Close-up art for the three memory beats (placeholders CloseUps/memory_city_height.png, memory_war.png, memory_equalitas.png).",
    "viewer_s17_5": "The \"go home and talk to Finn\" hint is gone with the narration - the quest log has to carry it.",
    "wolf_heal": "A healing effect from your hands, and the wolf licking your hand and padding away.",
    "finn_spar_after_2": "Animation: handing over Finn's blade.",
    "finn_rev_c_3": "Animation: opening a hidden compartment in the forge - PickingUp stands in.",
    "farewell_1": "Animation: Finn standing to address the gathering.",
    "farewell_2": "Animation: a hand on your shoulder.",
    "road_end_1": "The narrated ending is gone - the end-of-arc card follows the choice straight away. An epilogue close-up could take its place.",
}

# Lines that read text in the world (signs, tablets, puzzles, tutorials) rather than narrating - kept on purpose.
KEPT = {
    "well_puzzle", "well_puzzle_fail", "well_puzzle_done", "well_fixed", "well_puzzle_step1", "well_puzzle_step2",
    "well_puzzle_step3", "wisperwood_door", "tablet_1", "tablet_2", "tablet_3", "tablet_4", "tablet_5",
    "shrine_pedestals", "shrine_pedestals_fail", "shrine_pedestals_step1", "shrine_pedestals_step2",
    "shrine_pedestals_step3", "shrine_pedestals_step4", "shrine_pedestals_step5", "shrine_door_open", "split_wall",
    "heart_sight", "heart_step", "memory_door", "shrine_crystal_glow", "shrine_crystal_dim", "central_mural_empty",
    "warden_dormant", "warden_rest", "viewer_done", "wounded_wolf",
}

STAGING = {"shootArrow", "moveTo", "moveToPlayer", "teleport", "face", "playAnimation", "wait", "cameraShake", "soulFragment",
           "expression", "closeUp", "closeUpEnd"}


def params(command):
    return {p["type"]: p["value"] for p in command["parameters"]}


def number(value):
    return int(value) if isinstance(value, float) and value.is_integer() else value


def describe(command):
    """A command as a stage direction."""
    kind, p = command["type"], params(command)
    if kind == "playAnimation":
        text = f"{p['who']} plays **{p['animation']}**"
        if number(p.get("wait", 0)):
            text += " (the line waits for it)"
        if p.get("then"):
            text += f", then {p['then']}"
        return text
    if kind == "moveTo":
        return f"{p['who']} walks to {p['to']}" + (f" ({p['speed']})" if p.get("speed") else "")
    if kind == "moveToPlayer":
        return f"{p['who']} walks up to the player" + (f" ({p['speed']})" if p.get("speed") else "")
    if kind == "teleport":
        return f"{p['who']} is placed at {p['to']}"
    if kind == "face":
        return f"{p['who']} turns to face {p['towards']}"
    if kind == "wait":
        return f"pause {number(p['seconds'])}s"
    if kind == "cameraShake":
        return "the camera shakes"
    if kind == "soulFragment":
        return f"a soul fragment drifts from {p['from']} into the player"
    if kind == "shootArrow":
        return f"{p['who']} looses an arrow east, {number(p.get('distance', 64))} px, after {number(p.get('delay', 0.8))}s"
    if kind == "closeUp":
        return f"close-up: `{p['image']}`"
    if kind == "closeUpEnd":
        return "the close-up closes"
    return None


def effect(command):
    """A command that changes the game rather than the scene."""
    kind, p = command["type"], params(command)
    if kind == "giveItem":
        return f"gives {p['item']} x{number(p.get('count', 1))}"
    if kind == "removeItem":
        return f"takes {p['item']} x{number(p.get('count', 1))}"
    if kind in ("setIntFlag", "setFlag"):
        return f"`{p['flag']}` = {number(p['value'])}"
    args = ", ".join(f"{k}={number(v)}" for k, v in p.items())
    return f"{kind}({args})"


def condition_text(conditions):
    parts = []
    for cond in conditions or []:
        p = {x["type"]: x["value"] for x in cond["parameters"]}
        if cond["type"] == "hasIntFlag":
            parts.append(f"`{p['flag']}` {p['op']} {number(p['value'])}")
        else:
            parts.append(cond["type"])
    return " and ".join(parts) or "otherwise"


def main():
    nodes = []
    file_of = {}
    for name in DIALOGUE_FILES:
        document = json.loads((DIALOGUE_DIR / f"{name}.json").read_text(encoding="utf-8"))
        for node in document["nodes"]:
            file_of[node["node"]] = (name, document.get("root"))
        nodes += document["nodes"]
    strings = json.loads(STRINGS.read_text(encoding="utf-8"))["strings"]
    scene_starts = dict(SCENES)

    out = ["# Arc 1 dialogue", "",
           "Generated from `assets/Dialogue/Arc1/*.json` (one file per character) and `assets/Localization/en.json` by",
           "`python tools/dialogue_to_markup.py` - regenerate it after the dialogue changes.", "",
           "## How to read and mark this", "",
           "- Each `##` heading is one dialogue file, each `###` a scene in it and each `####` one node (its id in the JSON).",
           "- **Speaker** *(expression)*: the line. The expression shows on the speaker's portrait; with no art for it",
           "  yet, the portrait shows its default face with the expression written underneath.",
           "- *Staging* lines are what plays while the line shows (or, for a node with no line, before moving on).",
           "- *Effects* are what the node changes in the game (items, flags, fights).",
           "- **Missing** notes are things for you to add (art, animations, markers, sounds).",
           "- *Kept* marks text read in the world (signs, tablets, puzzles, tutorials) - not narration, so it stays.",
           "- To ask for a change, add a line starting with `> EDIT:` under the node, e.g.",
           "  `> EDIT: Kira should laugh here` - anything marked that way can then be applied.", ""]

    # Summary of what's missing.
    out += ["## Still to add", "",
            "### Expression art", "",
            "Every portrait (assets/Portraits, rendered by 3dModelsToSprites `job.portrait.json`) has only its default",
            "face - the rigs can't change faces. These expressions are asked for and show as captions for now:", ""]
    moods = {}
    for node in nodes:
        speaker = node["data"].get("speaker")
        for command in node["data"].get("commands", []):
            if command["type"] == "expression" and speaker:
                moods.setdefault(speaker, set()).add(params(command)["mood"])
    for speaker in sorted(moods):
        out.append(f"- **{speaker}**: {', '.join(sorted(moods[speaker]))}")
    out += ["", "When a face exists for one, list it in `Dialogue/Portraits.wren` `EXPRESSIONS` (and wire its image).", "",
            "### Per scene", ""]
    for node in nodes:
        if node["node"] in MISSING:
            out.append(f"- `{node['node']}` - {MISSING[node['node']]}")
    out += ["", "### Other", "",
            "- The Guide is a voice, but speaks with Claire's portrait (the Guide NPC wears Claire's sheet).",
            "- The player has a portrait (`Portraits/explorer_portrait.png`) but never speaks a line yet.",
            "- `[PH]` lines are placeholder writing from the original script.", ""]

    out += ["# The script", ""]
    current_file = None
    for node in nodes:
        node_id, d = node["node"], node["data"]
        if file_of[node_id][0] != current_file:
            current_file, root = file_of[node_id]
            out += [f"## {current_file} - `assets/Dialogue/Arc1/{current_file}.json` (root `{root}`)", ""]
        if node_id in scene_starts:
            out += [f"### {scene_starts[node_id]}", ""]
        out.append(f"#### `{node_id}`")
        out.append("")
        commands = d.get("commands", [])
        mood = next((params(c)["mood"] for c in commands if c["type"] == "expression"), None)
        if "text" in d:
            text = strings.get(d["text"], f"<missing string {d['text']}>").replace("\n", " ")
            speaker = d.get("speaker")
            if speaker:
                line = f"**{speaker}**" + (f" *({mood})*" if mood else "") + f": {text}"
            else:
                line = f"{text}" + ("  *(kept - read in the world)*" if node_id in KEPT else "")
            out.append(line)
            out.append("")
        staging = [s for s in (describe(c) for c in commands) if s]
        if staging:
            out.append("*Staging:* " + "; ".join(staging))
            out.append("")
        effects = [effect(c) for c in commands if c["type"] not in STAGING]
        if effects:
            out.append("*Effects:* " + "; ".join(effects))
            out.append("")
        for index, choice in enumerate(d.get("choices", []), 1):
            out.append(f"{index}. {strings.get(choice['text'], choice['text'])} → `{choice['target']}`")
        if d.get("choices"):
            out.append("")
        for target in d.get("targets", []):
            out.append(f"- if {condition_text(target.get('conditions'))} → `{target['node']}`")
        if d.get("targets"):
            out.append("")
        if "next" in d:
            out.append(f"→ `{d['next']}`")
            out.append("")
        if node_id in MISSING:
            out.append(f"**Missing:** {MISSING[node_id]}")
            out.append("")

    OUT.write_bytes(("\n".join(out).rstrip() + "\n").encode("utf-8"))
    print(f"wrote {OUT.relative_to(ROOT)} ({len(nodes)} nodes)")


if __name__ == "__main__":
    main()
