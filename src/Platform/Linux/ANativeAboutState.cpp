#include "pch.h"
#include "ANativeAboutState.h"
#include "NativeAboutLinks.h"
#include "NativePreferences.h"
#include "GUI/AGUI2.h"
#include "GUI/T2GUIFontManager.h"
#include "Sound/ASoundManager.h"
#include "SoundBank/ui.h"
#include <cstdio>
#include <Core/TMemoryDebugOn.h>
TOSHI_NAMESPACE_USING
TDEFINE_CLASS(ANativeAboutState);

void ANativeAboutState::OnInsertion() {
    const bool id=BarnyardLinux::NativePreferences().indonesian;
    auto* font=T2GUIFontManager::FindFont("Rekord18");
    if(!font)font=T2GUIFontManager::FindFont(AGUI2STYLE_FONT_PRIMARY);
    const TWCHAR* labels[]={L"Saweria: saweria.co/Kabako",L"Ko-fi: ko-fi.com/katonbagaskoro",id?L"Kembali":L"Back"};
    for(int i=0;i<3;++i) {
        auto& b=m_buttons[i];
        b.Create(font,font->GetTextWidth(labels[i]),labels[i],8.0f);
        b.SetFocusedColour(AGUI2STYLE_COLOR_FOCUSED);
        b.SetEnabledColour(AGUI2STYLE_COLOR_SECONDARY);
        b.SetDisabledColour(AGUI2STYLE_COLOR_DISABLED);
        b.SetFocusedScale(1.0f);b.OnFocusLost();b.SetId(i);
        b.SetTextShadow(TTRUE,0xa0000000,0.6f);
        m_oMenu.AddMenuItem(b);
    }
    m_oMenu.SetFocusAt(m_buttons[2]); // Back is the safe default, never opens a URL automatically.
    m_oDialogTitle.SetText(id?L"Tentang":L"About");
    BaseClass::OnInsertion();
    TFLOAT rootWidth,rootHeight;AGUI2::GetRootElement()->GetDimensions(rootWidth,rootHeight);
    const TFLOAT width=TMath::Min(rootWidth-48.0f,600.0f);
    m_body.Create(font,width-56.0f);m_body.SetScale(0.95f);
    m_body.SetText(id ?
        L"Barnyard - Native Linux\nPort native Linux oleh\nKaton Bagaskoro\n\n"
        L"Game asli: Blue Tongue Entertainment / THQ\n"
        L"Toshi kini dimiliki THQ Nordic\n"
        L"OpenBarnyard / dekompilasi Toshi:\nAdventureT dan InfiniteC0re\n\n"
        L"Port komunitas tidak resmi, tanpa afiliasi.\n"
        L"Donasi sukarela, bukan syarat untuk bermain."
        :
        L"Barnyard - Native Linux\nNative Linux port by\nKaton Bagaskoro\n\n"
        L"Original game: Blue Tongue Entertainment / THQ\n"
        L"Toshi is now owned by THQ Nordic\n"
        L"OpenBarnyard / Toshi decompilation:\nAdventureT and InfiniteC0re\n\n"
        L"Unofficial community port, not affiliated.\n"
        L"Donations are optional, never required to play.");
    m_body.SetColour(0xfffff0af);m_body.SetShadow(TTRUE,0xa0000000);
    m_body.SetShadowOffset(2.0f,2.0f);
    TFLOAT bodyHeight=m_body.GetHeight(),menuHeight=m_oMenu.GetHeight();
    if(bodyHeight+menuHeight+100.0f>rootHeight) {
        m_body.SetScale(0.95f*TMath::Max(0.65f,(rootHeight-menuHeight-100.0f)/bodyHeight));
        bodyHeight=m_body.GetHeight();
    }
    const TFLOAT gap=16.0f;
    m_body.SetTransform(0.0f,-(menuHeight+gap)*0.5f-8.0f);
    m_oMenu.SetTransform(0.0f,(bodyHeight+gap)*0.5f-8.0f);
    m_oDialog.SetDialogSize(width,bodyHeight+menuHeight+gap+88.0f);
    m_oRootElement.AddChildTail(m_body);
    m_status.Create(font,width-40.0f);m_status.SetScale(0.75f);
    m_status.SetText(id?L"Tombol donasi membuka browser sistem.":L"Donation buttons open your system browser.");
    m_status.SetColour(0xffffffff);
    m_status.SetTransform(0.0f,(bodyHeight+menuHeight+gap)*0.5f+14.0f);
    m_oRootElement.AddChildTail(m_status);
}
TBOOL ANativeAboutState::OnUpdate(TFLOAT dt) {
    m_body.SetAlpha(m_fMenuOpacity);m_body.SetShadowAlpha(m_fMenuOpacity*0.6f);
    m_status.SetAlpha(m_fMenuOpacity);
    // The base can synchronously pop/delete this state on closing.
    return BaseClass::OnUpdate(dt);
}
void ANativeAboutState::OnMenuItemActivated(AGUI2MenuItem* item) {
    int choice=TSTATICCAST(AGUI2Button,item)->GetId();
    if(choice==2){OnButtonActivated(&m_buttons[2]);return;}
    if(choice<0 || choice>1)return;
    const bool id=BarnyardLinux::NativePreferences().indonesian;
    if(!BarnyardLinux::OpenDonationPage(static_cast<unsigned>(choice))) {
        fprintf(stderr,"About: system browser could not open %s\n",BarnyardLinux::DonationURL(choice));
        m_status.SetText(id?L"Browser gagal dibuka. Gunakan alamat pada tombol.":L"Could not open browser. Use the address on the button.");
        ASoundManager::GetSingleton()->PlayCue(soundbank::UI_NEGATIVE);
    } else {
        m_status.SetText(id?L"Permintaan membuka browser dikirim. Alt+Tab untuk kembali.":L"Browser launch requested. Alt+Tab to return to the game.");
    }
}
void ANativeAboutState::OnRemoval() {
    m_body.RemoveSelf();m_status.RemoveSelf();
    for(auto& b:m_buttons)b.RemoveSelf();
    BaseClass::OnRemoval();
}
