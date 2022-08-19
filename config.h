#include <X11/keysym.h>

struct Key
{
	KeySym keysym;
	unsigned int modifiers;
};

Key keyBinds[] = {
	{XK_E, 		Mod1Mask},
	{XK_Return,	Mod1Mask},
	{XK_D,		Mod1Mask},
	{XK_T,		Mod1Mask}
};
   
