#pragma once
#include <cstdint>
#include <string>
namespace BarnyardLinux {
struct Preferences {
    bool fullscreen=true, captureMouse=true, indonesian=false;
    uint32_t windowWidth=800, windowHeight=600;
    bool showFPS=false;
};
const Preferences& NativePreferences();
bool StoreNativePreferences(const Preferences& value);
const std::string& PreferencesError();
void ReloadNativePreferences();
// Stable static UTF-16 translations; never changes the asset/audio language ID.
const char16_t* LocalizedText(const char16_t* original);
}
