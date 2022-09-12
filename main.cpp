//DEV BRANCH
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/Xutil.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <fcntl.h>

#include "structs.h"
#include "util.h"
#include "ewmh.h"

#include "config.h"

using std::cout;
using std::map;
using std::pair;
using std::vector;

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

Window bar;

int currWS = 1;

void keyPress(XKeyEvent e);
void configureRequest(XConfigureRequestEvent e);
void mapRequest(XMapRequestEvent e);
void destroyNotify(XDestroyWindowEvent e);
void clientMessage(XClientMessageEvent e);

static int OnXError(Display* display, XErrorEvent* e);

void tile(int frameID, int x, int y, int w, int h);
void untile(int frameID);

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
		dup2(null, 1);
		dup2(null, 2);
		execvp((char*)arg.str[0], (char**)arg.str);
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

	untile(prevWS);
	tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
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
	tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
	XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
}
int FFCF(int sID)
{
	if(frames.find(sID)->second.isClient)
		return sID;
	return FFCF(frames.find(sID)->second.subFrameIDs[0]);
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
			case Left: return (frames.find(fID)->second.pID > numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
			case Right: return (frames.find(fID)->second.pID > numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
		}
	}
	else if(pDir == horizontal)
	{
		switch(dir)
		{
			case Left: i--; break;
			case Right: i++; break;
			case Up: return (frames.find(fID)->second.pID > numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
			case Down: return (frames.find(fID)->second.pID > numWS)? dirFind(frames.find(fID)->second.pID, dir) : fID;
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
			cout << "Moving to: " << pID << "\n";
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
		tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
		XSetInputFocus(dpy, focusedWindow, RevertToPointerRoot, CurrentTime);
		return;
	}
	XSetInputFocus(dpy, focusedWindow, RevertToPointerRoot, CurrentTime);
}

void keyPress(XKeyEvent e)
{
	if(e.same_screen!=1) return;
	KeySym keysym = XLookupKeysym(&e, 0);
	for(int i = 0; i < sizeof(keyBinds)/sizeof(keyBinds[0]); i++)
	{
		if(keyBinds[i].keysym == keysym && keyBinds[i].modifiers == e.state)
		{
			keyBinds[i].function(keyBinds[i].arg);
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

	unsigned char* data;
	Atom type;
	int status = getProp(e.window, "_NET_WM_WINDOW_TYPE", &type, &data);
	if (status == Success && ((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", false))
	{
		XWindowAttributes attr;
		XGetWindowAttributes(dpy, e.window, &attr);
		bH = attr.height;
		bar = e.window;
		XFree(data);
		return;
	}
	XFree(data);

	

	Window focusedWindow;
	int revertToReturn;
	XGetInputFocus(dpy, &focusedWindow, &revertToReturn);
	XSetInputFocus(dpy, e.window, RevertToNone, CurrentTime);
	XSelectInput(dpy, e.window, EnterWindowMask);
	
	//Make client
	Client c = {currClientID, e.window, false};
	currClientID++;

	//Add to clients map
	clients.insert(pair<int, Client>(c.ID, c));

	//Make frame
	int pID = (frameIDS.count(focusedWindow)>0)? frames.find(frameIDS.find(focusedWindow)->second)->second.pID : currWS;
	vector<int> v;
	vector<int> floating;
	Frame f = {currFrameID, pID, true, c.ID, noDir, v, false, floating};
	currFrameID++;


	//Add ID to frameIDS map
	frameIDS.insert(pair<Window, int>(e.window, f.ID));

	status = getProp(e.window, "_NET_WM_STATE", &type, &data);
	if(status == Success && type!=None && (((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_STATE_MODAL", false) || ((Atom*)data)[0] == XInternAtom(dpy, "_NET_WM_STATE_ABOVE", false)))
	{
		clients.find(c.ID)->second.floating = true;
		frames.find(pID)->second.floatingFrameIDs.push_back(f.ID);
		frames.insert(pair<int, Frame>(f.ID, f));
		setWindowDesktop(e.window, currWS);
		updateClientList(clients);
		XFree(data);
		tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
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

	tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);
}

void destroyNotify(XDestroyWindowEvent e)
{
	if(frameIDS.count(e.window)<1)
		return;
	cout << "Destroy notif\n";
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
	tile(currWS, outerGaps, outerGaps, sW - outerGaps*2, sH - outerGaps*2 - bH);

	updateClientList(clients);
}
void clientMessage(XClientMessageEvent e)
{
	//cout << "Client message: " << XGetAtomName(dpy, e.message_type) << "\n";
	if(e.message_type == XInternAtom(dpy, "_NET_CURRENT_DESKTOP", false))
	{
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

	}
}

static int OnXError(Display* display, XErrorEvent* e)
{
	cout << "XError " << e->type << ":\n";
	char buffer_return[100];
	XGetErrorText(dpy, e->type, buffer_return, sizeof(buffer_return));
	printf("\t%s\n", buffer_return);
	return 0;
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
		wX += gaps;
		wY += gaps;
		wW -= gaps * 2;
		wH -= gaps * 2;
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
	dpy = XOpenDisplay(nullptr);
	root = Window(DefaultRootWindow(dpy));

	int screenNum = DefaultScreen(dpy);
	sW = DisplayWidth(dpy, screenNum);
	sH = DisplayHeight(dpy, screenNum);

	XSetErrorHandler(OnXError);
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);

	for(int i = 0; i < sizeof(keyBinds)/sizeof(keyBinds[0]); i++)
	{
		XGrabKey(dpy, XKeysymToKeycode(dpy, keyBinds[i].keysym), keyBinds[i].modifiers, root, false, GrabModeAsync, GrabModeAsync);
	}

	//EWMH
	initEWMH(&dpy, &root, numWS, workspaceNames);
	setCurrentDesktop(1);

	for(int i = 1; i < numWS + 1; i++)
	{
		vector<int> v;
		Frame rootFrame = {i, noID, false, noID, horizontal, v, true};
		frames.insert(pair<int, Frame>(i, rootFrame));
		currFrameID++;
	}
	for(int i = 0; i < sizeof(startup)/sizeof(startup[0]); i++)
	{
		if(fork() == 0)
		{
			system((startup[i] + " > /dev/null 2> /dev/null").c_str());
			exit(0);
		}
	}

		XSetInputFocus(dpy, root, RevertToNone, CurrentTime);
	cout << "Begin mainloop\n";

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
			case EnterNotify:
				//cout << e.xcrossing.window << "\n";
				if(e.xcrossing.window == root)
					break;
				XSetInputFocus(dpy, e.xcrossing.window, RevertToNone, CurrentTime);
				break;
			case ClientMessage:
				clientMessage(e.xclient);
				break;
			default:
				//cout << "Unhandled event, code: " << evNames[e.type] << "!\n";
				break;
		}
	}

	XCloseDisplay(dpy);
}
