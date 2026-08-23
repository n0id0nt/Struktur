// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: localization

class Localization {
    // Loads every language listed in a localization manifest file
    foreign static loadManifest(arg0)
    // Loads (or merges into) one language from its own localization file
    foreign static loadLanguage(arg0,arg1)
    // Gets the localized string for a key in the active language (falling back to the default language, then to "[key]", if missing)
    foreign static get(arg0)
    // Checks whether a key has a string in the active or default language
    foreign static hasString(arg0)
    // Sets the active language (must already be loaded)
    foreign static setLanguage(arg0)
    // Gets the active language code
    foreign static getLanguage()
    // Gets every currently-loaded language code
    foreign static getAvailableLanguages()
}

