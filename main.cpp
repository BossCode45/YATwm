#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <libnotify/notification.h>
#include <toml++/toml.hpp>

#include <libnotify/notify.h>

#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
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
#include <vector>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <fcntl.h>

#include "structs.h"
#include "config.h"
#include "util.h"
#include "ewmh.h"

using std::cout;
using std::string;
using std::endl;
using std::map;
using std::pair;
using std::vector;

std::ofstream yatlog;

#define log(x) yatlog << x << std::endl

Config cfg;

Display* dpy;
Window root;
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

Window bar;

int currWS = 1;


// Usefull functions
int FFCF(int sID);
void detectScreens();
void updateMousePos();
void focusRoot(int root);
void handleConfigErrs(Err cfgErr);

void keyPress(XKeyEvent e);
void configureRequest(XConfigureRequestEvent e);
void mapRequest(XMapRequestEvent e);
void destroyNotify(XDestroyWindowEvent e);
void enterNotify(XEnterWindowEvent e);
void clientMessage(XClientMessageEvent e);

static int OnXError(Display* display, XErrorEvent* e);

void tileRoots();
void untileRoots();
void tile(int frameID, int x, int y, int w, int h);
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
	for(int i = 0; i < cfg.numWS; i++)
	{
		if(cfg.screenPreferences[i][0] < nscreens && focusedWorkspaces[cfg.screenPreferences[i][0]] == 0)
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
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		return;
	}
	int client = getFrame(getClientChild(root)).cID;
	Window w = getClient(client).w;
	//log("\tFocusing window: " << w);
	XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
}
void handleConfigErrs(Err cfgErr)
{
	if(cfgErr.code!=NOERR)
	{
		if(cfgErr.code == ERR_CFG_FATAL)
		{
			log("YATwm fatal error (Code " << cfgErr.code << ")\n" << cfgErr.errorMessage);
			std::string title = "YATwm fatal config error (Code " + std::to_string(cfgErr.code) + ")";
			std::string body = cfgErr.errorMessage;
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
			log("YATwm non fatal error (Code " << cfgErr.code << ")\n" << cfgErr.errorMessage);
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

//Keybind commands
void exit(const KeyArg arg)
{
	keepGoing = false;
}
void spawn(const KeyArg arg)
{
	if(fork() == 0)
	{
		int null = open("/dev/null", O_WRONLY);
		dup2(null, 0);
		dup2(null, 1);
		dup2(null, 2);
		const std::string argsStr = arg.str;
		vector<std::string> args = split(argsStr, ' ');
		char** execvpArgs = new char*[args.size()];
		for(int i = 0; i < args.size(); i++)
		{
			execvpArgs[i] = strdup(args[i].c_str());
		}
		execvp(execvpArgs[0], execvpArgs);
		exit(0);
	}
}
void toggle(const KeyArg arg)
{
	nextDir = nextDir = (nextDir==horizontal)? vertical : horizontal;
}
void kill(const KeyArg arg)
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
void changeWS(const KeyArg arg)
{
	int prevWS = currWS;

	currWS = arg.num;
	if(prevWS == currWS)
		return;
	untileRoots();

	//log("Changing WS with keybind");

	for(int i = 0; i < cfg.maxMonitors; i++)
	{
		if(nscreens > cfg.screenPreferences[arg.num - 1][i])
		{
			int screen = cfg.screenPreferences[arg.num - 1][i];
			//log("Found screen (screen " << screenPreferences[arg.num - 1][i] << ")");
			prevWS = focusedWorkspaces[cfg.screenPreferences[arg.num - 1][i]];
			//log("Changed prevWS");
			focusedWorkspaces[cfg.screenPreferences[arg.num - 1][i]] = arg.num;
			//log("Changed focusedWorkspaces");
			if(focusedScreen != cfg.screenPreferences[arg.num - 1][i])
			{
				focusedScreen = cfg.screenPreferences[arg.num - 1][i];
				XWarpPointer(dpy, root, root, 0, 0, 0, 0, screens[screen].x + screens[screen].w/2, screens[screen].y + screens[screen].h/2);
			}
			//log("Changed focusedScreen");
			break;
		}
	}

	//log("Finished changes");

	//log(prevWS);
	if(prevWS < 1 || prevWS > cfg.numWS)
	{
		//untile(prevWS);
	}
	//log("Untiled");
	//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	tileRoots();
	//log("Roots tiled");
	XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);

	//EWMH
	setCurrentDesktop(currWS);
}
void wToWS(const KeyArg arg)
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
			if(pSF.size() < 2 && !frames.find(pID)->second.isRoot)
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
	frames.find(fID)->second.pID = arg.num;
	frames.find(arg.num)->second.subFrameIDs.push_back(fID);

	//EWMH
	setWindowDesktop(focusedWindow, arg.num);

	XUnmapWindow(dpy, focusedWindow);
	//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	untileRoots();
	tileRoots();
	XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
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
			case Up: i--; break;
			case Down: i++; break;
			case Left: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
			case Right: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
		}
	}
	else if(pDir == horizontal)
	{
		switch(dir)
		{
			case Left: i--; break;
			case Right: i++; break;
			case Up: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
			case Down: return (frames.find(fID)->second.pID > cfg.numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
		}
	}
	if(i < 0)
		i = pSF.size() - 1;
	if(i == pSF.size())
		i = 0;

	return pSF[i];
}
void focChange(const KeyArg arg)
{
	Window focusedWindow;
	int revertToReturn;
	XGetInputFocus(dpy, &focusedWindow, &revertToReturn);
	if(focusedWindow == root)
		return;

	int fID = frameIDS.find(focusedWindow)->second;
	int nID = dirFind(fID, arg.dir);
	int fNID = FFCF(nID);
	Window w = clients.find(frames.find(fNID)->second.cID)->second.w;
	XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
}
void wMove(const KeyArg arg)
{
	Window focusedWindow;
	int revertToReturn;
	XGetInputFocus(dpy, &focusedWindow, &revertToReturn);
	if(focusedWindow == root)
		return;

	int fID = frameIDS.find(focusedWindow)->second;
	if(clients.find(frames.find(fID)->second.cID)->second.floating)
		return;
	int nID = dirFind(fID, arg.dir);
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
			if(oPSF.size() < 2 && !frames.find(oPID)->second.isRoot)
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
				if(arg.dir == Left || arg.dir == Right)
					return;
			}
			else
			{
				if(arg.dir == Up || arg.dir == Down)
					return;
			}
					
			int offset;
			if(arg.dir == Up || arg.dir == Left)
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
		XSetInputFocus(dpy, focusedWindow, RevertToPointerRoot, CurrentTime);
		return;
	}
	XSetInputFocus(dpy, focusedWindow, RevertToPointerRoot, CurrentTime);
}
void bashSpawn(const KeyArg arg)
{
	if(fork() == 0)
	{
		int null = open("/dev/null", O_WRONLY);
		dup2(null, 0);
		dup2(null, 1);
		dup2(null, 2);
		system(arg.str);
		exit(0);
	}

}
void reload(const KeyArg arg)
{
	detectScreens();

	//Load config again
	Err cfgErr = cfg.reload();
	//Error check
	handleConfigErrs(cfgErr);

	//Re tile
	tileRoots();
}
void wsDump(const KeyArg arg)
{
	log("Workspace dump:");
	for(int i = 1; i < currFrameID; i++)
	{
		if(getFrame(i).isClient)
		{
			int id = i;
			while(!getFrame(id).isRoot)
			{
				id=getFrame(id).pID;
			}
			log("\tClient with ID: " << getClient(getFrame(i).cID).w << ", on worskapce " << id);
		}
	}
}
void nextMonitor(const KeyArg arg)
{
	focusedScreen++;
	if(focusedScreen >= nscreens)
		focusedScreen = 0;

	XWarpPointer(dpy, root, root, 0, 0, 0, 0, screens[focusedScreen].x + screens[focusedScreen].w/2, screens[focusedScreen].y + screens[focusedScreen].h/2);
	focusRoot(focusedWorkspaces[focusedScreen]);
}

void keyPress(XKeyEvent e)
{
	if(e.same_screen!=1) return;
	updateMousePos();
	//cout << "Keypress recieved\n";
	KeySym keysym = XLookupKeysym(&e, 0);
	//cout << "\t" << XKeysymToString(keysym) << " super: " << ((e.state & Mod4Mask) == Mod4Mask) << " alt: " << ((e.state & Mod1Mask) == Mod1Mask) << " shift: " << ((e.state & ShiftMask) == ShiftMask) << std::endl;
	for(int i = 0; i < cfg.bindsc; i++)
	{
		if(cfg.binds[i].keysym == keysym && (e.state & cfg.binds[i].modifiers) == cfg.binds[i].modifiers)
		{
			cfg.binds[i].func(cfg.binds[i].args);
		}
	}
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
	XConfigureWindow(dpy, e.window, e.value_mask, &changes);
}

void mapRequest(XMapRequestEvent e)
{
	XMapWindow(dpy, e.window);

	XTextProperty name;
	bool gotName = XGetWMName(dpy, e.window, &name);
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, e.window, &attr);
	if(gotName)
		log("Mapping window: " << name.value);
	else
		log("Mapping window with unknown name (its probably mpv, mpv is annoying)");
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

	

	XSetInputFocus(dpy, e.window, RevertToNone, CurrentTime);
	XSelectInput(dpy, e.window, EnterWindowMask);
	
	//Make client
	Client c = {currClientID, e.window, false};
	currClientID++;

	//Add to clients map
	clients.insert(pair<int, Client>(c.ID, c));

	//Make frame
	//pID = (frameIDS.count(focusedWindow)>0)? frames.find(frameIDS.find(focusedWindow)->second)->second.pID : currWS;
	vector<int> v;
	vector<int> floating;
	Frame f = {currFrameID, pID, true, c.ID, noDir, v, false, floating};
	currFrameID++;


	//Add ID to frameIDS map
	frameIDS.insert(pair<Window, int>(e.window, f.ID));

	status = getProp(e.window, "_NET_WM_STATE", &type, &data);
	if(status == Success && type!=None && (((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_STATE_MODAL", false) || ((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_STATE_ABOVE", false)))
	{
		log("\tWindow floating");
		clients.find(c.ID)->second.floating = true;
		frames.find(pID)->second.floatingFrameIDs.push_back(f.ID);
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
		Frame pF = {currFrameID, pID, false, noID, nextDir, v, false, floating};

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
		pS = frames.find(pID)->second.floatingFrameIDs;
	}
	for(int i = 0; i < pS.size(); i++)
	{
		if(frames.find(pS[i])->second.ID == fID)
		{
			pS.erase(pS.begin() + i);
			clients.erase(frames.find(fID)->second.cID);
			frames.erase(fID);
			frameIDS.erase(e.window);

			if(pS.size() < 2 && !frames.find(pID)->second.isRoot)
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
	XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	//tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	tileRoots();

	updateClientList(clients);
}
void enterNotify(XEnterWindowEvent e)
{
	//log(e.xcrossing.x);
	/* Cancel if crossing into root
	if(e.xcrossing.window == root)
		break;
	*/
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
	XSetInputFocus(dpy, e.window, RevertToNone, CurrentTime);
}
void clientMessage(XClientMessageEvent e)
{
	char* name = XGetAtomName(dpy, e.message_type);
	log("Client message: " << name);
	if(e.message_type == XInternAtom(dpy, "_NET_CURRENT_DESKTOP", false))
	{
		changeWS({.num = (int)((long)e.data.l[0] + 1)});
		/*
		//Change desktop
		int nextWS = (long)e.data.l[0] + 1;
		int prevWS = currWS;
		currWS = nextWS;

		if(prevWS == currWS)
			return;

		untile(prevWS);
		tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);

		//EWMH
		setCurrentDesktop(currWS);
		*/
	}
	XFree(name);
}

static int OnXError(Display* display, XErrorEvent* e)
{
	log("XError " << e->type);
	return 0;
}

void tileRoots()
{
	for(int i = 0; i < nscreens; i++)
	{
		tile(focusedWorkspaces[i], screens[i].x + cfg.outerGaps, screens[i].y + cfg.outerGaps, screens[i].w - cfg.outerGaps*2, screens[i].h - cfg.outerGaps*2 - bH);
	}
}
void untileRoots()
{
	for(int i = 0; i < nscreens; i++)
	{
		untile(focusedWorkspaces[i]);
	}
}
void tile(int frameID, int x, int y, int w, int h)
{
	for(int fID : frames.find(frameID)->second.floatingFrameIDs)
	{
		Window w = clients.find(frames.find(fID)->second.cID)->second.w;
		XMapWindow(dpy, w);
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
			tile(fID, wX, wY, wW, wH);
			continue;
		}
		wX += cfg.gaps;
		wY += cfg.gaps;
		wW -= cfg.gaps * 2;
		wH -= cfg.gaps * 2;
		Client c = clients.find(f.cID)->second;
		XMapWindow(dpy, c.w);
		XMoveWindow(dpy, c.w,
					wX, wY);
		XResizeWindow(dpy, c.w,
					wW, wH);
	}
}

void untile(int frameID)
{
	for(int fID : frames.find(frameID)->second.floatingFrameIDs)
	{
		Window w = clients.find(frames.find(fID)->second.cID)->second.w;
		XUnmapWindow(dpy, w);
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

int main(int argc, char** argv)
{
	if(argc > 1)
	{
		if(strcmp(argv[1], "--version") == 0)
		{
			const char* version =
				"YATwm for X\n"
				"version 0.0.0";
			cout << version << endl;
			return 0;
		}
	}
	//Important init stuff
	mX = mY = 0;
	dpy = XOpenDisplay(nullptr);
	root = Window(DefaultRootWindow(dpy));


	//Config
	std::string home = getenv("HOME");
	std::string pathAfterHome = "/.config/YATwm/config.toml";
	std::string file = home + pathAfterHome;
	Err cfgErr = cfg.loadFromFile(file);

	//Log
	yatlog.open(cfg.logFile, std::ios_base::app);

	//Print starting message
	auto timeUnformatted = std::chrono::system_clock::now();
	std::time_t time = std::chrono::system_clock::to_time_t(timeUnformatted);
	log("\nYAT STARTING: " << std::ctime(&time) << "--------------------------------------");

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

	XSetErrorHandler(OnXError);
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | EnterWindowMask);

	for(int i = 0; i < cfg.bindsc; i++)
	{
		XGrabKey(dpy, XKeysymToKeycode(dpy, cfg.binds[i].keysym), cfg.binds[i].modifiers, root, false, GrabModeAsync, GrabModeAsync);
		//log("Grabbing " << XKeysymToString(cfg.binds[i].keysym));
	}
	XDefineCursor(dpy, root, XCreateFontCursor(dpy, XC_top_left_arrow));
	//EWMH
	initEWMH(&dpy, &root, cfg.numWS,cfg. workspaceNames);
	setCurrentDesktop(1);

	for(int i = 1; i < cfg.numWS + 1; i++)
	{
		vector<int> v;
		Frame rootFrame = {i, noID, false, noID, horizontal, v, true};
		frames.insert(pair<int, Frame>(i, rootFrame));
		currFrameID++;
	}
	for(int i = 0; i < cfg.startupBashc; i++)
	{
		if(fork() == 0)
		{
			system((cfg.startupBash[i] + " > /dev/null 2> /dev/null").c_str());
			exit(0);
		}
	}

	XSetInputFocus(dpy, root, RevertToNone, CurrentTime);
	XWarpPointer(dpy, root, root, 0, 0, 0, 0, 960, 540);

	log("Begin mainloop");
	while(keepGoing)
	{
		XEvent e;
		XNextEvent(dpy, &e);

		switch(e.type)
		{
			case KeyPress:
				keyPress(e.xkey);
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
				//cout << "Unhandled event, code: " << evNames[e.type] << "!\n";
				break;
		}
	}

	//Kill children

	XCloseDisplay(dpy);
}
