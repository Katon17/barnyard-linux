#include "Platform/Linux/NativeAboutLinks.h"
#include <SDL2/SDL.h>
#include <cassert>
#include <cstring>
#include <iostream>
static int calls=0,result=0;
static const char* last=nullptr;
extern "C" int SDLCALL SDL_OpenURL(const char* url){++calls;last=url;return result;}
int main(){
 using namespace BarnyardLinux;
 assert(std::strcmp(DonationURL(0),"https://saweria.co/Kabako")==0);
 assert(std::strcmp(DonationURL(1),"https://ko-fi.com/katonbagaskoro")==0);
 assert(!DonationURL(2) && !DonationURL(999));
 assert(calls==0); // No browser on lookup or menu creation.
 assert(OpenDonationPage(0) && calls==1 && std::strcmp(last,DonationURL(0))==0);
 assert(OpenDonationPage(1) && calls==2 && std::strcmp(last,DonationURL(1))==0);
 assert(!OpenDonationPage(2) && calls==2);
 result=-1;assert(!OpenDonationPage(1) && calls==3);
 std::cout<<"PASS: exact fixed HTTPS donation URLs, explicit activation only, invalid index rejected, browser-dispatch success/failure paths. SDL dispatcher intercepted; no real browser/payment/network used.\n";
}
