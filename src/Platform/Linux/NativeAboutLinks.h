#pragma once
namespace BarnyardLinux {
// Only these fixed HTTPS destinations can be opened; no shell/user-supplied URL.
const char* DonationURL(unsigned index);
bool OpenDonationPage(unsigned index);
}
