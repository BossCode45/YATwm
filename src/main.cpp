#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <bits/getopt_core.h>
#include <libnotify/notification.h>
#include <libnotify/notify.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <sys/poll.h>
#include <sys/select.h>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <fcntl.h>
#include <poll.h>
#include <getopt.h>

#include "IPC.h"
#include "commands.h"
#include "keybinds.h"
#include "structs.h"
#include "config.h"
#include "util.h"
#include "ewmh.h"
#include "error.h"

using std::cout;
using std::string;
using std::endl;
using std::map;
using std::pair;
using std::vector;

std::ofstream yatlog;
std::time_t timeNow;
std::tm *now;
char nowString[80];

#define log(x)					\
	updateTime();				\
	yatlog << nowString << x << std::endl

Display* dpy;
Window root;

Globals globals = {dpy, root};

void updateMousePos();

CommandsModule commandsModule;
Config cfg(commandsModule);
KeybindsModule keybindsModule(commandsModule, cfg, globals, &updateMousePos);
IPCServerModule ipc(commandsModule, cfg, globals);

int sW, sH;
int bH;
TileDir nextDir = horizontal;

bool keepGoing = true;

map<int, Client> clients;
int currClientID = 0;
map<int, Frame> frames;
int currFrameID = 1;
map<Window, int> frameIDS;

ScreenInfo* screens;
int* focusedWorkspaces;
int focusedScreen = 0;
int nscreens;
int mX, mY;

#define getClient(c) clients.find(c)->second
#define getFrame(f) frames.find(f)->second
#define getFrameID(w) frameIDS.find(w)->second

Window bar;

int currWS = 1;


// Usefull functions
int FFCF(int sID);
void detectScreens();
void focusRoot(int root);
void handleConfigErrs(vector<Err> cfgErrs);
void updateTime();
void cWS(int newWS);
void focusWindow(Window w);

void configureRequest(XConfigureRequestEvent e);
void mapRequest(XMapRequestEvent e);
void destroyNotify(XDestroyWindowEvent e);
void enterNotify(XEnterWindowEvent e);
void clientMessage(XClientMessageEvent e);

static int OnXError(Display* display, XErrorEvent* e);

// Tiling
// Call this one to tile everything (it does all the fancy stuff trust me just call this one)
void tileRoots();
// Call this one to until everything (it handles multiple monitors)
void untileRoots();
// This is to be called by tileRoots, it takes in the x, y, w, and h of where it's allowed to tile windows to, and returns the ID of a fullscreen client if one is found, or noID (-1) if none are found
int tile(int frameID, int x, int y, int w, int h);
// This is to be called by tileRoots, it takes in a frameID and recursively unmaps all its children
void untile(int frameID);

// Usefull functions
int FFCF(int sID)
{
	if(frames.find(sID)->second.isClient)
		return sID;
	return FFCF(frames.find(sID)->second.subFrameIDs[0]);
}
void detectScreens()
{
	delete[] screens;
	delete[] focusedWorkspaces;
	log("Detecting screens: ");
	XRRMonitorInfo* monitors = XRRGetMonitors(dpy, root, true, &nscreens);
	log("\t" << nscreens << " monitors");
	screens = new ScreenInfo[nscreens];
	focusedWorkspaces = new int[nscreens];
	for(int i = 0; i < nscreens; i++)
	{
		focusedWorkspaces[i] = i * 5 + 1;
		char* name = XGetAtomName(dpy, monitors[i].name);
		screens[i] = {name, monitors[i].x, monitors[i].y, monitors[i].width, monitors[i].height}; 
		log("\tMonitor " << i + 1 << " - " << screens[i].name);
		log("\t\tx: " << screens[i].x << ", y: " << screens[i].y);
		log("\t\tw: " << screens[i].w << ", h: " << screens[i].h);
		XFree(name);
	}
	for(int i = 0; i < cfg.workspaces.size(); i++)
	{
		if(cfg.workspaces[i].screenPreferences[0] < nscreens && focusedWorkspaces[cfg.workspaces[i].screenPreferences[0]] == 0)
		{
			//focusedWorkspaces[screenPreferences[i][0]] = i+1;
		}
	}
	XFree(monitors);
}
void updateMousePos()
{
	Window rootRet, childRet;
	int rX, rY, cX, cY;
	unsigned int maskRet;
	XQueryPointer(dpy, root, &rootRet, &childRet, &rX, &rY, &cX, &cY, &maskRet);
	mX = rX;
	mY = rY;
}
int getClientChild(int fID)
{
	if(getFrame(fID).isClient)
		return fID;
	else
		return getClientChild(getFrame(fID).subFrameIDs[0]);
}
void focusRoot(int root)
{
	//log("Focusing root: " << root);
	if(getFrame(root).subFrameIDs.size() == 0)
	{
		//log("\tRoot has no children");
		focusWindow(root);
		return;
	}
	int client = getFrame(getClientChild(root)).cID;
	Window w = getClient(client).w;
	//log("\tFocusing window: " << w);
	focusWindow(w);
}
void handleConfigErrs(vector<Err> cfgErrs)
{
	for(Err cfgErr : cfgErrs)
	{
		if(cfgErr.code == CFG_ERR_FATAL)
		{
			log("YATwm fatal error (Code " << cfgErr.code << ")\n" << cfgErr.message);
			std::string title = "YATwm fatal config error (Code " + std::to_string(cfgErr.code) + ")";
			std::string body = cfgErr.message;
			NotifyNotification* n = notify_notification_new(title.c_str(),
									body.c_str(),
									0);
			notify_notification_set_timeout(n, 10000);
			if(!notify_notification_show(n, 0))
			{
				log("notification failed");
			}
		}
		else
		{
			log("YATwm non fatal error (Code " << cfgErr.code << ")\n" << cfgErr.message);
			std::string title = "YATwm non fatal config error (Code " + std::to_string(cfgErr.code) + ")";
			std::string body = "Check logs for more information";
			NotifyNotification* n = notify_notification_new(title.c_str(),
									body.c_str(),
									0);
			notify_notification_set_timeout(n, 10000);
			if(!notify_notification_show(n, 0))
			{
				log("notification failed");
			}
		}
	}
}
void updateTime()
{
	timeNow = std::time(0);
	now = std::localtime(&timeNow);
	strftime(nowString, sizeof(nowString), "[%H:%M:%S] ", now);
}
void cWS(int newWS)
{
	int prevWS = currWS;

	currWS = newWS;
	if(prevWS == currWS)
		return;
	untileRoots();

	//log("Changing WS with keybind");

	for(int i = 0; i < cfg.workspaces[newWS - 1].screenPreferencesc; i++)
	{
		if(nscreens > cfg.workspaces[newWS - 1].screenPreferences[i])
		{
			int screen = cfg.workspaces[newWS - 1].screenPreferences[i];
			//log("Found screen (screen " << screenPreferences[arg.num - 1][i] << ")");
			prevWS = focusedWorkspaces[screen];
			//log("Changed prevWS");
			focusedWorkspaces[screen] = newWS;
			//log("Changed focusedWorkspaces");
			if(focusedScreen != screen)
			{
				focusedScreen = screen;
				XWarpPointer(dpy, root, root, 0, 0, 0, 0, screens[screen].x + screens[screen].w/2, screens[screen].y + screens[screen].h/2);
			}
			//log("Changed focusedScreen");
			break;
		}
	}

	//log("Finished changes");

	//log(prevWS);
	// LOOK: what is this for?????
	if(prevWS < 1 || prevWS > cfg.workspaces.size())
	{
		//untile(prevWS);
	}
	//log("Untiled");
	//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	tileRoots();
	//log("Roots tiled");
	focusWindow(getFrame(currWS).rootData->focus);

	//EWMH
	setCurrentDesktop(currWS);
}
void focusWindow(Window w)
{
	XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
	getFrame(currWS).rootData->focus = w;
}

//Keybind commands
const void exit(const CommandArg* argv)
{
	keepGoing = false;
}
const void spawn(const CommandArg* argv)
{
	if(fork() == 0)
	{
		const std::string argsStr = argv[0].str;
		vector<std::string> args = split(argsStr, ' ');
		char** execvpArgs = new char*[args.size()];
		for(int i = 0; i < args.size(); i++)
		{
			execvpArgs[i] = strdup(args[i].c_str());
		}
		int null = open("/dev/null", O_WRONLY);
		dup2(null, 0);
		dup2(null, 1);
		dup2(null, 2);
		execvp(execvpArgs[0], execvpArgs);
		exit(0);
	}
}
const void spawnOnce(const CommandArg* argv)
{
	if(cfg.loaded)
		return;
	else spawn(argv);
}
const void toggle(const CommandArg* argv)
{
	nextDir = nextDir = (nextDir==horizontal)? vertical : horizontal;
}
const void kill(const CommandArg* argv)
{
	Window w;
	int revertToReturn;
	XGetInputFocus(dpy, &w, &revertToReturn);
	Atom* supported_protocols;
	int num_supported_protocols;
	if (XGetWMProtocols(dpy,
			    w,
			    &supported_protocols,
			    &num_supported_protocols) &&
	    (std::find(supported_protocols,
		       supported_protocols + num_supported_protocols,
		       XInternAtom(dpy, "WM_DELETE_WINDOW", false)) !=
	     supported_protocols + num_supported_protocols)) {
		// 1. Construct message.
		XEvent msg;
		memset(&msg, 0, sizeof(msg));
		msg.xclient.type = ClientMessage;
		msg.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", false);
		msg.xclient.window = w;
		msg.xclient.format = 32;
		msg.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
		// 2. Send message to window to be closed.
		cout << "Nice kill\n";
		XSendEvent(dpy, w, false, 0, &msg);
	} else {
		cout << "Mean kill\n";
		XKillClient(dpy, w);
	}
}
const void changeWS(const CommandArg* argv)
{
	cWS(argv[0].num);
}
const void wToWS(const CommandArg* argv)
{
	Window focusedWindow;
	int revertToReturn;
	XGetInputFocus(dpy, &focusedWindow, &revertToReturn);
	if(focusedWindow == root)
		return;

	int fID = frameIDS.find(focusedWindow)->second;
	//TODO: make floating windows move WS
	if(clients.find(frames.find(fID)->second.cID)->second.floating)
		return;
	vector<int>& pSF = frames.find(frames.find(fID)->second.pID)->second.subFrameIDs;
	for(int i = 0; i < pSF.size(); i++)
	{
		if(pSF[i] == fID)
		{
			//Frame disolve
			pSF.erase(pSF.begin() + i);
			int pID = frames.find(fID)->second.pID;
			if(pSF.size() < 2 && !frames.find(pID)->second.rootData)
			{
				//Erase parent frame
				int lastChildID = frames.find(frames.find(pID)->second.subFrameIDs[0])->second.ID;
				int parentParentID = frames.find(pID)->second.pID;
				vector<int>& parentParentSubFrameIDs = frames.find(parentParentID)->second.subFrameIDs;
				for(int j = 0; j < parentParentSubFrameIDs.size(); j++)
				{
					if(parentParentSubFrameIDs[j] == pID)
					{
						parentParentSubFrameIDs[j] = lastChildID;
						frames.find(lastChildID)->second.pID = parentParentID;
						frames.erase(pID);
						break;
					}
				}
			}

			break;
		}
	}
	frames.find(fID)->second.pID = argv[0].num;
	frames.find(argv[0].num)->second.subFrameIDs.push_back(fID);

	//EWMH
	setWindowDesktop(focusedWindow, argv[0].num);

	XUnmapWindow(dpy, focusedWindow);
	//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	untileRoots();
	tileRoots();
	focusWindow(root);
}
int dirFind(int fID, MoveDir dir)
{
	vector<int>& pSF = frames.find(frames.find(fID)->second.pID)->second.subFrameIDs;
	TileDir pDir = frames.find(frames.find(fID)->second.pID)->second.dir;

	int i = 0;
	for(int f : pSF)
	{
		if(f == fID)
		{
			break;
		}
		i++;
	}

	if(pDir == vertical)
	{
		switch(dir)
		{
		case UP: i--; break;
		case DOWN: i++; break;
		case LEFT: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
		case RIGHT: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
		}
	}
	else if(pDir == horizontal)
	{
		switch(dir)
		{
		case LEFT: i--; break;
		case RIGHT: i++; break;
		case UP: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
		case DOWN: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
		}
	}
	if(i < 0)
		i = pSF.size() - 1;
	if(i == pSF.size())
		i = 0;

	return pSF[i];
}
const void focChange(const CommandArg* argv)
{
	Window focusedWindow;
	int revertToReturn;
	XGetInputFocus(dpy, &focusedWindow, &revertToReturn);
	if(focusedWindow == root)
		return;

	int fID = frameIDS.find(focusedWindow)->second;
	int nID = dirFind(fID, argv[0].dir);
	int fNID = FFCF(nID);
	Window w = clients.find(frames.find(fNID)->second.cID)->second.w;
	focusWindow(w);
}
const void wMove(const CommandArg* argv)
{
	Window focusedWindow;
	int revertToReturn;
	XGetInputFocus(dpy, &focusedWindow, &revertToReturn);
	if(focusedWindow == root)
		return;

	int fID = frameIDS.find(focusedWindow)->second;
	if(clients.find(frames.find(fID)->second.cID)->second.floating)
		return;
	int nID = dirFind(fID, argv[0].dir);
	int fNID = FFCF(nID);
	int pID = frames.find(fNID)->second.pID;
	int oPID = frames.find(fID)->second.pID;

	vector<int>& pSF = frames.find(pID)->second.subFrameIDs;
	vector<int>& oPSF = frames.find(oPID)->second.subFrameIDs;

	for(int i = 0; i < frames.find(oPID)->second.subFrameIDs.size(); i++)
	{
		if(oPSF[i] != fID)
			continue;

		if(pID!=oPID)
		{
			//Frame dissolve
			oPSF.erase(oPSF.begin() + i);
			if(oPSF.size() < 2 && !frames.find(oPID)->second.rootData)
			{
				//Erase parent frame
				int lastChildID = frames.find(frames.find(oPID)->second.subFrameIDs[0])->second.ID;
				int parentParentID = frames.find(oPID)->second.pID;
				vector<int>& parentParentSubFrameIDs = frames.find(parentParentID)->second.subFrameIDs;
				for(int j = 0; j < parentParentSubFrameIDs.size(); j++)
				{
					if(parentParentSubFrameIDs[j] == oPID)
					{
						parentParentSubFrameIDs[j] = lastChildID;
						frames.find(lastChildID)->second.pID = parentParentID;
						frames.erase(oPID);
						break;
					}
				}
			}

			frames.find(fID)->second.pID = pID;
			pSF.push_back(fID);
		}
		else
		{
			if(frames.find(pID)->second.dir == vertical)
			{
				if(argv[0].dir == LEFT || argv[0].dir == RIGHT)
					return;
			}
			else
			{
				if(argv[0].dir == UP || argv[0].dir == DOWN)
					return;
			}
					
			int offset;
			if(argv[0].dir == UP || argv[0].dir == LEFT)
				offset = -1;
			else
				offset = 1;

			int swapPos = i + offset;
			
			if(swapPos == pSF.size())
				swapPos = 0;
			else if(swapPos == -1)
				swapPos = pSF.size() - 1;

			std::swap(pSF[i], pSF[swapPos]);
		}
		untileRoots();
		tileRoots();
		//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
		focusWindow(focusedWindow);
		return;
	}
	focusWindow(focusedWindow);
}
const void bashSpawn(const CommandArg* argv)
{
	if(fork() == 0)
	{
		int null = open("/dev/null", O_WRONLY);
		dup2(null, 0);
		dup2(null, 1);
		dup2(null, 2);
		system(argv[0].str);
		exit(0);
	}
}
const void bashSpawnOnce(const CommandArg* argv)
{
	if(cfg.loaded)
		return;
	else bashSpawn(argv);
}
const void reload(const CommandArg* argv)
{
	detectScreens();

	//Clear keybinds
	keybindsModule.clearKeybinds();

	//Load config again
	vector<Err> cfgErr = cfg.reloadFile();
	//Error check
	handleConfigErrs(cfgErr);

	//Re tile
	untileRoots();
	tileRoots();
}
const void wsDump(const CommandArg* argv)
{
	log("Workspace dump:");
	for(int i = 1; i < currFrameID; i++)
	{
		if(getFrame(i).isClient)
		{
			int id = i;
			while(!getFrame(id).rootData)
			{
				id=getFrame(id).pID;
			}
			log("\tClient with ID: " << getClient(getFrame(i).cID).w << ", on worskapce " << id);
		}
	}
}
const void nextMonitor(const CommandArg* argv)
{
	focusedScreen++;
	if(focusedScreen >= nscreens)
		focusedScreen = 0;

	XWarpPointer(dpy, root, root, 0, 0, 0, 0, screens[focusedScreen].x + screens[focusedScreen].w/2, screens[focusedScreen].y + screens[focusedScreen].h/2);
	focusRoot(focusedWorkspaces[focusedScreen]);
}
const void fullscreen(const CommandArg* arg)
{
	Window focusedWindow;
	int focusedRevert;
	XGetInputFocus(dpy, &focusedWindow, &focusedRevert);

	int fID = getFrameID(focusedWindow);
	int cID = getFrame(fID).cID;
	getClient(cID).fullscreen ^= true;
	tileRoots();
	setFullscreen(focusedWindow, getClient(cID).fullscreen); 
}

void configureRequest(XConfigureRequestEvent e)
{
	XWindowChanges changes;
	changes.x = e.x;
	changes.y = e.y;
	changes.width = e.width;
	changes.height = e.height;
	changes.border_width = e.border_width;
	changes.sibling = e.above;
	changes.stack_mode = e.detail;
	XConfigureWindow(dpy, e.window, (unsigned int) e.value_mask, &changes);
	log("Configure request: " << e.window);
	//focusWindow(e.window);
	//tileRoots();
}

void mapRequest(XMapRequestEvent e)
{
	XMapWindow(dpy, e.window);

	XTextProperty name;
	bool gotName = XGetWMName(dpy, e.window, &name);
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, e.window, &attr);
	if(gotName)
	{
		log("Mapping window: " << name.value);
	}
	else
	{
		log("Mapping window with unknown name (its probably mpv, mpv is annoying)");
	}
	log("\tWindow ID: " << e.window);

	Window focusedWindow;
	int revertToReturn;
	int pID;
	XGetInputFocus(dpy, &focusedWindow, &revertToReturn);
	if(focusedWindow && focusedWindow != root && frameIDS.count(focusedWindow)>0)
	{
		//Use focused to determine parent
		pID = frames.find(frameIDS.find(focusedWindow)->second)->second.pID;
	}
	else
	{
		Window rootRet, childRet;
		int rX, rY, cX, cY;
		unsigned int maskRet;
		XQueryPointer(dpy, root, &rootRet, &childRet, &rX, &rY, &cX, &cY, &maskRet);
		mX = rX;
		mY = rY;
		int monitor = 0;
		for(int i = 0; i < nscreens; i++)
		{
			if(screens[i].x <= mX && mX < screens[i].x + screens[i].w)
			{
				if(screens[i].y <= mY && mY < screens[i].y + screens[i].h)
				{
					monitor = i;
				}
			}
		}
		pID = focusedWorkspaces[monitor];
		focusedScreen = monitor;
		/*
		  if(mX == rX && mY == rY)
		  {
		  //Use focused screen
		  log("\tFocused screen is: " << focusedScreen);
		  }
		  else
		  {
		  //Use mouse
		  //TODO: Make this find the monitor
		  log("\tMouse is at x: " << rX << ", y: " << rY);
		  mX = rX;
		  mY = rY;
		  }
		*/
	}

	unsigned char* data;
	Atom type;
	int status = getProp(e.window, "_NET_WM_WINDOW_TYPE", &type, &data);
	if (status == Success && type != None && ((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", false))
	{
		log("\tWindow was bar");
		bH = attr.height;
		bar = e.window;
		XFree(data);
		return;
	}
	XFree(data);

	


	XSelectInput(dpy, e.window, EnterWindowMask);
	
	//Make client
	Client c = {currClientID, e.window, false, false};
	currClientID++;

	//Add to clients map
	clients.insert(pair<int, Client>(c.ID, c));

	//Make frame
	//pID = (frameIDS.count(focusedWindow)>0)? frames.find(frameIDS.find(focusedWindow)->second)->second.pID : currWS;
	vector<int> v;
	Frame f = {currFrameID, pID, true, c.ID, noDir, v, NULL};
	currFrameID++;


	//Add ID to frameIDS map
	frameIDS.insert(pair<Window, int>(e.window, f.ID));

	status = getProp(e.window, "_NET_WM_STATE", &type, &data);
	if(status == Success && type!=None && (((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_STATE_MODAL", false) || ((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_STATE_ABOVE", false)))
	{
		cout << "Floating" << endl;
		clients.find(c.ID)->second.floating = true;
		frames.find(pID)->second.rootData->floatingFrameIDs.push_back(f.ID);
		frames.insert(pair<int, Frame>(f.ID, f));
		setWindowDesktop(e.window, currWS);
		updateClientList(clients);
		XFree(data);
		//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
		tileRoots();
		return;
	}
	XFree(data);

	//Check how to add
	if(nextDir == frames.find(pID)->second.dir || frameIDS.count(focusedWindow)==0)
	{
		//Add to focused parent
		frames.find(pID)->second.subFrameIDs.push_back(f.ID);
	}
	else
	{
		//Get parent sub frames for later use
		vector<int>& pS = frames.find(pID)->second.subFrameIDs;

		//Get index of focused frame in parent sub frames
		int index;
		for(index = 0; index < pS.size(); index++)
		{
			if(pS[index] == frames.find(frameIDS.find(focusedWindow)->second)->second.ID)
				break;
		}

		//Make new frame
		vector<int> v;
		v.push_back(frames.find(frameIDS.find(focusedWindow)->second)->second.ID);
		v.push_back(f.ID);
		Frame pF = {currFrameID, pID, false, noID, nextDir, v, NULL};

		//Update the IDS
		f.pID = currFrameID;
		frames.find(frames.find(frameIDS.find(focusedWindow)->second)->second.ID)->second.pID = currFrameID;
		pS[index] = currFrameID;

		currFrameID++;

		//Insert the new frame into the frames map
		frames.insert(pair<int, Frame>(pF.ID, pF));
	}
	
	//Add to frames map
	frames.insert(pair<int, Frame>(f.ID, f));

	setWindowDesktop(e.window, currWS);
	updateClientList(clients);

	//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	focusWindow(e.window);
	tileRoots();
}

void destroyNotify(XDestroyWindowEvent e)
{
	if(frameIDS.count(e.window)<1)
		return;
	log("Destroy notif");
	log("\tWindow ID: " << e.window);
	int fID = frameIDS.find(e.window)->second;
	int pID = frames.find(fID)->second.pID;
	vector<int>& pS = frames.find(pID)->second.subFrameIDs;
	if(clients.find(frames.find(fID)->second.cID)->second.floating)
	{
		pS = frames.find(pID)->second.rootData->floatingFrameIDs;
	}
	for(int i = 0; i < pS.size(); i++)
	{
		if(frames.find(pS[i])->second.ID == fID)
		{
			pS.erase(pS.begin() + i);
			clients.erase(frames.find(fID)->second.cID);
			frames.erase(fID);
			frameIDS.erase(e.window);

			if(pS.size() < 2 && !frames.find(pID)->second.rootData)
			{
				//Erase parent frame
				int lastChildID = frames.find(frames.find(pID)->second.subFrameIDs[0])->second.ID;
				int parentParentID = frames.find(pID)->second.pID;
				vector<int>& parentParentSubFrameIDs = frames.find(parentParentID)->second.subFrameIDs;
				for(int j = 0; j < parentParentSubFrameIDs.size(); j++)
				{
					if(parentParentSubFrameIDs[j] == pID)
					{
						parentParentSubFrameIDs[j] = lastChildID;
						frames.find(lastChildID)->second.pID = parentParentID;
						frames.erase(pID);
						break;
					}
				}
			}
			break;
		}
	}
	focusWindow(root);
	//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	tileRoots();

	updateClientList(clients);
}
void enterNotify(XEnterWindowEvent e)
{
	//log(e.xcrossing.x);
	//Cancel if crossing into root
	if(e.window == root)
		return;
	
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, e.window, &attr);
	int monitor = 0;
	for(int i = 0; i < nscreens; i++)
	{
		if(screens[i].x <= attr.x && attr.x < screens[i].x + screens[i].w)
		{
			if(screens[i].y <= attr.y && attr.y < screens[i].y + screens[i].h)
			{
				monitor = i;
			}
		}
	}
	focusedScreen = monitor;
	focusWindow(e.window);
}
void clientMessage(XClientMessageEvent e)
{
	char* name = XGetAtomName(dpy, e.message_type);
	log("Client message: " << name);
	if(e.message_type == XInternAtom(dpy, "_NET_CURRENT_DESKTOP", false))
	{
		cWS(e.data.l[0] + 1);
		/*
		//Change desktop
		int nextWS = (long)e.data.l[0] + 1;
		int prevWS = currWS;
		currWS = nextWS;

		if(prevWS == currWS)
		return;

		untile(prevWS);
		tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
		focusWindow(root);

		//EWMH
		setCurrentDesktop(currWS);
		*/
	}
	else if(e.message_type == XInternAtom(dpy, "_NET_WM_STATE", false))
	{
		if((Atom)e.data.l[0] == 0)
			log("\tremove");
		if((Atom)e.data.l[0] == 1)
			log("\ttoggle");
		if((Atom)e.data.l[0] == 2)
			log("\tadd");
		char* prop1 = XGetAtomName(dpy, (Atom)e.data.l[1]);
		if((Atom)e.data.l[1] == XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", false))
		{
			int fID = getFrameID(e.window);
			int cID = getFrame(fID).cID;
			getClient(cID).fullscreen = (Atom) e.data.l[0] > 0;
			setFullscreen(e.window, (Atom) e.data.l[0] > 0); 
			tileRoots();
		}
		XFree(prop1);
	}
	XFree(name);
}

static int OnXError(Display* display, XErrorEvent* e)
{
	char* error = new char[50];
	XGetErrorText(dpy, e->type, error, 50);
	log("XError " << error);
	delete[] error;
	return 0;
}

void tileRoots()
{
	for(int i = 0; i < nscreens; i++)
	{
		int fullscreenClientID = tile(focusedWorkspaces[i], screens[i].x + cfg.outerGaps, screens[i].y + cfg.outerGaps, screens[i].w - cfg.outerGaps*2, screens[i].h - cfg.outerGaps*2 - bH);
		if(fullscreenClientID!=noID)
		{
			untile(focusedWorkspaces[i]);
			Client c = getClient(fullscreenClientID);
			XMapWindow(dpy, c.w);
			XMoveWindow(dpy, c.w,
				    screens[i].x, screens[i].y);
			XResizeWindow(dpy, c.w,
				      screens[i].w, screens[i].h);
		}
	}
}
void untileRoots()
{
	for(int i = 0; i < nscreens; i++)
	{
		untile(focusedWorkspaces[i]);
	}
}
int tile(int frameID, int x, int y, int w, int h)
{
	if(getFrame(frameID).rootData)
	{
		for(int fID : frames.find(frameID)->second.rootData->floatingFrameIDs)
		{
			Window w = clients.find(frames.find(fID)->second.cID)->second.w;
			XMapWindow(dpy, w);
		}
	}
	TileDir dir = frames.find(frameID)->second.dir;
	int i = 0;
	vector<int>& subFrameIDs = frames.find(frameID)->second.subFrameIDs;
	for(int fID : subFrameIDs)
	{
		Frame f = frames.find(fID)->second;
		int wX = (dir==horizontal) ? x + i * (w/subFrameIDs.size()) : x;
		int wY = (dir==vertical) ?  y + i * (h/subFrameIDs.size()) : y;
		int wW = (dir==horizontal) ? w/subFrameIDs.size() : w;
		int wH = (dir==vertical) ? h/subFrameIDs.size() : h;
		i++;
		if(i==subFrameIDs.size())
		{
			wW = (dir==horizontal) ? w - (wX - x) : w;
			wH = (dir==vertical) ? h - (wY - y) : h;
		}
		if(!f.isClient)
		{
			int fullscreenClientID = tile(fID, wX, wY, wW, wH);
			if(fullscreenClientID == noID)
				return fullscreenClientID;
			continue;
		}
		Client c = clients.find(f.cID)->second;
		if(c.fullscreen)
			return c.ID;
		wX += cfg.gaps;
		wY += cfg.gaps;
		wW -= cfg.gaps * 2;
		wH -= cfg.gaps * 2;
		XMapWindow(dpy, c.w);
		XMoveWindow(dpy, c.w,
			    wX, wY);
		XResizeWindow(dpy, c.w,
			      wW, wH);
	}
	return noID;
}

void untile(int frameID)
{
	if(getFrame(frameID).rootData)
	{
		for(int fID : frames.find(frameID)->second.rootData->floatingFrameIDs)
		{
			Window w = clients.find(frames.find(fID)->second.cID)->second.w;
			XUnmapWindow(dpy, w);
		}
	}
	vector<int>& subFrameIDs = frames.find(frameID)->second.subFrameIDs;
	TileDir dir = frames.find(frameID)->second.dir;
	for(int fID : subFrameIDs)
	{
		Frame f = frames.find(fID)->second;
		if(!f.isClient)
		{
			untile(fID);
			continue;
		}
		Client c = clients.find(f.cID)->second;
		XUnmapWindow(dpy, c.w);
	}
}

void printVersion()
{
	const char* version =
	"YATwm for X\n"
	"version 0.0.1";
	cout << version << endl;
}

int main(int argc, char** argv)
{
	int versionFlag = 0;
	bool immediateExit = false;
	while(1)
	{
		static option long_options[] = {{"version", no_argument, &versionFlag, 1},
										{0, 0, 0, 0}};
		
		int optionIndex;
		char c = getopt_long(argc, argv, "v", long_options, &optionIndex);

		if(c == -1)
			break;
		
		switch(c)
		{
		case 0:
			if(long_options[optionIndex].flag != 0)
				break;
			//Option had arg
			break;
		case 'v':
			versionFlag = 1;
		case '?':
			//Error??
			break;
		default:
			//Big error???
			cout << "BIG ERROR WITH OPTIONS" << endl;
		}
	}

	if(versionFlag == 1)
	{
		printVersion();
		immediateExit = true;
	}

	if(optind < argc)
	{
		//Extra options - probably meant to be a command sent on IPC
		std::string message;
		while(optind < argc)
		{
			message += argv[optind++];
			if(optind != argc)
				message += " ";
		}

		cout << message << endl;

		IPCClientModule IPCClient;
		switch(IPCClient.init())
		{
		case 1:
			cout << "X error: is YATwm running?" << endl;
			exit(1);
		case -1:
			cout << "Socket error" << endl;
			exit(1);
		}
		IPCClient.sendMessage(message.c_str(), message.length());
		char buff[256];
		IPCClient.getMessage(buff, 256);
		cout << buff << endl;
		IPCClient.quit();
		
		immediateExit = true;
	}

	if(immediateExit)
		exit(0);
			
	//Important init stuff
	mX = mY = 0;
	dpy = XOpenDisplay(nullptr);
	root = Window(DefaultRootWindow(dpy));
	// Adding commands
	commandsModule.addCommand("exit", exit, 0, {});
	commandsModule.addCommand("spawn", spawn, 1, {STR_REST});
	commandsModule.addCommand("spawnOnce", spawnOnce, 1, {STR_REST});
	commandsModule.addCommand("toggle", toggle, 0, {});
	commandsModule.addCommand("kill", kill, 0, {});
	commandsModule.addCommand("changeWS", changeWS, 1, {NUM});
	commandsModule.addCommand("wToWS", wToWS, 1, {NUM});
	commandsModule.addCommand("focChange", focChange, 1, {MOVDIR});
	commandsModule.addCommand("bashSpawn", bashSpawn, 1, {STR_REST});
	commandsModule.addCommand("bashSpawnOnce", bashSpawnOnce, 1, {STR_REST});
	commandsModule.addCommand("reload", reload, 0, {});
	commandsModule.addCommand("wsDump", wsDump, 0, {});
	commandsModule.addCommand("nextMonitor", nextMonitor, 0, {});
	commandsModule.addCommand("fullscreen", fullscreen, 0, {});

	//Config
	std::vector<Err> cfgErr;

	cout << "Registered commands" << endl;
	
	char* confDir = getenv("XDG_CONFIG_HOME");
	if(confDir != NULL)
	{
		cfgErr = cfg.loadFromFile(string(confDir) + "/YATwm/config");
	}
	else
	{
		string home = getenv("HOME");
		cfgErr = cfg.loadFromFile(home + "/.config/YATwm/config");
	}

	cout << "Done config" << endl;
	
	//Log
	yatlog.open(cfg.logFile, std::ios_base::app);
	yatlog << "\n" << endl;

	//Print starting message
	log("-------- YATWM STARTING --------");

	//Notifications
	notify_init("YATwm");

	//Error check config
	handleConfigErrs(cfgErr);

	screens = new ScreenInfo[1];
	focusedWorkspaces = new int[1];
	detectScreens();

	int screenNum = DefaultScreen(dpy);
	sW = DisplayWidth(dpy, screenNum);
	sH = DisplayHeight(dpy, screenNum);

	//XSetErrorHandler(OnXError);
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | EnterWindowMask);

	XDefineCursor(dpy, root, XCreateFontCursor(dpy, XC_top_left_arrow));
	//EWMH
	initEWMH(&dpy, &root, cfg.workspaces.size(), cfg.workspaces);
	setCurrentDesktop(1);

	ipc.init();

	for(int i = 1; i < cfg.numWS + 1; i++)
	{
		vector<int> v;
		RootData* rootData = new RootData;
		rootData->focus = root;
		Frame rootFrame = {i, noID, false, noID, horizontal, v, rootData};
		frames.insert(pair<int, Frame>(i, rootFrame));
		currFrameID++;
	}

	focusWindow(root);
	XWarpPointer(dpy, root, root, 0, 0, 0, 0, 960, 540);

	fd_set fdset;
	int x11fd = ConnectionNumber(dpy);
	FD_ZERO(&fdset);
	FD_SET(x11fd, &fdset);
	FD_SET(ipc.getFD(), &fdset);
	
	log("Begin mainloop");
	while(keepGoing)
	{
		FD_ZERO(&fdset);
		FD_SET(x11fd, &fdset);
		FD_SET(ipc.getFD(), &fdset);
		int ready = select(std::max(x11fd, ipc.getFD()) + 1, &fdset, NULL, NULL, NULL);
		if(FD_ISSET(ipc.getFD(), &fdset))
		{
			ipc.doListen();
		}
		if(FD_ISSET(x11fd, &fdset))
		{
			XEvent e;
			while(XPending(dpy))
			{
				XNextEvent(dpy, &e);

				switch(e.type)
				{
				case KeyPress:
					keybindsModule.handleKeypress(e.xkey);
					break;
				case ConfigureRequest:
					configureRequest(e.xconfigurerequest);
					break;
				case MapRequest:
					mapRequest(e.xmaprequest);
					break;
				case DestroyNotify:
					destroyNotify(e.xdestroywindow);
					break;
				case EnterNotify:
					enterNotify(e.xcrossing);
					break;
				case ClientMessage:
					clientMessage(e.xclient);
					break;
				default:
					// cout << "Unhandled event: " << getEventName(e.type) << endl;
					break;
				}
			}
		}
		if(ready == -1)
		{
			cout << "E" << endl;
			log("ERROR");
		}
	}

	//Kill children
	ipc.quitIPC();
	XCloseDisplay(dpy);
}
