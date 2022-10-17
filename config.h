#include <X11/keysym.h>
#include <X11/Xlib.h>

#include <vector>
#include <string>

//Startup
const std::string startup[] = {"picom -fD 3", "feh --bg-scale /usr/share/backgrounds/vapor_trails_blue.png", "~/.config/polybar/launch.sh", "emacs --daemon"};

//Main config
// Sensible gaps
const int gaps = 3;
const int outerGaps = 3;
// Huge gaps
/*
const int gaps = 20;
const int outerGaps = 30;
*/

const std::string logFile = "/tmp/yatlog.txt";

//WS config
const int numWS = 10;
const std::string workspaceNames[] = {"1: ", "2: 拾", "3: ", "4: ", "5: ", "6: ", "7: 拾", "8: ", "9: ", "10: "};
//If you have more then 2 monitors change the number below
const int maxMonitors = 2;
const int screenPreferences[][maxMonitors] = {{0}, {0}, {0}, {0}, {0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}};

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
	const unsigned int modifiers;
	const KeySym keysym;
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
KEYCOM(bashSpawn);
KEYCOM(screenTest);

// Super key mod
#define MOD Mod4Mask
// Alt key mod
//#define MOD Mod1Mask
#define SHIFT ShiftMask

// Programs to run for keybinds
const char* alacritty[] = {"alacritty", NULL};
const char* rofi[] = {"rofi", "-i", "-show", "drun", NULL};
const char* qutebrowser[] = {"qutebrowser", NULL};
const char* i3lock[] = {"i3lock", "-eti", "/usr/share/backgrounds/lockscreen.png", NULL};
const char* suspend[] = {"systemctl", "suspend", NULL};

// Script to run for keybinds
// Script I made to run an xrandr command
const char* monConfig[] = {"~/.yat_commands/monitor-config.sh"};


#define WSKEY(K, X) \
	{MOD, K, changeWS, {.num = X}}, \
	{MOD|SHIFT, K, wToWS, {.num = X}}

const Key keyBinds[] = {
	// Modifiers	//Key			//Func			//Args
	// General
	{MOD, 			XK_e,			exit,			{NULL}},
	{MOD,			XK_Return, 		spawn,			{.str = alacritty}},
	{MOD,			XK_d,	 		spawn,			{.str = rofi}},
	{MOD,			XK_t,			toggle,			{NULL}},
	{MOD,			XK_q,			kill,			{NULL}},
	{MOD,			XK_c,			spawn,			{.str = qutebrowser}},
	{MOD,			XK_x,			spawn,			{.str = i3lock}},
	{MOD|SHIFT,		XK_x,			spawn,			{.str = i3lock}},
	{MOD|SHIFT,		XK_x,			spawn,			{.str = suspend}},
	{MOD,			XK_m,			bashSpawn,		{.str = monConfig}},
	{MOD|SHIFT,		XK_t,			screenTest,		{NULL}},
	// Focus
	{MOD,			XK_h,			focChange,		{.dir = Left}},
	{MOD,			XK_j,			focChange,		{.dir = Down}},
	{MOD,			XK_k,			focChange,		{.dir = Up}},
	{MOD,			XK_l,			focChange,		{.dir = Right}},
	// Window moving
	{MOD|SHIFT,		XK_h,			wMove,			{.dir = Left}},
	{MOD|SHIFT,		XK_j,			wMove,			{.dir = Down}},
	{MOD|SHIFT,		XK_k,			wMove,			{.dir = Up}},
	{MOD|SHIFT,		XK_l,			wMove,			{.dir = Right}},
	// Workspaces
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
