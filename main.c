#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#define MAX_PLAYERS 4
#define MAX_PATH_LENGTH 256
#define MAX_PROGRESS_LENGTH 24

unsigned int COMMAND_SIZE = 4;

// MPC-HC API commands
unsigned int CMD_CONNECT = 0x50000000;
unsigned int CMD_STATE = 0x50000001;
unsigned int CMD_PLAYMODE = 0x50000002;
unsigned int CMD_NOWPLAYING = 0x50000003;
unsigned int CMD_NOTIFYSEEK = 0x50000008;
unsigned int CMD_DISCONNECT = 0x5000000B;
unsigned int CMD_OPENFILE = 0xA0000000;
unsigned int CMD_STOP = 0xA0000001;
unsigned int CMD_CLOSEFILE = 0xA0000002;
unsigned int CMD_PLAY = 0xA0000004;
unsigned int CMD_PAUSE = 0xA0000005;
unsigned int CMD_SETPOSITION = 0xA0002000;
unsigned int CMD_CLOSEAPP = 0xA0004006;
int LOADSTATE_LOADED = 50;
int LOADSTATE_CLOSED = 48;
int PLAYSTATE_PLAY = 48;
int PLAYSTATE_PAUSE = 49;

typedef struct {
	HWND hwnd;
} MpcInstance;

typedef struct {
	MpcInstance* instances;
	int size;
	int count;
	int hasFilePath;
	COPYDATASTRUCT cds;
	wchar_t* pos;
	wchar_t* path;
	int wasPause;
} MpcHost;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int wmain(int argc, wchar_t *argv[])
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	const wchar_t CLASS_NAME[] = L"MPC-HC Host Class";
	MpcInstance mpcInstances[MAX_PLAYERS];
	memset(mpcInstances, 0, sizeof(HWND) * MAX_PLAYERS);
	MpcHost host;
	host.size = MAX_PLAYERS;
	host.instances = mpcInstances;
	host.count = 0;
	host.hasFilePath = 0;
	host.wasPause = 0;
	host.pos = malloc(MAX_PROGRESS_LENGTH);
	if(host.pos == NULL) {
		printf("malloc failed\n");
		return 1;
	}
	host.path = malloc(MAX_PATH_LENGTH * 2);
	if(host.path == NULL) {
		printf("malloc failed\n");
		return 1;
	}
	
	WNDCLASS wc = {0};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"MPC-HC Host",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	if(hwnd == NULL) {
		printf("CreateWindow failed\n");
		return 1;
	}
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)&host);
	printf("Start up to 4 MPC-HC instances from cmd with arguments: /new /slave %d\n", hwnd);
	printf("For example: mpc-hc64.exe /new /slave %d\n", hwnd);
	printf("First connected instance will be main, other instances will sync to it.\n");
	
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DestroyWindow(hwnd);
	free(host.path);
	free(host.pos);
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COPYDATA:
		COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
		HWND sender = (HWND) wParam;
		MpcHost* host = (MpcHost*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		
		if(cds->dwData == CMD_CONNECT)
		{
			MpcHost* host = (MpcHost*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
			if(host->count < host->size) {
				HWND newHwnd = (HWND)((UINT64)_wtoi(cds->lpData));
				host->instances[host->count].hwnd = newHwnd;
				host->count += 1;
				printf("MPC-HC connected: %d\n", newHwnd);
			}
		}
		
		if(cds->dwData == CMD_STATE)
		{
			if(sender == host->instances[0].hwnd) {
				if(*((int*)cds->lpData) == LOADSTATE_LOADED) {
					host->hasFilePath = 0;
				}
				if(*((int*)cds->lpData) == LOADSTATE_CLOSED) {
					host->hasFilePath = 0;
					host->cds.dwData = CMD_CLOSEFILE;
					host->cds.cbData = COMMAND_SIZE;
					for(int i = 1; i < host->count; i++) {
						SendMessage(host->instances[i].hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&host->cds);
					}	
				}
			}
		}
		
		if(cds->dwData == CMD_PLAYMODE)
		{
			int state = *((int*)cds->lpData);
			if(sender == host->instances[0].hwnd) {
				if(state == PLAYSTATE_PLAY) {
					if(host->wasPause) {
						host->wasPause = 0;
						host->cds.dwData = CMD_PLAY;
						host->cds.cbData = COMMAND_SIZE;
						for(int i = 1; i < host->count; i++) {
							SendMessage(host->instances[i].hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&host->cds);
						}
						
					}
				}
				if(state == PLAYSTATE_PAUSE) {
					host->wasPause = 1;
					host->cds.dwData = CMD_PAUSE;
					host->cds.cbData = COMMAND_SIZE;
					for(int i = 1; i < host->count; i++) {
						SendMessage(host->instances[i].hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&host->cds);
					}
				}
			}
		}
		
		if(cds->dwData == CMD_NOWPLAYING)
		{
			if(sender != host->instances[0].hwnd) {
				return 0;
			}
			if(!host->hasFilePath) {
				int wStrLen = wcslen(cds->lpData);
				int startpos, endpos;
				int counter = 0;
				wchar_t* data = (wchar_t*) cds->lpData;
				for(int i = 0; i < wStrLen; i++) {
					if(data[i] == L'|') {
						if(i > 0) {
							if(data[i-1] == L'\\') {
								continue;
							}
						}
						if(counter == 2) {
							startpos = i + 1;
						}
						if(counter == 3) {
							endpos = i + 1;
						}
						counter++;
					}
				}
				int pathLen = endpos - startpos;
				memcpy(host->path, data + startpos, pathLen * 2);
				host->path[pathLen-1] = L'\0';
				host->hasFilePath = 1;
				host->cds.dwData = CMD_OPENFILE;
				host->cds.cbData = pathLen * 2;
				host->cds.lpData = host->path;

				for(int i = 1; i < host->count; i++) {
					SendMessage(host->instances[i].hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&host->cds);
				}
			}
		}
		
		if(cds->dwData == CMD_NOTIFYSEEK)
		{
			if(sender == host->instances[0].hwnd) {
				memcpy(host->pos, cds->lpData, cds->cbData);
				host->cds.dwData = CMD_SETPOSITION;
				host->cds.cbData = cds->cbData;
				host->cds.lpData = host->pos;
				for(int i = 1; i < host->count; i++) {
					SendMessage(host->instances[i].hwnd, WM_COPYDATA, 0, (LPARAM)&host->cds);
				}
			}
		}

		if(cds->dwData == CMD_DISCONNECT)
		{
			if(sender == host->instances[0].hwnd) {
				for(int i = 1; i < host->count; i++) {
					host->cds.dwData = CMD_CLOSEAPP;
					host->cds.cbData = COMMAND_SIZE;
					SendMessage(host->instances[i].hwnd, WM_COPYDATA, 0, (LPARAM)&host->cds);
				}
				PostQuitMessage(0);
			} else {
				int index = -1;
				for(int i = 1; i < host->count; i++) {
					if(sender == host->instances[i].hwnd) {
						index = i;
					}
				}
				if(index == -1) {
					return 0;
				}
				for(int i = 1; i < host->count; i++) {
					if(i > index) {
						host->instances[i-1] = host->instances[i];
					}
				}
				host->count -= 1;
			}
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
