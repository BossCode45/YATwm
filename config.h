#include <X11/keysym.h>

typedef union
{
	const char** str;
	const int* num;
} KeyArg;

struct Key
{
	KeySym keysym;
	unsigned int modifiers;
	void (*function)(const KeyArg arg);
	const KeyArg arg;
};

//Keybind commands
void exit(const KeyArg arg);
void spawn(const KeyArg arg);
void toggle(const KeyArg arg);
void kill(const KeyArg arg);

const char* alacritty[] = {"alacritty", NULL};
const char* rofi[] = {"rofi", "-i", "-show" "drun", NULL};

unsigned int mod = Mod1Mask;

static struct Key keyBinds[] = {
	//Key			//Modifiers		//Func			//Args
	{XK_E, 			mod,			exit,			{NULL}},
	{XK_Return,		mod, 			spawn,			{alacritty}},
	{XK_D,			mod,	 		spawn,			{rofi}},
	{XK_T,			mod,			toggle,			{NULL}},
	{XK_Q,			mod,			kill,			{NULL}}
};
