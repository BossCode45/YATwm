#include <X11/keysym.h>
#include <X11/Xlib.h>

#include <vector>
#include <string>

//Startup
std::string startup[] = {"picom -fD 3", "feh --bg-scale /usr/share/backgrounds/vapor_trails_blue.png"};

//Main config
int gaps = 10;
int outerGaps = 30;

int numWS = 5;

//Keys
//The types and perhaps functions likely to be moved to seperate header file later
typedef union
{
	const char** str;
	const int num;
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
void changeWS(const KeyArg arg);

const char* alacritty[] = {"alacritty", NULL};
const char* rofi[] = {"rofi", "-i", "-show", "drun", NULL};

#define WSKEY(K, X) \
	{K, mod, changeWS, {.num = X - 1}},

unsigned int mod = Mod1Mask;

static struct Key keyBinds[] = {
	//Key			//Modifiers		//Func			//Args
	{XK_e, 			mod,			exit,			{NULL}},
	{XK_Return,		mod, 			spawn,			{.str = alacritty}},
	{XK_d,			mod,	 		spawn,			{.str = rofi}},
	{XK_t,			mod,			toggle,			{NULL}},
	{XK_q,			mod,			kill,			{NULL}},
	{XK_1,			mod,			changeWS,		{.num = 1}},
	{XK_2,			mod,			changeWS,		{.num = 2}},
	{XK_3,			mod,			changeWS,		{.num = 3}},
	{XK_4,			mod,			changeWS,		{.num = 4}},
	{XK_5,			mod,			changeWS,		{.num = 5}},
	//WSKEY(XK_1, 1)
	//WSKEY(XK_2, 2)
	//WSKEY(XK_3, 3)
	//WSKEY(XK_4, 4)
	//WSKEY(XK_5, 5)
};
