#pragma once
#include "GameInterface/ABYardMenuState.h"
// Linux community port credits. Original game/engine ownership is unchanged.
class ANativeAboutState : public ABYardMenuState {
public:
    TDECLARE_CLASS(ANativeAboutState,ABYardMenuState);
    void OnInsertion() override;
    void OnRemoval() override;
    void OnMenuClose() override { AGameState::Remove(); }
    TBOOL OnUpdate(TFLOAT dt) override;
    void OnMenuItemActivated(AGUI2MenuItem* item) override;
private:
    AGUI2TextBox m_body;
    AGUI2TextBox m_status;
    AGUI2Button m_buttons[3];
};
