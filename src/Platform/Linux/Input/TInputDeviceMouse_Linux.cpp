// Linux adaptation of upstream WASM device; upstream license applies.
#include "ToshiPCH.h"
#include "TInputDeviceMouse_Linux.h"

#include <SDL2/SDL.h>

//-----------------------------------------------------------------------------
// Enables memory debugging.
// Note: Should be the last include!
//-----------------------------------------------------------------------------
#include "Core/TMemoryDebugOn.h"

TOSHI_NAMESPACE_START

TDEFINE_CLASS( TInputLinuxDeviceMouse );

TInputLinuxDeviceMouse::TInputLinuxDeviceMouse()
{
	m_events.clear();
	m_uiButtons        = 0;
	m_iPendingX        = 0;
	m_iPendingY        = 0;
	m_iPendingSteps    = 0;
	m_iDeltaX          = 0;
	m_iDeltaY          = 0;
	m_iDeltaSteps      = 0;

}

TInputLinuxDeviceMouse::~TInputLinuxDeviceMouse()
{
}

void TInputLinuxDeviceMouse::SetButton(TINT button, TBOOL down) {
    if (IsAcquired() && button >= 0 && button < 8) m_events.emplace_back(button, down);
}
void TInputLinuxDeviceMouse::ReleaseButtons() {
    for (int i = 0; i < 8; ++i) SetButton(i, TFALSE);
    m_iPendingX = m_iPendingY = m_iPendingSteps = 0;
    StopAllRepeats();
}
void TInputLinuxDeviceMouse::HandleEvent(const SDL_Event& event) {
    if (!IsAcquired()) return;
    if (event.type == SDL_MOUSEMOTION) AddMovement(event.motion.xrel, event.motion.yrel);
    if (event.type == SDL_MOUSEWHEEL)
        AddWheel(event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y);
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
        int b = event.button.button;
        // Engine order is left, right, middle, extra buttons.
        b = b == SDL_BUTTON_LEFT ? 0 : b == SDL_BUTTON_RIGHT ? 1 : b == SDL_BUTTON_MIDDLE ? 2 : b - 1;
        SetButton(b, event.type == SDL_MOUSEBUTTONDOWN);
    }
}

TBOOL TInputLinuxDeviceMouse::Acquire()
{
	SetAcquired( TTRUE );
	return TTRUE;
}

TBOOL TInputLinuxDeviceMouse::Unacquire()
{
	Flush();
	SetAcquired( TFALSE );
	return TTRUE;
}

void TInputLinuxDeviceMouse::Release()
{
}

void TInputLinuxDeviceMouse::Update( TFLOAT a_fDeltaTime )
{
	m_iDeltaX     = m_iPendingX;
	m_iDeltaY     = m_iPendingY;
	m_iDeltaSteps = m_iPendingSteps;

	m_iPendingX     = 0;
	m_iPendingY     = 0;
	m_iPendingSteps = 0;
}

TBOOL TInputLinuxDeviceMouse::Flush()
{
	m_events.clear();
	m_uiButtons = 0;
	m_iDeltaX = m_iDeltaY = m_iDeltaSteps = 0;
	StopAllRepeats();
	m_iPendingX     = 0;
	m_iPendingY     = 0;
	m_iPendingSteps = 0;
	return TTRUE;
}

TINT TInputLinuxDeviceMouse::ProcessEvents( EventEmitter& a_rEmitter, TFLOAT a_fDeltaTime )
{
	TINT iNumProcessed = ProcessRepeats( a_rEmitter, a_fDeltaTime );

    for (const auto& [button, down] : m_events) {
        TUINT32 bit = 1u << button;
        if (bool(m_uiButtons & bit) == down) continue;
        m_uiButtons = down ? m_uiButtons | bit : m_uiButtons & ~bit;
        a_rEmitter.Throw(TInputInterface::InputEvent(this, BUTTON_1 + button,
            down ? TInputInterface::EVENT_TYPE_GONE_DOWN : TInputInterface::EVENT_TYPE_GONE_UP));
        ++iNumProcessed;
    }
    m_events.clear();

	if ( m_iDeltaX != 0 || m_iDeltaY != 0 )
	{
		a_rEmitter.Throw( TInputInterface::InputEvent( this, AXIS_CURSOR, TInputInterface::EVENT_TYPE_MOVED, m_iDeltaX, m_iDeltaY ) );
		iNumProcessed++;
	}

	if ( m_iDeltaSteps != 0 )
	{
		a_rEmitter.Throw( TInputInterface::InputEvent( this, AXIS_WHEEL, TInputInterface::EVENT_TYPE_MOVED, 0.0f, TFLOAT( m_iDeltaSteps * 120 ) ) );

		const Doodad iButton = m_iDeltaSteps > 0 ? BUTTON_WHEEL_FORWARD : BUTTON_WHEEL_BACKWARD;
		a_rEmitter.Throw( TInputInterface::InputEvent( this, iButton, TInputInterface::EVENT_TYPE_GONE_DOWN ) );
		a_rEmitter.Throw( TInputInterface::InputEvent( this, iButton, TInputInterface::EVENT_TYPE_GONE_UP ) );
		iNumProcessed += 3;
	}

	return iNumProcessed;
}

TBOOL TInputLinuxDeviceMouse::IsDown( Doodad a_iDoodad ) const
{
	if ( a_iDoodad >= BUTTON_1 && a_iDoodad <= BUTTON_8 )
		return ( m_uiButtons & ( 1u << ( a_iDoodad - BUTTON_1 ) ) ) != 0;

	return TFALSE;
}

TINT TInputLinuxDeviceMouse::GetAxisInt( Doodad a_iDoodad, TINT a_iAxis ) const
{
	if ( a_iDoodad == AXIS_CURSOR )
		return a_iAxis == 0 ? m_iDeltaX : a_iAxis == 1 ? m_iDeltaY : 0;

	return 0;
}

TFLOAT TInputLinuxDeviceMouse::GetAxisFloat( Doodad a_iDoodad, TINT a_iAxis ) const
{
	if ( a_iDoodad == AXIS_WHEEL )
		return TFLOAT( m_iDeltaSteps * 120 );

	return TFLOAT( GetAxisInt( a_iDoodad, a_iAxis ) );
}

TOSHI_NAMESPACE_END
