// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: ride

class Ride {
    // The app-wide ride settings as a map.
    foreign static settings()
    // Changes one ride setting (validated); returns false for an unknown key. saveSettings() persists.
    foreign static setSetting(arg0,arg1)
    // Writes ride_settings.json.
    foreign static saveSettings()
    // Built-in and user workouts: list of maps (id, name, source, durationSeconds, error).
    foreign static workouts()
    // Loads a workout by id or path for a preview (id, ftpWatts) -> map with ok/error/steps.
    foreign static loadWorkout(arg0,arg1)
    // The folder players drop their own .zwo/.mrc files into.
    foreign static userWorkoutsFolder()
    // A workout path from the command line, returned once (else null).
    foreign static commandLineWorkout()
    // True when started with --demo.
    foreign static isDemo()
    // The saved world settings as a string for World.load / World.applySettings.
    foreign static worldSettings()
    // Starts a Free Ride (name, ftpWatts, maxHeartRateBpm, thresholdHeartRateBpm, weightKg).
    foreign static startFreeRide(arg0,arg1,arg2,arg3,arg4)
    // Starts a workout (id, name, ftpWatts, maxHeartRateBpm, thresholdHeartRateBpm, weightKg) -> map.
    foreign static startWorkout(arg0,arg1,arg2,arg3,arg4,arg5)
    // True while a ride is running.
    foreign static isActive()
    // Snapshot of the ride for the HUD, or null.
    foreign static state()
    // Switches ERG on/off.
    foreign static toggleErg()
    // +1/-1: ERG on trims the target, ERG off changes the grade.
    foreign static adjustResistance(arg0)
    // "watts" or "percent".
    foreign static setTrimMode(arg0)
    // Free Ride ERG target in watts (clamped).
    foreign static setFreeRideTarget(arg0)
    // +1/-1: Free Ride target +/- 5 W.
    foreign static nudgeTarget(arg0)
    // Pauses or resumes the ride.
    foreign static togglePause()
    // Skips the current workout step.
    foreign static skipStep()
    // Adds seconds to the current step.
    foreign static extendStep(arg0)
    // Starts a new lap (Free Ride).
    foreign static lap()
    // Ends the ride and writes FIT + JSON -> map with ok, paths and a summary.
    foreign static finish()
    // Drops the ride session.
    foreign static discard()
    // Fills a UIGraph with the last N seconds of power, heart rate and target.
    foreign static updateGraph(arg0,arg1)
    // Fills a UIGraph with the workout profile and position marker.
    foreign static updateProfile(arg0)
    // Fills a UIGraph with a workout's profile (graph, id, ftpWatts).
    foreign static previewProfile(arg0,arg1,arg2)
}

