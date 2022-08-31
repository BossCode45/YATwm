#include <X11/keysym.h>
#include <X11/Xlib.h>

#include <vector>
#include <string>

//Startup
std::string startup[] = {"picom -fD 3", "feh --bg-scale /usr/share/backgrounds/vapor_trails_blue.png", "~/.config/polybar/launch.sh"};

//Main config
int gaps = 10;
int outerGaps = 30;

//WS config
const int numWS = 10;
std::string workspaceNames[] = {"1: ", "2: 拾", "3: ", "4: ", "5: ", "6: ", "7: 拾", "8: ", "9: ", "10: "};

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
const char* qutebrowser[] = {"qutebrowser", NULL};

const char* i3lock[] = {"i3lock", "-eti", "/usr/share/backgrounds/lockscreen.png", NULL};
const char* suspend[] = {"systemctl", "suspend", NULL};

#define MOD Mod4Mask
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
	{XK_c,			MOD,			spawn,			{.str = qutebrowser}},
	{XK_x,			MOD,			spawn,			{.str = i3lock}},
	{XK_x,			MOD|SHIFT,		spawn,			{.str = i3lock}},
	{XK_x,			MOD|SHIFT,		spawn,			{.str = suspend}},
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
	WSKEY(XK_5, 5),
	WSKEY(XK_6, 6),
	WSKEY(XK_7, 7),
	WSKEY(XK_8, 8),
	WSKEY(XK_9, 9),
	WSKEY(XK_0, 10),
};
