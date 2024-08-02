/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <vector>
#include <unordered_map>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>

#include "input.h"

#include <kos.h>

//print >>f, "int inp_key_code(const char *key_name) { int i; if (!strcmp(key_name, \"-?-\")) return -1; else for (i = 0; i < 512; i++) if (!strcmp(key_strings[i], key_name)) return i; return -1; }"

// this header is protected so you don't include it from anywere
#define KEYS_INCLUDE
#include "keynames.h"
#undef KEYS_INCLUDE

/*
std::unordered_map<int, std::vector<int> > _3DSkeys = {
	{KEY_LEFT, {KEY_a, KEY_MINUS}},
	{KEY_RIGHT, {KEY_d, KEY_EQUALS}},
	{KEY_UP, {KEY_SPACE}},
	{KEY_A, {KEY_RETURN}},
	{KEY_B, {KEY_SPACE}},
	{KEY_Y, {KEY_MOUSE_1}},
	{KEY_TOUCH, {KEY_MOUSE_1}},
	{KEY_L, {KEY_MOUSE_2}},
	{KEY_R, {KEY_MOUSE_WHEEL_DOWN}},
	{KEY_START, {KEY_ESCAPE}},
};
*/

void CInput::AddEvent(int Unicode, int Key, int Flags)
{
	if(m_NumEvents != INPUT_BUFFER_SIZE)
	{
		m_aInputEvents[m_NumEvents].m_Unicode = Unicode;
		m_aInputEvents[m_NumEvents].m_Key = Key;
		m_aInputEvents[m_NumEvents].m_Flags = Flags;
		m_NumEvents++;
	}
}

static int MapKey(int k)
{
	if (k >= KBD_KEY_A  && k <= KBD_KEY_Z)   return KEY_a  + (k - KBD_KEY_A);
	if (k >= KBD_KEY_0  && k <= KBD_KEY_9)   return KEY_0  + (k - KBD_KEY_0);
	if (k >= KBD_KEY_F1 && k <= KBD_KEY_F12) return KEY_F1 + (k - KBD_KEY_F1);
	// KBD_KEY_PAD_0 isn't before KBD_KEY_PAD_1
	if (k >= KBD_KEY_PAD_1 && k <= KBD_KEY_PAD_9) return KEY_KP1 + (k - KBD_KEY_PAD_1);

	switch (k) {
	case KBD_KEY_ENTER:     return KEY_RETURN;
	case KBD_KEY_ESCAPE:    return KEY_ESCAPE;
	case KBD_KEY_BACKSPACE: return KEY_BACKSPACE;
	case KBD_KEY_TAB:       return KEY_TAB;
	case KBD_KEY_SPACE:     return KEY_SPACE;
	case KBD_KEY_MINUS:     return KEY_MINUS;
	case KBD_KEY_PLUS:      return KEY_EQUALS;
	case KBD_KEY_LBRACKET:  return KEY_LEFTBRACKET;
	case KBD_KEY_RBRACKET:  return KEY_RIGHTBRACKET;
	case KBD_KEY_BACKSLASH: return KEY_BACKSLASH;
	case KBD_KEY_SEMICOLON: return KEY_SEMICOLON;
	case KBD_KEY_QUOTE:     return KEY_QUOTE;
	case KBD_KEY_TILDE:     return KEY_BACKQUOTE;
	case KBD_KEY_COMMA:     return KEY_COMMA;
	case KBD_KEY_PERIOD:    return KEY_PERIOD;
	case KBD_KEY_SLASH:     return KEY_SLASH;
	case KBD_KEY_CAPSLOCK:  return KEY_CAPSLOCK;
	case KBD_KEY_PRINT:     return KEY_PRINT;
	case KBD_KEY_SCRLOCK:   return KEY_SCROLLOCK;
	case KBD_KEY_PAUSE:     return KEY_PAUSE;
	case KBD_KEY_INSERT:    return KEY_INSERT;
	case KBD_KEY_HOME:      return KEY_HOME;
	case KBD_KEY_PGUP:      return KEY_PAGEUP;
	case KBD_KEY_DEL:       return KEY_DELETE;
	case KBD_KEY_END:       return KEY_END;
	case KBD_KEY_PGDOWN:    return KEY_PAGEDOWN;
	case KBD_KEY_RIGHT:     return KEY_RIGHTARROW;
	case KBD_KEY_LEFT:      return KEY_LEFTARROW;
	case KBD_KEY_DOWN:      return KEY_DOWNARROW;
	case KBD_KEY_UP:        return KEY_UPARROW;

	case KBD_KEY_PAD_NUMLOCK:  return KEY_NUMLOCK;
	case KBD_KEY_PAD_DIVIDE:   return KEY_KP_DIVIDE;
	case KBD_KEY_PAD_MULTIPLY: return KEY_KP_MULTIPLY;
	case KBD_KEY_PAD_MINUS:    return KEY_KP_MINUS;
	case KBD_KEY_PAD_PLUS:     return KEY_KP_PLUS;
	case KBD_KEY_PAD_ENTER:    return KEY_KP_ENTER;
	case KBD_KEY_PAD_0:        return KEY_KP0;
	case KBD_KEY_PAD_PERIOD:   return KEY_KP_PERIOD;
	}
	return KEY_UNKNOWN;
}

void CInput::HandleKeyboardInput(void* pKbd, void* pKbdState)
{
	static kbd_state_t prevState = {0};

	maple_device_t *kbd = (maple_device_t *)pKbd;
	kbd_state_t *kstate = (kbd_state_t *)pKbdState;

	int Unicode = 0, ret;
	while ( (ret = kbd_queue_pop(kbd, 1)) != -1 )
		Unicode = ret;

	if (Unicode)
	{
		AddEvent(Unicode, KEY_UNKNOWN, IInput::FLAG_PRESS);
	}

	for (int i = KBD_KEY_A; i < KBD_KEY_S3; i++)
	{
		if (kstate->matrix[i] == prevState.matrix[i]) continue;
		int Key = MapKey(i);
		if (Key == KEY_UNKNOWN) continue;

		int Action = -1;

		switch (kstate->matrix[i])
		{
			case 0: // None
				Action = IInput::FLAG_RELEASE;
				break;
			case 1:
				Action = IInput::FLAG_PRESS;
				break;
		}

		if (Action != -1)
		{
			m_aInputCount[m_InputCurrent][Key].m_Presses++;
			if (Action == IInput::FLAG_PRESS) m_aInputState[m_InputCurrent][Key] = 1;
			AddEvent(0, Key, Action);
		}
	}

	prevState = *kstate;
}

void CInput::HandleMouseInput(void* pMstate)
{
	mouse_state_t *mstate = (mouse_state_t *)pMstate;

	static int oldBtns = 0;

	int down = mstate->buttons &~ oldBtns;
	int up = oldBtns &~ mstate->buttons;
	int held = mstate->buttons;

	if (held & MOUSE_LEFTBUTTON) m_aInputState[m_InputCurrent][KEY_MOUSE_1] = 1;
	if (held & MOUSE_RIGHTBUTTON) m_aInputState[m_InputCurrent][KEY_MOUSE_2] = 1;
	if (held & MOUSE_SIDEBUTTON) m_aInputState[m_InputCurrent][KEY_MOUSE_3] = 1;

	int Key = -1;
	if (down)
	{
		if (down & MOUSE_LEFTBUTTON) Key = KEY_MOUSE_1;
		if (down & MOUSE_RIGHTBUTTON) Key = KEY_MOUSE_2;
		if (down & MOUSE_SIDEBUTTON) Key = KEY_MOUSE_3;

		if (Key != -1)
		{
			m_aInputCount[m_InputCurrent][Key].m_Presses++;
			m_aInputState[m_InputCurrent][Key] = 1;
			AddEvent(0, Key, IInput::FLAG_PRESS);
		}
	}

	Key = -1;
	if (up)
	{
		if (up & MOUSE_LEFTBUTTON)
		{
			Key = KEY_MOUSE_1;
			m_ReleaseDelta = time_get() - m_LastRelease;
			m_LastRelease = time_get();
		}
		if (up & MOUSE_RIGHTBUTTON) Key = KEY_MOUSE_2;
		if (up & MOUSE_SIDEBUTTON) Key = KEY_MOUSE_3;

		if (Key != -1)
		{
			m_aInputCount[m_InputCurrent][Key].m_Presses++;
			AddEvent(0, Key, IInput::FLAG_RELEASE);
		}
	}

	Key = -1;
	if (mstate->dz > 0) Key = KEY_MOUSE_WHEEL_DOWN;
	if (mstate->dz < 0) Key = KEY_MOUSE_WHEEL_UP;
	if (Key != -1)
	{
		m_aInputCount[m_InputCurrent][Key].m_Presses++;
		m_aInputState[m_InputCurrent][Key] = 1;
		AddEvent(0, Key, IInput::FLAG_PRESS);
	}

	oldBtns = mstate->buttons;
}

CInput::CInput()
{
	mem_zero(m_aInputCount, sizeof(m_aInputCount));
	mem_zero(m_aInputState, sizeof(m_aInputState));

	m_InputCurrent = 0;
	m_InputGrabbed = 0;
	m_InputDispatched = false;

	m_LastRelease = 0;
	m_ReleaseDelta = -1;

	m_NumEvents = 0;

	m_VideoRestartNeeded = 0;
}

void CInput::Init()
{
	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();

	cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y, (cont_btn_callback_t)arch_exit);
}

void CInput::MouseRelative(float *x, float *y)
{
	int nx = 0, ny = 0;
	float Sens = ((g_Config.m_ClDyncam && g_Config.m_ClDyncamMousesens) ? g_Config.m_ClDyncamMousesens : g_Config.m_InpMousesens) / 100.0f;

	maple_device_t *mouse = maple_enum_type(0, MAPLE_FUNC_MOUSE);
	if (!mouse)
	{
		*x = 0;
		*y = 0;
		return;
	}

	mouse_state_t *mstate = (mouse_state_t *)maple_dev_status(mouse);
	if (!mstate)
	{
		*x = 0;
		*y = 0;
		return;
	}

	nx = mstate->dx;
	ny = mstate->dy;

	*x = nx*Sens;
	*y = ny*Sens;
}

void CInput::MouseModeAbsolute()
{
	m_InputGrabbed = 0;
}

void CInput::MouseModeRelative()
{
	m_InputGrabbed = 1;
}

int CInput::MouseDoubleClick()
{
	if(m_ReleaseDelta >= 0 && m_ReleaseDelta < (time_freq() / 3))
	{
		m_LastRelease = 0;
		m_ReleaseDelta = -1;
		return 1;
	}
	return 0;
}

void CInput::ClearKeyStates()
{
	mem_zero(m_aInputState, sizeof(m_aInputState));
	mem_zero(m_aInputCount, sizeof(m_aInputCount));
}

int CInput::KeyState(int Key)
{
	return m_aInputState[m_InputCurrent][Key];
}

int CInput::Update()
{
	if(m_InputGrabbed && !Graphics()->WindowActive())
		MouseModeAbsolute();

	/*if(!input_grabbed && Graphics()->WindowActive())
		Input()->MouseModeRelative();*/

	if(m_InputDispatched)
	{
		// clear and begin count on the other one
		m_InputCurrent^=1;
		mem_zero(&m_aInputCount[m_InputCurrent], sizeof(m_aInputCount[m_InputCurrent]));
		mem_zero(&m_aInputState[m_InputCurrent], sizeof(m_aInputState[m_InputCurrent]));
		m_InputDispatched = false;
	}

	/*
	hidScanInput();

	u32 held = hidKeysHeld();
	if (held & KEY_TOUCH || held & KEY_Y) m_aInputState[m_InputCurrent][KEY_MOUSE_1] = 1;
	if (held & KEY_L) m_aInputState[m_InputCurrent][KEY_MOUSE_2] = 1;

	u32 down = hidKeysDown();

	for(auto it = _3DSkeys.begin(); it != _3DSkeys.end(); ++it)
	{
		if (!(down & it->first)) continue;
		for (int& Key : it->second)
		{
			m_aInputCount[m_InputCurrent][Key].m_Presses++;
			m_aInputState[m_InputCurrent][Key] = 1;
			AddEvent(0, Key, IInput::FLAG_PRESS);
		}
	}

	u32 up = hidKeysUp();

	for(auto it = _3DSkeys.begin(); it != _3DSkeys.end(); ++it)
	{
		if (!(up & it->first)) continue;
		for (int& Key : it->second)
		{
			m_aInputCount[m_InputCurrent][Key].m_Presses++;
			AddEvent(0, Key, IInput::FLAG_RELEASE);
		}
	}
	*/

	maple_device_t *cont, *mouse, *kbd;
	cont_state_t *cstate;
	mouse_state_t *mstate;
	kbd_state_t *kstate;

	mouse = maple_enum_type(0, MAPLE_FUNC_MOUSE);
	if (mouse)
	{
		mstate = (mouse_state_t *)maple_dev_status(mouse);
		if (mstate)
			HandleMouseInput((void*)mstate);
	}

	kbd = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
	if (kbd)
	{
		kstate = (kbd_state_t *)maple_dev_status(kbd);
		if (kstate)
			HandleKeyboardInput((void*)kbd, (void*)kstate);
	}

	cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	if (cont)
	{
		cstate = (cont_state_t *)maple_dev_status(cont);
		if (cstate)
		{
			
		}
	}

	/*
	{
		int i;
		Uint8 *pState = SDL_GetKeyState(&i);
		if(i >= KEY_LAST)
			i = KEY_LAST-1;
		mem_copy(m_aInputState[m_InputCurrent], pState, i);
	}

	// these states must always be updated manually because they are not in the GetKeyState from SDL
	int i = SDL_GetMouseState(NULL, NULL);
	if(i&SDL_BUTTON(1)) m_aInputState[m_InputCurrent][KEY_MOUSE_1] = 1; // 1 is left
	if(i&SDL_BUTTON(3)) m_aInputState[m_InputCurrent][KEY_MOUSE_2] = 1; // 3 is right
	if(i&SDL_BUTTON(2)) m_aInputState[m_InputCurrent][KEY_MOUSE_3] = 1; // 2 is middle
	if(i&SDL_BUTTON(4)) m_aInputState[m_InputCurrent][KEY_MOUSE_4] = 1;
	if(i&SDL_BUTTON(5)) m_aInputState[m_InputCurrent][KEY_MOUSE_5] = 1;
	if(i&SDL_BUTTON(6)) m_aInputState[m_InputCurrent][KEY_MOUSE_6] = 1;
	if(i&SDL_BUTTON(7)) m_aInputState[m_InputCurrent][KEY_MOUSE_7] = 1;
	if(i&SDL_BUTTON(8)) m_aInputState[m_InputCurrent][KEY_MOUSE_8] = 1;
	if(i&SDL_BUTTON(9)) m_aInputState[m_InputCurrent][KEY_MOUSE_9] = 1;

	{
		SDL_Event Event;

		while(SDL_PollEvent(&Event))
		{
			int Key = -1;
			int Action = IInput::FLAG_PRESS;
			switch (Event.type)
			{
				// handle keys
				case SDL_KEYDOWN:
					// skip private use area of the BMP(contains the unicodes for keyboard function keys on MacOS)
					if(Event.key.keysym.unicode < 0xE000 || Event.key.keysym.unicode > 0xF8FF)	// ignore_convention
						AddEvent(Event.key.keysym.unicode, 0, 0); // ignore_convention
					Key = Event.key.keysym.sym; // ignore_convention
					break;
				case SDL_KEYUP:
					Action = IInput::FLAG_RELEASE;
					Key = Event.key.keysym.sym; // ignore_convention
					break;

				// handle mouse buttons
				case SDL_MOUSEBUTTONUP:
					Action = IInput::FLAG_RELEASE;

					if(Event.button.button == 1) // ignore_convention
					{
						m_ReleaseDelta = time_get() - m_LastRelease;
						m_LastRelease = time_get();
					}

					// fall through
				case SDL_MOUSEBUTTONDOWN:
					if(Event.button.button == SDL_BUTTON_LEFT) Key = KEY_MOUSE_1; // ignore_convention
					if(Event.button.button == SDL_BUTTON_RIGHT) Key = KEY_MOUSE_2; // ignore_convention
					if(Event.button.button == SDL_BUTTON_MIDDLE) Key = KEY_MOUSE_3; // ignore_convention
					if(Event.button.button == SDL_BUTTON_WHEELUP) Key = KEY_MOUSE_WHEEL_UP; // ignore_convention
					if(Event.button.button == SDL_BUTTON_WHEELDOWN) Key = KEY_MOUSE_WHEEL_DOWN; // ignore_convention
					if(Event.button.button == 6) Key = KEY_MOUSE_6; // ignore_convention
					if(Event.button.button == 7) Key = KEY_MOUSE_7; // ignore_convention
					if(Event.button.button == 8) Key = KEY_MOUSE_8; // ignore_convention
					if(Event.button.button == 9) Key = KEY_MOUSE_9; // ignore_convention
					break;

				// other messages
				case SDL_QUIT:
					return 1;

#if defined(__ANDROID__)
				case SDL_VIDEORESIZE:
					m_VideoRestartNeeded = 1;
					break;
#endif
			}

			//
			if(Key != -1)
			{
				m_aInputCount[m_InputCurrent][Key].m_Presses++;
				if(Action == IInput::FLAG_PRESS)
					m_aInputState[m_InputCurrent][Key] = 1;
				AddEvent(0, Key, Action);
			}

		}
	}
	*/

	return 0;
}

int CInput::VideoRestartNeeded()
{
	if( m_VideoRestartNeeded )
	{
		m_VideoRestartNeeded = 0;
		return 1;
	}
	return 0;
}

IEngineInput *CreateEngineInput() { return new CInput; }
