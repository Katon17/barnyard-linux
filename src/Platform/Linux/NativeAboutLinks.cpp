#include "NativeAboutLinks.h"
#include <SDL2/SDL.h>
namespace BarnyardLinux {
const char* DonationURL(unsigned index) {
    static constexpr const char* urls[]={"https://saweria.co/Kabako","https://ko-fi.com/katonbagaskoro"};
    return index<2 ? urls[index] : nullptr;
}
bool OpenDonationPage(unsigned index) {
    const char* url=DonationURL(index);
    return url && SDL_OpenURL(url)==0;
}
}
