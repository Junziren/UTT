#pragma once
#include <JuceHeader.h>
#include "UTTLookAndFeel.h"

// ============================================================
// ThemeLoader — Serum‑inspired theme manager
//
// Themes live in Documents/UTT/Themes/ as either:
//   1. A .json file (legacy, backward compatible)
//   2. A subfolder with theme.json + optional Fonts/ + preview.png + colormap.json
//
// Built‑in themes are embedded and installed into the Themes folder
// on first run, using the folder format.
// ============================================================
class ThemeLoader
{
public:
    // Path helpers
    static juce::File getRootDir();        // Documents/UTT/
    static juce::File getThemesDir();      // Documents/UTT/Themes/
    static juce::File getPresetsDir();     // Documents/UTT/Presets/
    static juce::File getRecordingsDir();  // Documents/UTT/Recordings/

    // Directory setup
    static void ensureDirectoriesExist();
    static void installBuiltInThemes();    // only if not already present

    // Discovery
    static juce::StringArray getAvailableThemeNames();
    static juce::Array<juce::File>   getAvailableThemeFolders();
    static juce::File getPreviewForTheme(const juce::String& name);

    // Loading
    static std::unique_ptr<UTTLookAndFeel> loadTheme(const juce::String& name);
    static std::unique_ptr<UTTLookAndFeel> loadFromFolder(const juce::File& folder);
    static std::unique_ptr<UTTLookAndFeel> loadFromJSON(const juce::File& jsonFile);

    // Built‑in fallback
    static std::unique_ptr<UTTLookAndFeel> getBuiltIn(int index);
    static int getNumBuiltIn() { return 3; }
    static juce::StringArray getBuiltInNames();

    // Save a currently active theme as a user theme
    static juce::File saveUserTheme(const UTTTheme& theme);

private:
    static UTTTheme parseThemeJSON(const juce::var& json, const juce::File& folder);
    static juce::Colour parseColour(const juce::var& v, const juce::Colour& fallback);
    static void loadFontsFromFolder(const juce::File& fontsDir);

    // Write a built‑in theme to a JSON file inside its own folder
    static void writeBuiltInThemeFolder(int index, const juce::File& parentDir);
    static juce::var makeBuiltInThemeVar(int index);
};
