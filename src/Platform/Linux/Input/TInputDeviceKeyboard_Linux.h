#pragma once
#include "Input/TInputDeviceKeyboard.h"
#include <SDL2/SDL.h>
#include <vector>
#include <utility>

TOSHI_NAMESPACE_START

class TInputLinuxDeviceKeyboard : public TInputDeviceKeyboard
{
public:
	TDECLARE_CLASS( TInputLinuxDeviceKeyboard, TInputDeviceKeyboard );

	static constexpr TINT MAX_KEY_INDEX  = 256;

public:
	TInputLinuxDeviceKeyboard();
	~TInputLinuxDeviceKeyboard();

	virtual TBOOL   Acquire() override;
	virtual TBOOL   Unacquire() override;
	virtual void    Release() override;
	virtual void    Update( TFLOAT a_fDeltaTime ) override;
	virtual TBOOL   Flush() override;
	virtual TINT    ProcessEvents( EventEmitter& a_rEmitter, TFLOAT a_fDeltaTime ) override;
	virtual TINT    GetButtonCount() const override { return MAX_KEY_INDEX; }
	virtual TBOOL   IsDown( Doodad a_iDoodad ) const override;
	virtual TBOOL   IsEnabled() const override { return TTRUE; }
	virtual TBOOL   IsShiftDown() const override;
	virtual TBOOL   IsControlDown() const override;
	virtual TBOOL   IsAltDown() const override;
	virtual TBOOL   WasDown( Doodad a_iDoodad ) const override;
	virtual TWCHAR* TranslateDoodadToCharacter( Doodad a_iDoodad ) const override;

	void HandleEvent(const SDL_Event& event);
	void ReleaseKeys();
	void QueueKey( Doodad a_iDoodad, TBOOL a_bDown );

private:
	TBOOL  m_abDown[ MAX_KEY_INDEX ];
	TBOOL  m_abWasDown[ MAX_KEY_INDEX ];
	std::vector<std::pair<Doodad, bool>> m_events;
};

TOSHI_NAMESPACE_END
