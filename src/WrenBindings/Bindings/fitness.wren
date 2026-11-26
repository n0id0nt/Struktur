// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: fitness

class Fitness {
    // True when a Bluetooth LE adapter (or the simulated backend) is usable.
    foreign static isAvailable()
    // True when STRUKTUR_FITNESS_SIMULATE replaced Bluetooth with fake devices.
    foreign static isSimulated()
    // Starts scanning for trainers, heart rate monitors and power meters.
    foreign static startScan()
    // Stops scanning.
    foreign static stopScan()
    // True while a scan is running.
    foreign static isScanning()
    // Devices found by scanning: list of maps (address, name, rssi, roles, inUseAs).
    foreign static devices()
    // Connects a device in a role (role, address, name) and remembers it. Returns false if refused.
    foreign static assign(arg0,arg1,arg2)
    // Disconnects and forgets the device in a role.
    foreign static unassign(arg0)
    // Connects every remembered device that is not already connected.
    foreign static connectRemembered()
    // The remembered device for a role (address, name) or null.
    foreign static remembered(arg0)
    // Connection status of a role's device (state, name, address, control, mode) or null.
    foreign static status(arg0)
    // Latest values reported by a role's device, as a map.
    foreign static sample(arg0)
    // True when a trainer or power meter is connected.
    foreign static hasPowerSource()
    // ERG target in watts. Returns the command result as a string.
    foreign static setTargetPower(arg0)
    // Simulation grade in percent. Returns the command result as a string.
    foreign static setGrade(arg0)
    // Manual resistance 0..1 (simulation grade when supported). Returns the command result.
    foreign static setManualResistance(arg0)
    // Removes the trainer target (free ride). Returns the command result as a string.
    foreign static clearTarget()
}

