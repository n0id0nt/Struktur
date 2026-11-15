// UI/Inventory/MapPage.wren
// The inventory's Map tab (unlocked by carrying the Old Map - see States/InventoryState.wren): every room the player
// has been in (World/MapLog.wren) drawn as a rectangle where it sits in the LDtk world, filled with the colour of its
// Terrain, scaled to fit. Interiors (WorpPoint levels) aren't drawn - the doorways the player took into them show as
// small door markers in the room outside instead. The room the player is standing in has a bright outline and a
// marker; inside an interior, its doorway is highlighted instead. A legend along the bottom names each terrain colour
// (and the doorway marker). Pure view: `model` is
//   { "rooms": [{ "id", "bounds": [x, y, w, h], "color": [r, g, b], "terrain" }],
//     "doors": [{ "interior", "room", "position": [x, y] }], "current": room id or null,
//     "currentName": display text, "labels": { "title", "here", "empty", "door" }, "terrainNames": { terrain: text } }
import "math" for Vec2, Vec4
import "ui" for UIColor
import "UI/Inventory/InventoryTheme" for InventoryTheme

var PAGE_SIZE = Vec2.new(1072, 476)
var MAP_PAD = 24
var HEADER_HEIGHT = 44
var LEGEND_HEIGHT = 44
var MAX_SCALE = 0.5             // a handful of rooms shouldn't fill the page edge to edge
var EDGE = 2                    // dark border between neighbouring rooms
var CURRENT_RING = 3
var MARKER_SIZE = 12
var SWATCH_SIZE = 18
var DOOR_SIZE = 10              // a doorway marker (an interior's entrance), centred on the door
var DOOR_COLOR = Vec4.new(246, 214, 160, 255)
var LEGEND_STRIDE = 190

class MapPage {
    construct new(parent, model, boldFont, bookFont) {
        _root = InventoryTheme.group(parent, Vec2.new(0, 0), PAGE_SIZE, 2)
        var well = InventoryTheme.frame(_root, "well", Vec2.new(0, 0), Vec2.new(0, 0), PAGE_SIZE, Vec2.new(0, 0), 3)
        var labels = model["labels"]

        InventoryTheme.label(well, labels["title"], Vec2.new(MAP_PAD, 14), Vec2.new(0, 0), Vec2.new(0, 0), 26.0,
            boldFont, InventoryTheme.accent, 4)
        if (model["currentName"] != null) {
            InventoryTheme.label(well, "%(labels["here"]) %(model["currentName"])", Vec2.new(-MAP_PAD, 18),
                Vec2.new(1, 0), Vec2.new(1, 0), 20.0, bookFont, InventoryTheme.text, 4)
        }

        if (model["rooms"].count == 0) {
            InventoryTheme.label(well, labels["empty"], Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 24.0,
                bookFont, InventoryTheme.textMuted, 4)
        } else {
            drawRooms_(well, model, bookFont)
        }
    }

    drawRooms_(well, model, bookFont) {
        var rooms = model["rooms"]
        // Fit the visited rooms' bounding box into the area between the header and the legend, centred.
        var minX = rooms[0]["bounds"][0]
        var minY = rooms[0]["bounds"][1]
        var maxX = minX
        var maxY = minY
        for (room in rooms) {
            var b = room["bounds"]
            if (b[0] < minX) minX = b[0]
            if (b[1] < minY) minY = b[1]
            if (b[0] + b[2] > maxX) maxX = b[0] + b[2]
            if (b[1] + b[3] > maxY) maxY = b[1] + b[3]
        }
        var areaW = PAGE_SIZE.x - 2 * MAP_PAD
        var areaH = PAGE_SIZE.y - HEADER_HEIGHT - LEGEND_HEIGHT - MAP_PAD
        var scale = areaW / (maxX - minX)
        if (areaH / (maxY - minY) < scale) scale = areaH / (maxY - minY)
        if (scale > MAX_SCALE) scale = MAX_SCALE
        var originX = MAP_PAD + (areaW - (maxX - minX) * scale) / 2
        var originY = HEADER_HEIGHT + (areaH - (maxY - minY) * scale) / 2

        for (room in rooms) {
            var b = room["bounds"]
            var x = (originX + (b[0] - minX) * scale).round
            var y = (originY + (b[1] - minY) * scale).round
            var w = (b[2] * scale).round
            var h = (b[3] * scale).round
            var isCurrent = room["id"] == model["current"]
            if (isCurrent) {
                MapPage.rect_(well, x - CURRENT_RING, y - CURRENT_RING, w + 2 * CURRENT_RING, h + 2 * CURRENT_RING,
                    InventoryTheme.accent, 6)
            }
            MapPage.rect_(well, x, y, w, h, InventoryTheme.textDark, isCurrent ? 7 : 4)
            var c = room["color"]
            MapPage.rect_(well, x + EDGE, y + EDGE, w - 2 * EDGE, h - 2 * EDGE, Vec4.new(c[0], c[1], c[2], 255),
                isCurrent ? 8 : 5)
            if (isCurrent) {
                var mx = x + ((w - MARKER_SIZE) / 2).round
                var my = y + ((h - MARKER_SIZE) / 2).round
                MapPage.rect_(well, mx - 2, my - 2, MARKER_SIZE + 4, MARKER_SIZE + 4, InventoryTheme.textDark, 9)
                MapPage.rect_(well, mx, my, MARKER_SIZE, MARKER_SIZE, InventoryTheme.text, 10)
            }
        }

        // Doorways into interiors. The one leading to the interior the player is in now stands out.
        var doors = model["doors"]
        for (door in doors) {
            var p = door["position"]
            var size = DOOR_SIZE
            var isCurrent = door["interior"] == model["current"]
            if (isCurrent) size = MARKER_SIZE
            var x = (originX + (p[0] - minX) * scale - size / 2).round
            var y = (originY + (p[1] - minY) * scale - size / 2).round
            if (isCurrent) {
                MapPage.rect_(well, x - 2 - CURRENT_RING, y - 2 - CURRENT_RING, size + 4 + 2 * CURRENT_RING,
                    size + 4 + 2 * CURRENT_RING, InventoryTheme.accent, 11)
            }
            MapPage.rect_(well, x - 2, y - 2, size + 4, size + 4, InventoryTheme.textDark, isCurrent ? 12 : 9)
            MapPage.rect_(well, x, y, size, size, isCurrent ? InventoryTheme.text : DOOR_COLOR, isCurrent ? 13 : 10)
        }

        // Legend: one swatch per terrain on the map, in the order they were first visited, then the doorway marker.
        var seen = []
        var legendY = PAGE_SIZE.y - LEGEND_HEIGHT + 4
        for (room in rooms) {
            var terrain = room["terrain"]
            if (!seen.contains(terrain)) {
                var x = MAP_PAD + seen.count * LEGEND_STRIDE
                seen.add(terrain)
                var c = room["color"]
                MapPage.rect_(well, x - EDGE, legendY - EDGE, SWATCH_SIZE + 2 * EDGE, SWATCH_SIZE + 2 * EDGE,
                    InventoryTheme.textDark, 4)
                MapPage.rect_(well, x, legendY, SWATCH_SIZE, SWATCH_SIZE, Vec4.new(c[0], c[1], c[2], 255), 5)
                var names = model["terrainNames"]
                var name = names.containsKey(terrain) ? names[terrain] : terrain
                InventoryTheme.label(well, name, Vec2.new(x + SWATCH_SIZE + 10, legendY - 1), Vec2.new(0, 0),
                    Vec2.new(0, 0), 18.0, bookFont, InventoryTheme.text, 4)
            }
        }
        if (doors.count > 0) {
            var x = MAP_PAD + seen.count * LEGEND_STRIDE
            var inset = (SWATCH_SIZE - DOOR_SIZE) / 2
            MapPage.rect_(well, x + inset - 2, legendY + inset - 2, DOOR_SIZE + 4, DOOR_SIZE + 4, InventoryTheme.textDark, 4)
            MapPage.rect_(well, x + inset, legendY + inset, DOOR_SIZE, DOOR_SIZE, DOOR_COLOR, 5)
            InventoryTheme.label(well, model["labels"]["door"], Vec2.new(x + SWATCH_SIZE + 10, legendY - 1),
                Vec2.new(0, 0), Vec2.new(0, 0), 18.0, bookFont, InventoryTheme.text, 4)
        }
    }

    static rect_(parent, x, y, w, h, color, z) {
        var rect = UIColor.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(w, h), Vec2.new(0, 0))
        rect.setColor(color)
        rect.setZIndex(z)
        parent.addChild(rect)
        return rect
    }

    root { _root }

    // Nothing to focus - the tab keeps focus.
    firstFocusable { null }
    linkUpTo(tab) {}

    setVisible(visible) { _root.setVisible(visible) }

    release() {
        _root = null
    }
}
