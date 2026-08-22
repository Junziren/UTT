#include "ThemeLoader.h"

// ================================================================
// Built‑in theme data (embedded, written as folders on first run)
// ================================================================
static const char* kBuiltInThemeJSON[3] = {
    // Channel Strip (warm analog console)
    R"({
"name":"Channel Strip","description":"Classic analog console channel strip",
"colors":{"background":"#1a1a23","panelBackground":"#2a2a35","headerBg":"#222230",
"textPrimary":"#e8e8e8","textSecondary":"#999999","textHighlight":"#cc6600",
"accent":"#cc6600","accent2":"#cc4400",
"knobCap":"#c8c8c8","knobBase":"#4a4a4a","knobIndicator":"#cc6600","knobArcBg":"#3a3a3a","knobValueText":"#00cc66",
"vuNeedle":"#ff4400","vuBacklight":"#ffcc00",
"meterBg":"#0a0a0a","meterLow":"#00cc66","meterMid":"#cccc00","meterHigh":"#cc0000",
"meterPeakHold":"#ffffff","meterTick":"#888888",
"buttonOn":"#ff3300","buttonOff":"#3d1a1a","buttonHover":"#ff5533","buttonGlow":"#44ff3300",
"border":"#555555","sectionLine":"#333333",
"scopeBg":"#0d0d0d","scopeGrid":"#1a1a1a","scopeLine":"#00ff66","scopeLine2":"#ff6600",
"scopeAxesText":"#666666",
"corrPositive":"#00cc66","corrNegative":"#cc0000","corrNeutral":"#888888",
"lufsPass":"#00cc66","lufsWarn":"#ff6600","lufsFail":"#cc0000",
"comboBg":"#2a2a2a","comboOutline":"#555555","comboText":"#dddddd","comboArrow":"#cc6600",
"recordActive":"#ff0000","recordInactive":"#444444"},
"fonts":{"main":"Arial","mono":"Consolas"},
"sizes":{"fontMain":13,"fontSmall":11,"fontTitle":15,"fontMono":11}
})",

    // Dark Modern (clean blue tech)
    R"({
"name":"Dark Modern","description":"Flat dark theme with blue accents",
"colors":{"background":"#121218","panelBackground":"#1e1e28","headerBg":"#161620",
"textPrimary":"#e0e0e0","textSecondary":"#808080","textHighlight":"#2196F3",
"accent":"#2196F3","accent2":"#1976D2",
"knobCap":"#e0e0e0","knobBase":"#3a3a3a","knobIndicator":"#2196F3","knobArcBg":"#2a2a3a","knobValueText":"#42A5F5",
"vuNeedle":"#448AFF","vuBacklight":"#2962FF",
"meterBg":"#0a0a10","meterLow":"#00E676","meterMid":"#FFD600","meterHigh":"#FF1744",
"meterPeakHold":"#ffffff","meterTick":"#666666",
"buttonOn":"#2196F3","buttonOff":"#1a3050","buttonHover":"#42A5F5","buttonGlow":"#332196F3",
"border":"#333333","sectionLine":"#282828",
"scopeBg":"#0a0a0a","scopeGrid":"#181818","scopeLine":"#40C4FF","scopeLine2":"#FF4081",
"scopeAxesText":"#555555",
"corrPositive":"#00E676","corrNegative":"#FF1744","corrNeutral":"#777777",
"lufsPass":"#00E676","lufsWarn":"#FFD600","lufsFail":"#FF1744",
"comboBg":"#222222","comboOutline":"#444444","comboText":"#dddddd","comboArrow":"#2196F3",
"recordActive":"#FF1744","recordInactive":"#3a3a3a"},
"fonts":{"main":"Segoe UI","mono":"Consolas"},
"sizes":{"fontMain":12,"fontSmall":10,"fontTitle":14,"fontMono":11}
})",

    // Retro Light (vintage cream)
    R"({
"name":"Retro Light","description":"Vintage analog equipment cream tones",
"colors":{"background":"#e8e0d0","panelBackground":"#ddd5c5","headerBg":"#d5ccbb",
"textPrimary":"#2a2a2a","textSecondary":"#666666","textHighlight":"#8B4513",
"accent":"#8B4513","accent2":"#A0522D",
"knobCap":"#555555","knobBase":"#c0b8a0","knobIndicator":"#8B4513","knobArcBg":"#d0c8b0","knobValueText":"#2E7D32",
"vuNeedle":"#8B0000","vuBacklight":"#FFF8DC",
"meterBg":"#d5ccc0","meterLow":"#2E7D32","meterMid":"#F57F17","meterHigh":"#C62828",
"meterPeakHold":"#333333","meterTick":"#999999",
"buttonOn":"#8B4513","buttonOff":"#d5ccc0","buttonHover":"#A0522D","buttonGlow":"#448B4513",
"border":"#bfb8a8","sectionLine":"#c8c0b0",
"scopeBg":"#ddd5c5","scopeGrid":"#c8c0b0","scopeLine":"#2E7D32","scopeLine2":"#C62828",
"scopeAxesText":"#888888",
"corrPositive":"#2E7D32","corrNegative":"#C62828","corrNeutral":"#999999",
"lufsPass":"#2E7D32","lufsWarn":"#F57F17","lufsFail":"#C62828",
"comboBg":"#ddd5c5","comboOutline":"#bfb8a8","comboText":"#2a2a2a","comboArrow":"#8B4513",
"recordActive":"#C62828","recordInactive":"#cccccc"},
"fonts":{"main":"Georgia","mono":"Consolas"},
"sizes":{"fontMain":13,"fontSmall":10,"fontTitle":16,"fontMono":11}
})"
};

// ================================================================
// Path helpers
// ================================================================
juce::File ThemeLoader::getRootDir()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
               .getChildFile("UTT");
}

juce::File ThemeLoader::getThemesDir()
{
    return getRootDir().getChildFile("Themes");
}

juce::File ThemeLoader::getPresetsDir()
{
    return getRootDir().getChildFile("Presets");
}

juce::File ThemeLoader::getRecordingsDir()
{
    return getRootDir().getChildFile("Recordings");
}

void ThemeLoader::ensureDirectoriesExist()
{
    getThemesDir().createDirectory();
    getPresetsDir().createDirectory();
    getRecordingsDir().createDirectory();
}

// ================================================================
// Install built‑in themes as folders (Serum‑style)
// ================================================================
void ThemeLoader::installBuiltInThemes()
{
    ensureDirectoriesExist();
    auto dir = getThemesDir();

    for (int i = 0; i < 3; ++i)
    {
        juce::var json = juce::JSON::parse(kBuiltInThemeJSON[i]);
        auto name = json["name"].toString();
        auto folder = dir.getChildFile(name);

        // Create folder + theme.json if not present
        if (!folder.exists())
        {
            folder.createDirectory();
            auto jsonFile = folder.getChildFile("theme.json");
            jsonFile.replaceWithText(kBuiltInThemeJSON[i]);
        }
        else if (!folder.getChildFile("theme.json").existsAsFile())
        {
            // Folder exists but no theme.json — write it
            folder.getChildFile("theme.json").replaceWithText(kBuiltInThemeJSON[i]);
        }
    }
}

// ================================================================
// Discovery — scan Themes folder for both .json files and subfolders
// ================================================================
juce::StringArray ThemeLoader::getAvailableThemeNames()
{
    installBuiltInThemes();

    juce::StringArray names;
    auto dir = getThemesDir();

    // 1. Scan subfolders that contain theme.json (Serum‑style)
    for (auto& d : dir.findChildFiles(juce::File::findDirectories, false))
    {
        auto jsonFile = d.getChildFile("theme.json");
        if (jsonFile.existsAsFile())
        {
            auto json = juce::JSON::parse(jsonFile);
            if (json.isObject() && json.hasProperty("name"))
                names.add(json["name"].toString());
            else
                names.add(d.getFileName());
        }
    }

    // 2. Scan loose .json files (legacy / extra)
    for (auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        auto json = juce::JSON::parse(f);
        auto n = json.isObject() && json.hasProperty("name")
                    ? json["name"].toString()
                    : f.getFileNameWithoutExtension();
        if (!names.contains(n))
            names.add(n);
    }

    if (names.isEmpty())
        names = getBuiltInNames();

    return names;
}

juce::Array<juce::File> ThemeLoader::getAvailableThemeFolders()
{
    installBuiltInThemes();
    juce::Array<juce::File> folders;
    auto dir = getThemesDir();
    for (auto& d : dir.findChildFiles(juce::File::findDirectories, false))
    {
        if (d.getChildFile("theme.json").existsAsFile())
            folders.add(d);
    }
    return folders;
}

juce::File ThemeLoader::getPreviewForTheme(const juce::String& name)
{
    auto dir = getThemesDir();
    auto folder = dir.getChildFile(name);
    if (folder.isDirectory())
    {
        auto prev = folder.getChildFile("preview.png");
        if (prev.existsAsFile()) return prev;
    }
    return {};
}

// ================================================================
// Loading
// ================================================================
std::unique_ptr<UTTLookAndFeel> ThemeLoader::loadTheme(const juce::String& name)
{
    installBuiltInThemes();

    auto dir = getThemesDir();

    // First try: subfolder with theme.json
    auto folder = dir.getChildFile(name);
    if (folder.isDirectory() && folder.getChildFile("theme.json").existsAsFile())
        return loadFromFolder(folder);

    // Second try: JSON file
    auto jsonFile = dir.getChildFile(name + ".json");
    if (jsonFile.existsAsFile())
        return loadFromJSON(jsonFile);

    // Third: try matching any folder by name
    for (auto& d : dir.findChildFiles(juce::File::findDirectories, false))
    {
        auto jf = d.getChildFile("theme.json");
        if (jf.existsAsFile())
        {
            auto json = juce::JSON::parse(jf);
            if (json.isObject() && json["name"].toString() == name)
                return loadFromFolder(d);
        }
    }

    // Fallback: built‑in by name
    auto bNames = getBuiltInNames();
    for (int i = 0; i < bNames.size(); ++i)
        if (bNames[i] == name)
            return getBuiltIn(i);

    return getBuiltIn(0);
}

std::unique_ptr<UTTLookAndFeel> ThemeLoader::loadFromFolder(const juce::File& folder)
{
    auto jsonFile = folder.getChildFile("theme.json");
    if (!jsonFile.existsAsFile())
        return getBuiltIn(0);

    juce::var json;
    {
        juce::FileInputStream is(jsonFile);
        if (is.openedOk())
            json = juce::JSON::parse(is.readEntireStreamAsString());
    }

    if (!json.isObject())
        return getBuiltIn(0);

    auto theme = std::make_unique<UTTLookAndFeel>(parseThemeJSON(json, folder));

    // Load custom fonts from Fonts/ subfolder
    auto fontsDir = folder.getChildFile("Fonts");
    if (fontsDir.isDirectory())
        loadFontsFromFolder(fontsDir);

    
    // Load colormap if present (overrides theme colours from a separate JSON file)
    auto colorMapFile = folder.getChildFile("colormap.json");
    if (colorMapFile.existsAsFile())
    {
        juce::FileInputStream cmis(colorMapFile);
        if (cmis.openedOk())
        {
            auto cmJson = juce::JSON::parse(cmis.readEntireStreamAsString());
            if (cmJson.isObject() && cmJson["colors"].isObject())
            {
                auto cmColors = cmJson["colors"];
                auto& t = const_cast<UTTTheme&>(theme->getTheme());
                if (cmColors["accent"].isString())
                    t.accent = parseColour(cmColors["accent"], t.accent);
                if (cmColors["background"].isString())
                    t.background = parseColour(cmColors["background"], t.background);
            }
        }
    }

}

std::unique_ptr<UTTLookAndFeel> ThemeLoader::loadFromJSON(const juce::File& jsonFile)
{
    if (!jsonFile.existsAsFile())
        return getBuiltIn(0);

    juce::var json;
    {
        juce::FileInputStream is(jsonFile);
        if (is.openedOk())
            json = juce::JSON::parse(is.readEntireStreamAsString());
    }

    if (!json.isObject())
        return getBuiltIn(0);

    return std::make_unique<UTTLookAndFeel>(parseThemeJSON(json, jsonFile.getParentDirectory()));
}

// ================================================================
// Save user theme
// ================================================================
juce::File ThemeLoader::saveUserTheme(const UTTTheme& tm)
{
    auto dir = getThemesDir();
    auto folder = dir.getChildFile(tm.name);
    folder.createDirectory();

    // Build JSON
    juce::DynamicObject obj;
    obj.setProperty("name", tm.name);
    obj.setProperty("description", tm.description);

    juce::DynamicObject colorsObj;
    colorsObj.setProperty("background",     tm.background.toDisplayString(false));
    colorsObj.setProperty("panelBackground",tm.panelBg.toDisplayString(false));
    colorsObj.setProperty("textPrimary",    tm.textPrimary.toDisplayString(false));
    colorsObj.setProperty("accent",         tm.accent.toDisplayString(false));
    colorsObj.setProperty("accent2",        tm.accent2.toDisplayString(false));
    colorsObj.setProperty("meterLow",       tm.meterLow.toDisplayString(false));
    colorsObj.setProperty("meterMid",       tm.meterMid.toDisplayString(false));
    colorsObj.setProperty("meterHigh",      tm.meterHigh.toDisplayString(false));
    {
        juce::var colorsVar(new juce::DynamicObject(colorsObj));
        obj.setProperty("colors", colorsVar);
    }

    {
        juce::DynamicObject fontsObj;
        fontsObj.setProperty("main", tm.fontMain);
        fontsObj.setProperty("mono", tm.fontMono);
        juce::var fontsVar(new juce::DynamicObject(fontsObj));
        obj.setProperty("fonts", fontsVar);
    }

    {
        juce::DynamicObject sizesObj;
        sizesObj.setProperty("fontMain",  tm.fontSize);
        sizesObj.setProperty("fontSmall", tm.fontSizeSmall);
        sizesObj.setProperty("fontTitle", tm.fontSizeTitle);
        sizesObj.setProperty("fontMono",  tm.fontSizeMono);
        juce::var sizesVar(new juce::DynamicObject(sizesObj));
        obj.setProperty("sizes", sizesVar);
    }

    {
        juce::var objVar(new juce::DynamicObject(obj));
        folder.getChildFile("theme.json")
              .replaceWithText(juce::JSON::toString(objVar));
    }

    return folder;
}

// ================================================================
// Built‑in loading
// ================================================================
std::unique_ptr<UTTLookAndFeel> ThemeLoader::getBuiltIn(int index)
{
    juce::var json;
    if (index >= 0 && index < 3)
        json = juce::JSON::parse(kBuiltInThemeJSON[index]);

    if (!json.isObject())
        return std::make_unique<UTTLookAndFeel>();

    return std::make_unique<UTTLookAndFeel>(parseThemeJSON(json, {}));
}

juce::StringArray ThemeLoader::getBuiltInNames()
{
    return { "Channel Strip", "Dark Modern", "Retro Light" };
}

// ================================================================
// JSON parsing
// ================================================================
UTTTheme ThemeLoader::parseThemeJSON(const juce::var& json, const juce::File& folder)
{
    UTTTheme theme;
    theme.themeFolder = folder;

    auto colors = json["colors"];
    auto fonts  = json["fonts"];
    auto sizes  = json["sizes"];

    if (json.hasProperty("name"))        theme.name        = json["name"].toString();
    if (json.hasProperty("description")) theme.description = json["description"].toString();

    if (colors.isObject())
    {
        theme.background    = parseColour(colors["background"],     theme.background);
        theme.panelBg       = parseColour(colors["panelBackground"],theme.panelBg);
        theme.headerBg      = parseColour(colors["headerBg"],       theme.headerBg);
        theme.footerBg      = parseColour(colors.getProperty("headerBg", "background"), theme.footerBg);
        theme.textPrimary   = parseColour(colors["textPrimary"],    theme.textPrimary);
        theme.textSecondary = parseColour(colors["textSecondary"],  theme.textSecondary);
        theme.textHighlight = parseColour(colors["textHighlight"],  theme.textHighlight);
        theme.accent        = parseColour(colors["accent"],         theme.accent);
        theme.accent2       = parseColour(colors["accent2"],        theme.accent2);
        theme.knobCap       = parseColour(colors["knobCap"],        theme.knobCap);
        theme.knobBase      = parseColour(colors["knobBase"],       theme.knobBase);
        theme.knobIndicator = parseColour(colors["knobIndicator"],  theme.knobIndicator);
        theme.knobArcBg     = parseColour(colors["knobArcBg"],      theme.knobArcBg);
        theme.knobValueText = parseColour(colors["knobValueText"],  theme.knobValueText);
        theme.vuNeedle      = parseColour(colors["vuNeedle"],       theme.vuNeedle);
        theme.vuBacklight   = parseColour(colors["vuBacklight"],    theme.vuBacklight);
        theme.meterBg       = parseColour(colors["meterBg"],        theme.meterBg);
        theme.meterLow      = parseColour(colors["meterLow"],       theme.meterLow);
        theme.meterMid      = parseColour(colors["meterMid"],       theme.meterMid);
        theme.meterHigh     = parseColour(colors["meterHigh"],      theme.meterHigh);
        theme.meterPeakHold = parseColour(colors["meterPeakHold"],  theme.meterPeakHold);
        theme.meterTick     = parseColour(colors["meterTick"],      theme.meterTick);
        theme.buttonOn      = parseColour(colors["buttonOn"],       theme.buttonOn);
        theme.buttonOff     = parseColour(colors["buttonOff"],      theme.buttonOff);
        theme.buttonHover   = parseColour(colors["buttonHover"],    theme.buttonHover);
        theme.buttonGlow    = parseColour(colors.getProperty("buttonGlow","buttonOn"),
                                           theme.buttonOn.withAlpha(0.25f));
        theme.border        = parseColour(colors["border"],         theme.border);
        theme.sectionLine   = parseColour(colors["sectionLine"],    theme.sectionLine);
        theme.scopeBg       = parseColour(colors["scopeBg"],        theme.scopeBg);
        theme.scopeGrid     = parseColour(colors["scopeGrid"],      theme.scopeGrid);
        theme.scopeLine     = parseColour(colors["scopeLine"],      theme.scopeLine);
        theme.scopeLine2    = parseColour(colors["scopeLine2"],     theme.scopeLine2);
        theme.scopeAxesText = parseColour(colors["scopeAxesText"],  theme.scopeAxesText);
        theme.corrPositive  = parseColour(colors["corrPositive"],   theme.corrPositive);
        theme.corrNegative  = parseColour(colors["corrNegative"],   theme.corrNegative);
        theme.corrNeutral   = parseColour(colors["corrNeutral"],    theme.corrNeutral);
        theme.lufsPass      = parseColour(colors["lufsPass"],       theme.lufsPass);
        theme.lufsWarn      = parseColour(colors["lufsWarn"],       theme.lufsWarn);
        theme.lufsFail      = parseColour(colors["lufsFail"],       theme.lufsFail);
        theme.comboBg       = parseColour(colors["comboBg"],        theme.comboBg);
        theme.comboOutline  = parseColour(colors["comboOutline"],   theme.comboOutline);
        theme.comboText     = parseColour(colors["comboText"],      theme.comboText);
        theme.comboArrow    = parseColour(colors["comboArrow"],     theme.comboArrow);
        theme.recordActive  = parseColour(colors["recordActive"],   theme.recordActive);
        theme.recordInactive= parseColour(colors["recordInactive"], theme.recordInactive);
    }

    if (fonts.isObject())
    {
        if (fonts["main"].isString()) theme.fontMain = fonts["main"].toString();
        if (fonts["mono"].isString()) theme.fontMono = fonts["mono"].toString();
    }

    if (sizes.isObject())
    {
        if (sizes["fontMain"].isDouble())  theme.fontSize      = (float)sizes["fontMain"].operator double();
        if (sizes["fontSmall"].isDouble()) theme.fontSizeSmall = (float)sizes["fontSmall"].operator double();
        if (sizes["fontTitle"].isDouble()) theme.fontSizeTitle  = (float)sizes["fontTitle"].operator double();
        if (sizes["fontMono"].isDouble())  theme.fontSizeMono  = (float)sizes["fontMono"].operator double();
    }

    // Preview image
    auto preview = folder.getChildFile("preview.png");
    if (preview.existsAsFile())
        theme.previewImage = preview;

    // Collect font files from Fonts/ subfolder
    auto fontsDir = folder.getChildFile("Fonts");
    if (fontsDir.isDirectory())
    {
        for (auto& f : fontsDir.findChildFiles(juce::File::findFiles, false, "*.ttf"))
            theme.fontFiles.add(f);
        for (auto& f : fontsDir.findChildFiles(juce::File::findFiles, false, "*.otf"))
            theme.fontFiles.add(f);
    }

    return theme;
}

// ================================================================
// Font loading from a theme folder
// ================================================================
void ThemeLoader::loadFontsFromFolder(const juce::File& fontsDir)
{
    for (auto& f : fontsDir.findChildFiles(juce::File::findFiles, false, "*.ttf"))
    {
        auto data = f.loadFileAsString();
        juce::Typeface::createSystemTypefaceFor(data.toRawUTF8(), data.getNumBytesAsUTF8());
    }
    for (auto& f : fontsDir.findChildFiles(juce::File::findFiles, false, "*.otf"))
    {
        auto data = f.loadFileAsString();
        juce::Typeface::createSystemTypefaceFor(data.toRawUTF8(), data.getNumBytesAsUTF8());
    }
}

// ================================================================
// Colour parsing
// ================================================================
juce::Colour ThemeLoader::parseColour(const juce::var& v, const juce::Colour& fallback)
{
    if (!v.isString())
        return fallback;

    auto s = v.toString().trim();
    if (s.startsWith("#") && s.length() == 7)
        return juce::Colour::fromString("ff" + s.substring(1));
    if (s.startsWith("#") && s.length() == 9)
        return juce::Colour::fromString(s.substring(1));

    return fallback;
}




