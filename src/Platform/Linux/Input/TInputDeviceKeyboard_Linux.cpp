// Linux adaptation of upstream WASM device; upstream license applies.
#include "ToshiPCH.h"
#include "TInputDeviceKeyboard_Linux.h"

#include <SDL2/SDL.h>
#include <string.h>

//-----------------------------------------------------------------------------
// Enables memory debugging.
// Note: Should be the last include!
//-----------------------------------------------------------------------------
#include "Core/TMemoryDebugOn.h"

TOSHI_NAMESPACE_START

TDEFINE_CLASS( TInputLinuxDeviceKeyboard );

struct LinuxKeyName { SDL_Scancode scancode; TINT doodad; };
static const LinuxKeyName s_keys[] = {
    { SDL_SCANCODE_ESCAPE, TInputDeviceKeyboard::KEY_ESCAPE },
    { SDL_SCANCODE_1, TInputDeviceKeyboard::KEY_1 },
    { SDL_SCANCODE_2, TInputDeviceKeyboard::KEY_2 },
    { SDL_SCANCODE_3, TInputDeviceKeyboard::KEY_3 },
    { SDL_SCANCODE_4, TInputDeviceKeyboard::KEY_4 },
    { SDL_SCANCODE_5, TInputDeviceKeyboard::KEY_5 },
    { SDL_SCANCODE_6, TInputDeviceKeyboard::KEY_6 },
    { SDL_SCANCODE_7, TInputDeviceKeyboard::KEY_7 },
    { SDL_SCANCODE_8, TInputDeviceKeyboard::KEY_8 },
    { SDL_SCANCODE_9, TInputDeviceKeyboard::KEY_9 },
    { SDL_SCANCODE_0, TInputDeviceKeyboard::KEY_0 },
    { SDL_SCANCODE_MINUS, TInputDeviceKeyboard::KEY_MINUS },
    { SDL_SCANCODE_EQUALS, TInputDeviceKeyboard::KEY_EQUALS },
    { SDL_SCANCODE_BACKSPACE, TInputDeviceKeyboard::KEY_BACK },
    { SDL_SCANCODE_TAB, TInputDeviceKeyboard::KEY_TAB },
    { SDL_SCANCODE_Q, TInputDeviceKeyboard::KEY_Q },
    { SDL_SCANCODE_W, TInputDeviceKeyboard::KEY_W },
    { SDL_SCANCODE_E, TInputDeviceKeyboard::KEY_E },
    { SDL_SCANCODE_R, TInputDeviceKeyboard::KEY_R },
    { SDL_SCANCODE_T, TInputDeviceKeyboard::KEY_T },
    { SDL_SCANCODE_Y, TInputDeviceKeyboard::KEY_Y },
    { SDL_SCANCODE_U, TInputDeviceKeyboard::KEY_U },
    { SDL_SCANCODE_I, TInputDeviceKeyboard::KEY_I },
    { SDL_SCANCODE_O, TInputDeviceKeyboard::KEY_O },
    { SDL_SCANCODE_P, TInputDeviceKeyboard::KEY_P },
    { SDL_SCANCODE_LEFTBRACKET, TInputDeviceKeyboard::KEY_LBRACKET },
    { SDL_SCANCODE_RIGHTBRACKET, TInputDeviceKeyboard::KEY_RBRACKET },
    { SDL_SCANCODE_RETURN, TInputDeviceKeyboard::KEY_RETURN },
    { SDL_SCANCODE_LCTRL, TInputDeviceKeyboard::KEY_LCONTROL },
    { SDL_SCANCODE_A, TInputDeviceKeyboard::KEY_A },
    { SDL_SCANCODE_S, TInputDeviceKeyboard::KEY_S },
    { SDL_SCANCODE_D, TInputDeviceKeyboard::KEY_D },
    { SDL_SCANCODE_F, TInputDeviceKeyboard::KEY_F },
    { SDL_SCANCODE_G, TInputDeviceKeyboard::KEY_G },
    { SDL_SCANCODE_H, TInputDeviceKeyboard::KEY_H },
    { SDL_SCANCODE_J, TInputDeviceKeyboard::KEY_J },
    { SDL_SCANCODE_K, TInputDeviceKeyboard::KEY_K },
    { SDL_SCANCODE_L, TInputDeviceKeyboard::KEY_L },
    { SDL_SCANCODE_SEMICOLON, TInputDeviceKeyboard::KEY_SEMICOLON },
    { SDL_SCANCODE_APOSTROPHE, TInputDeviceKeyboard::KEY_APOSTROPHE },
    { SDL_SCANCODE_GRAVE, TInputDeviceKeyboard::KEY_GRAVE },
    { SDL_SCANCODE_LSHIFT, TInputDeviceKeyboard::KEY_LSHIFT },
    { SDL_SCANCODE_BACKSLASH, TInputDeviceKeyboard::KEY_BACKSLASH },
    { SDL_SCANCODE_Z, TInputDeviceKeyboard::KEY_Z },
    { SDL_SCANCODE_X, TInputDeviceKeyboard::KEY_X },
    { SDL_SCANCODE_C, TInputDeviceKeyboard::KEY_C },
    { SDL_SCANCODE_V, TInputDeviceKeyboard::KEY_V },
    { SDL_SCANCODE_B, TInputDeviceKeyboard::KEY_B },
    { SDL_SCANCODE_N, TInputDeviceKeyboard::KEY_N },
    { SDL_SCANCODE_M, TInputDeviceKeyboard::KEY_M },
    { SDL_SCANCODE_COMMA, TInputDeviceKeyboard::KEY_COMMA },
    { SDL_SCANCODE_PERIOD, TInputDeviceKeyboard::KEY_PERIOD },
    { SDL_SCANCODE_SLASH, TInputDeviceKeyboard::KEY_SLASH },
    { SDL_SCANCODE_RSHIFT, TInputDeviceKeyboard::KEY_RSHIFT },
    { SDL_SCANCODE_KP_MULTIPLY, TInputDeviceKeyboard::KEY_MULTIPLY },
    { SDL_SCANCODE_LALT, TInputDeviceKeyboard::KEY_LMENU },
    { SDL_SCANCODE_SPACE, TInputDeviceKeyboard::KEY_SPACE },
    { SDL_SCANCODE_CAPSLOCK, TInputDeviceKeyboard::KEY_CAPITAL },
    { SDL_SCANCODE_F1, TInputDeviceKeyboard::KEY_F1 },
    { SDL_SCANCODE_F2, TInputDeviceKeyboard::KEY_F2 },
    { SDL_SCANCODE_F3, TInputDeviceKeyboard::KEY_F3 },
    { SDL_SCANCODE_F4, TInputDeviceKeyboard::KEY_F4 },
    { SDL_SCANCODE_F5, TInputDeviceKeyboard::KEY_F5 },
    { SDL_SCANCODE_F6, TInputDeviceKeyboard::KEY_F6 },
    { SDL_SCANCODE_F7, TInputDeviceKeyboard::KEY_F7 },
    { SDL_SCANCODE_F8, TInputDeviceKeyboard::KEY_F8 },
    { SDL_SCANCODE_F9, TInputDeviceKeyboard::KEY_F9 },
    { SDL_SCANCODE_F10, TInputDeviceKeyboard::KEY_F10 },
    { SDL_SCANCODE_NUMLOCKCLEAR, TInputDeviceKeyboard::KEY_NUMLOCK },
    { SDL_SCANCODE_SCROLLLOCK, TInputDeviceKeyboard::KEY_SCROLL },
    { SDL_SCANCODE_KP_7, TInputDeviceKeyboard::KEY_NUMPAD7 },
    { SDL_SCANCODE_KP_8, TInputDeviceKeyboard::KEY_NUMPAD8 },
    { SDL_SCANCODE_KP_9, TInputDeviceKeyboard::KEY_NUMPAD9 },
    { SDL_SCANCODE_KP_MINUS, TInputDeviceKeyboard::KEY_SUBTRACT },
    { SDL_SCANCODE_KP_4, TInputDeviceKeyboard::KEY_NUMPAD4 },
    { SDL_SCANCODE_KP_5, TInputDeviceKeyboard::KEY_NUMPAD5 },
    { SDL_SCANCODE_KP_6, TInputDeviceKeyboard::KEY_NUMPAD6 },
    { SDL_SCANCODE_KP_PLUS, TInputDeviceKeyboard::KEY_ADD },
    { SDL_SCANCODE_KP_1, TInputDeviceKeyboard::KEY_NUMPAD1 },
    { SDL_SCANCODE_KP_2, TInputDeviceKeyboard::KEY_NUMPAD2 },
    { SDL_SCANCODE_KP_3, TInputDeviceKeyboard::KEY_NUMPAD3 },
    { SDL_SCANCODE_KP_0, TInputDeviceKeyboard::KEY_NUMPAD0 },
    { SDL_SCANCODE_KP_PERIOD, TInputDeviceKeyboard::KEY_DECIMAL },
    { SDL_SCANCODE_F11, TInputDeviceKeyboard::KEY_F11 },
    { SDL_SCANCODE_F12, TInputDeviceKeyboard::KEY_F12 },
    { SDL_SCANCODE_KP_ENTER, TInputDeviceKeyboard::KEY_NUMPADENTER },
    { SDL_SCANCODE_RCTRL, TInputDeviceKeyboard::KEY_RCONTROL },
    { SDL_SCANCODE_KP_DIVIDE, TInputDeviceKeyboard::KEY_DIVIDE },
    { SDL_SCANCODE_RALT, TInputDeviceKeyboard::KEY_RMENU },
    { SDL_SCANCODE_HOME, TInputDeviceKeyboard::KEY_HOME },
    { SDL_SCANCODE_PAUSE, TInputDeviceKeyboard::KEY_PAUSE },
    { SDL_SCANCODE_UP, TInputDeviceKeyboard::KEY_UP },
    { SDL_SCANCODE_PAGEUP, TInputDeviceKeyboard::KEY_PRIOR },
    { SDL_SCANCODE_LEFT, TInputDeviceKeyboard::KEY_LEFT },
    { SDL_SCANCODE_RIGHT, TInputDeviceKeyboard::KEY_RIGHT },
    { SDL_SCANCODE_END, TInputDeviceKeyboard::KEY_END },
    { SDL_SCANCODE_DOWN, TInputDeviceKeyboard::KEY_DOWN },
    { SDL_SCANCODE_PAGEDOWN, TInputDeviceKeyboard::KEY_NEXT },
    { SDL_SCANCODE_INSERT, TInputDeviceKeyboard::KEY_INSERT },
    { SDL_SCANCODE_DELETE, TInputDeviceKeyboard::KEY_DELETE },
};

static TINT GetKeyIndex( TINT a_iDoodad )
{
	const TINT iIndex = a_iDoodad - 0x20000;
	return ( iIndex >= 0 && iIndex < TInputLinuxDeviceKeyboard::MAX_KEY_INDEX ) ? iIndex : -1;
}

TInputLinuxDeviceKeyboard::TInputLinuxDeviceKeyboard()
{
	TUtil::MemClear( m_abDown, sizeof( m_abDown ) );
	TUtil::MemClear( m_abWasDown, sizeof( m_abWasDown ) );
	TUtil::MemClear(m_aMags, sizeof(m_aMags));
	m_events.clear();

}

TInputLinuxDeviceKeyboard::~TInputLinuxDeviceKeyboard()
{
}

void TInputLinuxDeviceKeyboard::QueueKey(Doodad key, TBOOL down) {
    if (IsAcquired() && GetKeyIndex(key) >= 0) m_events.emplace_back(key, down);
}
void TInputLinuxDeviceKeyboard::HandleEvent(const SDL_Event& event) {
    if ((event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) || event.key.repeat) return;
    for (const auto& key : s_keys) if (key.scancode == event.key.keysym.scancode) {
        QueueKey(key.doodad, event.type == SDL_KEYDOWN); break;
    }
}
void TInputLinuxDeviceKeyboard::ReleaseKeys() {
    // Release even keys pressed earlier in this same (unprocessed) frame.
    for (TINT i = 0; i < MAX_KEY_INDEX; ++i) QueueKey(0x20000 + i, TFALSE);
    StopAllRepeats();
}

TBOOL TInputLinuxDeviceKeyboard::Acquire()
{
	SetAcquired( TTRUE );
	return TTRUE;
}

TBOOL TInputLinuxDeviceKeyboard::Unacquire()
{
	Flush();
	SetAcquired( TFALSE );
	return TTRUE;
}

void TInputLinuxDeviceKeyboard::Release()
{
}

void TInputLinuxDeviceKeyboard::Update( TFLOAT a_fDeltaTime )
{
}

TBOOL TInputLinuxDeviceKeyboard::Flush() {
    m_events.clear();
    memset(m_abDown, 0, sizeof(m_abDown));
    memset(m_abWasDown, 0, sizeof(m_abWasDown));
    memset(m_aMags, 0, sizeof(m_aMags));
    StopAllRepeats();
    return TTRUE;
}

TINT TInputLinuxDeviceKeyboard::ProcessEvents( EventEmitter& a_rEmitter, TFLOAT a_fDeltaTime )
{
	TINT iNumProcessed = ProcessRepeats( a_rEmitter, a_fDeltaTime );
	iNumProcessed += ProcessVirtualButtons( a_rEmitter, a_fDeltaTime );

	TUtil::MemCopy( m_abWasDown, m_abDown, sizeof( m_abDown ) );

	for (const auto& [key, down] : m_events)
	{
		const TINT iKey = GetKeyIndex( key );

		if ( iKey < 0 || m_abDown[ iKey ] == down )
			continue;

		m_abDown[ iKey ] = down;

		if ( down )
			a_rEmitter.Throw( TInputInterface::InputEvent( this, key, TInputInterface::EVENT_TYPE_GONE_DOWN, TranslateDoodadToCharacter( key ) ) );
		else
			a_rEmitter.Throw( TInputInterface::InputEvent( this, key, TInputInterface::EVENT_TYPE_GONE_UP ) );

		iNumProcessed++;
	}

	m_events.clear();
	UpdateVirtualButtons();

	return iNumProcessed;
}

TBOOL TInputLinuxDeviceKeyboard::IsDown( Doodad a_iDoodad ) const
{
	if ( a_iDoodad == VIRTKEY_SHIFT ) return IsShiftDown();
	if ( a_iDoodad == VIRTKEY_CONTROL ) return IsControlDown();
	if ( a_iDoodad == VIRTKEY_ALT ) return IsAltDown();

	const TINT iKey = GetKeyIndex( a_iDoodad );
	return iKey >= 0 && m_abDown[ iKey ];
}

TBOOL TInputLinuxDeviceKeyboard::WasDown( Doodad a_iDoodad ) const
{
	const TINT iKey = GetKeyIndex( a_iDoodad );
	return iKey >= 0 && m_abWasDown[ iKey ];
}

TBOOL TInputLinuxDeviceKeyboard::IsShiftDown() const
{
	return IsDown( KEY_LSHIFT ) || IsDown( KEY_RSHIFT );
}

TBOOL TInputLinuxDeviceKeyboard::IsControlDown() const
{
	return IsDown( KEY_LCONTROL ) || IsDown( KEY_RCONTROL );
}

TBOOL TInputLinuxDeviceKeyboard::IsAltDown() const
{
	return IsDown( KEY_LMENU ) || IsDown( KEY_RMENU );
}

TWCHAR* TInputLinuxDeviceKeyboard::TranslateDoodadToCharacter(Doodad doodad) const {
    static TWCHAR character[2]{};
    character[0] = character[1] = 0;
    for (const auto& key : s_keys) if (key.doodad == doodad) {
        if (key.scancode >= SDL_SCANCODE_A && key.scancode <= SDL_SCANCODE_Z)
            character[0] = (IsShiftDown() ? 'A' : 'a') + key.scancode - SDL_SCANCODE_A;
        else if (key.scancode >= SDL_SCANCODE_1 && key.scancode <= SDL_SCANCODE_9)
            character[0] = '1' + key.scancode - SDL_SCANCODE_1;
        else if (key.scancode == SDL_SCANCODE_0) character[0] = '0';
        else if (key.scancode == SDL_SCANCODE_SPACE) character[0] = ' ';
        break;
    }
    return character;
}
TOSHI_NAMESPACE_END
