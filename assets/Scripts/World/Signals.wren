// World/Signals.wren
// Wires senders (GameObjects/PressurePlate.wren) to receivers (GameObjects/Door.wren) by a shared channel name.
// A channel is on while at least one sender on it is on, so two plates on one channel act as an OR; a receiver
// listening to several channels (allOn) needs every one of them, so plates on separate channels act as an AND.
// Receivers poll each update rather than being called back, so spawn order doesn't matter.
// A channel can also be retired once it has done its job (a door that opened for good): its senders go inactive.
// The gameplay state (States/ExperimentState.wren) clears it on exit.
class Signals {
    // source: anything unique to the sender (its entity id).
    static set(channel, source, on) {
        if (channel == null) {
            return
        }
        if (__channels == null) {
            __channels = {}
        }
        var sources = __channels[channel]
        if (sources == null) {
            sources = {}
            __channels[channel] = sources
        }
        if (on) {
            sources[source] = true
        } else {
            sources.remove(source)
        }
    }

    static isOn(channel) {
        if (__channels == null || channel == null) {
            return false
        }
        var sources = __channels[channel]
        return sources != null && sources.count > 0
    }

    // True when every channel in the list is on (false for an empty list).
    static allOn(channels) {
        if (channels == null || channels.count == 0) {
            return false
        }
        for (channel in channels) {
            if (!Signals.isOn(channel)) {
                return false
            }
        }
        return true
    }

    // Marks a channel as done for good - senders on it stop responding (GameObjects/PressurePlate.wren).
    static retire(channel) {
        if (__retired == null) {
            __retired = {}
        }
        __retired[channel] = true
    }

    static isRetired(channel) { __retired != null && channel != null && __retired.containsKey(channel) }

    static clear() {
        __channels = null
        __retired = null
    }
}
