import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Inventory" for Inventory
import "debug" for Debug
import "gameObjectComponents" for Camera
import "gameObject" for GameObject

class Commands {
    static registerFunctions() {
        DialogueRegistry.registerCommand("giveItem") { |params|
            var item = params["item"]
            Inventory.addItem(item)
            Debug.info("[Dialogue Command] Added item to inventory %(item)")
        }
        DialogueRegistry.registerCommand("removeItem") { |params|
            var item = params["item"]
            Inventory.removeItem(item)
            Debug.info("[Dialogue Command] Removed item from inventory %(item)")
        }
        DialogueRegistry.registerCommand("setIntFlag") { |params|
            var flag = params["flag"]
            var value = params["value"]
            var intFlag = FlagManager.setIntFlag(flag, value)
            Debug.info("[Dialogue Command] Set Int Flag for %(flag): %(value)")
        }
        DialogueRegistry.registerCommand("cameraShake") { |params|
            var cameras = GameObject.getAllWithComponent("Camera")
            for (cameraId in cameras) {
                Camera.addCameraTrauma(cameraId, 0.4)
            }
        }
        DialogueRegistry.registerCommand("pickupEntity") { |params|
            //var cameras = GameObject.getAllWithComponents(["Camera"])
            //for (cameraId in cameras) {
            //    Camera.addCameraTrauma(cameraId, 0.4)
            //}
        }
    }
}
