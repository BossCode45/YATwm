#include <X11/keysym.h>
#include <X11/Xlib.h>

#include <vector>
#include <string>

//Startup
std::string startup[] = {"picom -fD 3", "feh --bg-scale /usr/share/backgrounds/vapor_trails_blue.png", "~/.config/polybar/launch.sh"};

//Main config
int gaps = 10;
int outerGaps = 30;

int numWS = 5;

//Keys
//The types and perhaps functions likely to be moved to seperate header file later
enum MoveDir
{
	Up,
	Right,
	Down,
	Left
};

typedef union
{
	const char** str;
	const int num;
	const MoveDir dir;
} KeyArg;

struct Key
{
	KeySym keysym;
	unsigned int modifiers;
	void (*function)(const KeyArg arg);
	const KeyArg arg;
};

//Keybind commands
#define KEYCOM(X) \
	void X (const KeyArg arg)
KEYCOM(exit);
KEYCOM(spawn);
KEYCOM(toggle);
KEYCOM(kill);
KEYCOM(changeWS);
KEYCOM(wToWS);
KEYCOM(focChange);
KEYCOM(wMove);

const char* alacritty[] = {"alacritty", NULL};
const char* rofi[] = {"rofi", "-i", "-show", "drun", NULL};

#define MOD Mod1Mask
#define SHIFT ShiftMask

#define WSKEY(K, X) \
	{K, MOD, changeWS, {.num = X}}, \
	{K, MOD|SHIFT, wToWS, {.num = X}}

static struct Key keyBinds[] = {
	//Key			//Modifiers		//Func			//Args
	//General
	{XK_e, 			MOD,			exit,			{NULL}},
	{XK_Return,		MOD, 			spawn,			{.str = alacritty}},
	{XK_d,			MOD,	 		spawn,			{.str = rofi}},
	{XK_t,			MOD,			toggle,			{NULL}},
	{XK_q,			MOD,			kill,			{NULL}},
	//Focus
	{XK_h,			MOD,			focChange,		{.dir = Left}},
	{XK_j,			MOD,			focChange,		{.dir = Down}},
	{XK_k,			MOD,			focChange,		{.dir = Up}},
	{XK_l,			MOD,			focChange,		{.dir = Right}},
	//Window moving
	{XK_h,			MOD|SHIFT,		wMove,			{.dir = Left}},
	{XK_j,			MOD|SHIFT,		wMove,			{.dir = Down}},
	{XK_k,			MOD|SHIFT,		wMove,			{.dir = Up}},
	{XK_l,			MOD|SHIFT,		wMove,			{.dir = Right}},
	//Workspaces
	WSKEY(XK_1, 1),
	WSKEY(XK_2, 2),
	WSKEY(XK_3, 3),
	WSKEY(XK_4, 4),
	WSKEY(XK_5, 5)
};
