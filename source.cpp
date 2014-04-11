/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: source.cpp - An application that allows the user to send data or files via TCP or UDP.
--							 Displays statistics regarding the transfers.
--
-- PROGRAM: TCP/UDP test tool
--
-- FUNCTIONS:
-- int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow)
-- LRESULT CALLBACK WndProc (HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
-- DWORD WINAPI SendPacketsThread(LPVOID)
-- DWORD WINAPI SendFileThread(LPVOID)
-- long getDelay (SYSTEMTIME start, SYSTEMTIME end)
-- BOOL CALLBACK ProtocolAndPort(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
-- BOOL CALLBACK IPConnect(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
-- BOOL CALLBACK SendTestPackets(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
-- BOOL CALLBACK SendFile(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
--
--
-- DATE: February 1, 2014
--
-- REVISIONS: None
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- NOTES:
-- The program requires some simple formatting to be used properly.
-- The menu items bring up dialog boxes for text entry.
-- All retrieved data will be output to a file called "data.txt"
----------------------------------------------------------------------------------------------------------------------*/

#define STRICT

#include <winsock2.h>
#include <windows.h>
#include <WindowsX.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include "resource.h"
#include "bass.h"
#include <fstream>
#include <sstream>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFPORT 5150
#define DEFPROT "TCP"
#define DEFSIZE 255
#define MAXSIZE 1024
#define DEFNUMP 10
#define DATA_BUFSIZE 65000

#define MAX_BUTTONS 3
#define BUTTON_ERROR 0
#define MODE_SERVER 2
#define MODE_CLIENT 1
#define MODE_NONE   0
#define MULTICAST_IP "234.5.6.7"

TCHAR Name[] = TEXT("Comm Audio");
char str[80] = "";
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
#pragma warning (disable: 4096)

HANDLE SendPacketThrd; //Handle to the Send Packet thread
HANDLE SendFileThrd; //Handle to the Send File thread
DWORD SendPacketThrdID;
DWORD SendFileThrdID;
DWORD PlaySongThrdID;
HANDLE TCPServerThrd;
DWORD TCPServerThrdID;
bool areaset = false;
int iVertPos = 0, iHorzPos = 0;
RECT text_area;
int mode = MODE_NONE;

int gPort = DEFPORT; //Global port
bool gTCP = true; //use TCP flag
bool gUDP = false; //use UDP flag
std::string gIP = "127.0.0.1"; //global IP address
int gNumPackets = 10; //default number of packets
int gPacketSize = 1024; //default packet size
std::string fileToSend = ""; //location of file to send
SYSTEMTIME startTime; //start of receive/send time
SYSTEMTIME endTime; //end of receive/send time
DWORD gBytesSent = 0; //number of bytes sent in total
int gPktsSent = 0; //total packets sent by application
HWND * gHwnd = 0;

/*
	Structure: SocketData
	Params: SOCKET * s, HWND hwnd
	Description: used for passing a socket descriptor and the handle to the main
	window to a thread for sending data and refreshing statistics.
*/
struct SocketData {
	SOCKET * s;
	char songname[1024];
	HWND hwnd;
	bool * MulticastFlag;
	bool * MicFlag;
	bool * Streaming;
};

struct SongData {
	HSTREAM * songHandle;
};

/*
	Asynchronous communication message
*/
#define WM_SOCKET (WM_USER + 1)

DWORD WINAPI SendFileThread(LPVOID); //Send file thread prototype
long getDelay (SYSTEMTIME start, SYSTEMTIME end); //Function prototype for the start time end time difference calculator
size_t Create_Button(HWND &hwnd, LPARAM lParam, HWND ** buttons, size_t &buttonCount, 
	size_t x, size_t y, size_t width, size_t height, LPCWSTR name, size_t buttonID);
void GetSongList(std::string songfile, HWND lbHwnd);

struct sockaddr_in * gaddr = 0; //global pointer to the socket info

struct sockaddr_in * gaddrclient = 0; //global pointer to the socket info


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: WinMain
--
-- DATE: February 1, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- RETURNS: A Windows message wParam.
--
-- NOTES:
-- The Main function for the program. Responsible for creating the Windows window for the application.
----------------------------------------------------------------------------------------------------------------------*/
int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hprevInstance,
 						  LPSTR lspszCmdParam, int nCmdShow)
{
	HWND hwnd;
	MSG Msg;
	WNDCLASSEX Wcl;

	Wcl.cbSize = sizeof (WNDCLASSEX);
	Wcl.style = CS_HREDRAW | CS_VREDRAW;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION); // large icon 
	Wcl.hIconSm = NULL; // use small version of large icon
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);  // cursor style
	
	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH); //white background
	Wcl.lpszClassName = Name;
	
	Wcl.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN); // The menu Class
	Wcl.cbClsExtra = 0;      // no extra memory needed
	Wcl.cbWndExtra = 0; 
	
	if (!RegisterClassEx (&Wcl))
		return 0;

	hwnd = CreateWindow (Name, Name, WS_OVERLAPPEDWINDOW, 10, 10,
   							800, 600, NULL, NULL, hInst, NULL);

	ShowWindow (hwnd, nCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&Msg, NULL, 0, 0))
	{
   		TranslateMessage (&Msg);
		DispatchMessage (&Msg);
	}

	return Msg.wParam;
}



/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: WndProc
--
-- DATE: February 1, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- RETURNS: False or 0
--
-- NOTES:
-- The main procedure for the application. 
-- Handles the various menu options and their functionality.
-- Handles creating client send threads.
-- Handles processing and displaying statistics.
-- Handles creating receiving UDP/TCP sockets
-- Handles creating sending UDP/TCP sockets
-- Handles placing received data in a file.
-- Handles receiving data via TCP/UDP
----------------------------------------------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc (HWND hwnd, UINT Message,
                          WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	HINSTANCE hInst;
	HMENU menu;
	static SOCKET client = INVALID_SOCKET;
	static SOCKET server = INVALID_SOCKET;
	static SOCKET connection = INVALID_SOCKET;
	static bool SENDFLAG = false;
	static bool tcp = true;
	static bool udp = false;
	static int port = DEFPORT;
	static std::string ip = "127.0.0.1";
	WSADATA wsaData;
	WSABUF buffer;
	DWORD error;
	DWORD bytesReceived, flags;
	static struct sockaddr_in addr, addr2;
	static struct ip_mreq mcAddr; //multicast stuct
	char dataBuffer[DATA_BUFSIZE];
	const int value = 1;
	static std::ofstream file;
	int addr_len = 0;
	static bool firstRecv;
	static bool initRecv;
	long time;
	double subTime;
	static int pktsRecv;
	static int totalBytesRecv;
	std::string output;
	std::ostringstream oss;
	TCHAR outBuffer[200];
	static struct SocketData * sData;
	static struct SocketData * sFileData;
	
	static HWND * buttons;
	static size_t buttonCount = 0;
	static size_t buttonIDs[MAX_BUTTONS];

	int device = -1;
	int errorBass = 0;
	int freq = 44100;
	static HSTREAM streamHandle;
	static HSTREAM streamBuffer;
	//float streamDataBuffer[512];
	char streamDataBuffer[10000];
	std::ifstream song;
	std::ofstream out;
	int songSize = 0;
	int readPos = 0;
	static char * songmemory;
	DWORD readLength = 0;
	static struct SongData * songdata;
	static bool started;

	static bool MUSIC_PLAYING;
	static bool MUSIC_PAUSED;

	static HANDLE songEvent;
	static HANDLE TimerEvent;
	static LPWSTR songstring;
	static LPWSTR songstring2;
	std::string songlist;

	wchar_t songname[1024];
	char mbsongname[1024];
	DWORD lbIndex;
	DWORD lbCount;

	static HWND listbox;
	static HBITMAP hBitmap;
	static bool fFlag, bFlag;
	static bool multicastFlag;
	static bool MicFlag;
	static bool Streaming;

	int nIP_TTL=2;
						DWORD cbRet;
	
	switch (Message)
	{
		case WM_CREATE:
			BASS_Init(device, freq, 0, 0, NULL);
			multicastFlag = false;
			MicFlag = false;
			firstRecv = true;
			initRecv = true;
			pktsRecv = 0;
			gHwnd = &hwnd;
			sData = 0;
			totalBytesRecv = 0;
			buttons = (HWND *)malloc(sizeof(HWND) * MAX_BUTTONS);
			MUSIC_PLAYING = false;
			MUSIC_PAUSED = false;
			Streaming = false;
			started = false;
			songdata = (struct SongData *)malloc(sizeof(struct SongData));

			Create_Button(hwnd, lParam, &buttons, buttonCount, 10, 10, 70, 50, TEXT("Play"), BTN_PLAY);
			Create_Button(hwnd, lParam, &buttons, buttonCount, 90, 10, 70, 50, TEXT("Pause"), BTN_PAUSE);
			songEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SONGSTARTED"));

			streamHandle = BASS_StreamCreate(freq, 2, 0, STREAMPROC_PUSH, 0);
			//strcpy(songstring, "A Proper Story.mp3");
			songstring = TEXT("A Proper Story.mp3");
			songstring2 = TEXT("Soviet Connection.mp3");
			songdata->songHandle = &streamHandle;
			hInst = GetModuleHandle(NULL);
			listbox = CreateWindow(TEXT("LISTBOX"), TEXT(""), WS_CHILD | WS_VISIBLE | LBS_STANDARD, 
							520, 10, 250, 480, hwnd, NULL, hInst, NULL);
			//SendMessage(listbox, LB_ADDSTRING, 0, (LPARAM)songstring);
			//SendMessage(listbox, LB_ADDSTRING, 0, (LPARAM)songstring2);
			songlist = "song-list.txt";
			GetSongList(songlist, listbox);
			Create_Button(hwnd, lParam, &buttons, buttonCount, 550, 480, 90, 35, TEXT("refresh"),BTN_REFRESH);
			Create_Button(hwnd, lParam, &buttons, buttonCount, 650, 480, 90, 35, TEXT("stream"),BTN_STREAM);
			TimerEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("LIMITER_EVENT"));
		break;
		case WM_COMMAND:
			switch (LOWORD (wParam))
			{
				case IDM_MULTICAST_OFF:
					multicastFlag = false;
					if (mode == MODE_CLIENT) {
						hInst = GetModuleHandle(NULL);
						menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CLIENT_MC_OFF));
						SetMenu(hwnd, menu);
						//InvalidateRect(hwnd, NULL, TRUE);
					} else if (mode == MODE_SERVER) {
						hInst = GetModuleHandle(NULL);
						menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_SERVER_MC_OFF));
						SetMenu(hwnd, menu);
						//InvalidateRect(hwnd, NULL, TRUE);
					}
				break;
				case IDM_MULTICAST_ON:
					multicastFlag = true;
					if (mode == MODE_CLIENT) {
						hInst = GetModuleHandle(NULL);
						menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CLIENT_MC_ON));
						SetMenu(hwnd, menu);
						//InvalidateRect(hwnd, NULL, TRUE);
					} else if (mode == MODE_SERVER) {
						hInst = GetModuleHandle(NULL);
						menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_SERVER_MC_ON));
						SetMenu(hwnd, menu);
						//InvalidateRect(hwnd, NULL, TRUE);
					}
				break;
				case BTN_REFRESH:
				break;
				case BTN_STREAM:
					if (mode == MODE_SERVER) {
						lbIndex = SendMessage(listbox, LB_GETCURSEL, 0, 0);
						SendMessage(listbox, LB_GETTEXT, lbIndex, (LPARAM)songname);

						wcstombs(mbsongname, songname, 1024);
						if (sFileData != 0) {
							free(sFileData);
						}
						Streaming = true;
						sFileData = (struct SocketData *)malloc(sizeof(struct SocketData));
						sFileData->hwnd = hwnd;
						sFileData->s = &server;
						sFileData->MulticastFlag = &multicastFlag;
						sFileData->MicFlag = &MicFlag;
						sFileData->Streaming = &Streaming;
						strcpy(sFileData->songname, mbsongname);
						if (SENDFLAG && server != INVALID_SOCKET) {
							SendFileThrd = CreateThread(NULL, 0, SendFileThread, (LPVOID)sFileData, 0, &SendFileThrdID);
						}
					}
				break;
				/*
				 *	Play the song.
				 */
				case BTN_PLAY:
					if (!MUSIC_PLAYING && !MUSIC_PAUSED) {
						MUSIC_PLAYING = true;
						if (started) {
							BASS_ChannelPlay(streamHandle, FALSE);
						}
					} else if (MUSIC_PAUSED) {
						MUSIC_PAUSED = false;
						BASS_ChannelPlay(streamHandle, FALSE);
					}
				break;
				/*
				 *	Pause the playback of the song.
				 */
				case BTN_PAUSE:
					if (MUSIC_PLAYING && !MUSIC_PAUSED) {
						MUSIC_PAUSED = true;
					
						BASS_ChannelPause(streamHandle);
					} else if (MUSIC_PAUSED) {
						MUSIC_PAUSED = false;
						BASS_ChannelPlay(streamHandle, FALSE);
					}
				break;
				/*
				 *	Creates the client socket and attempts to connect to the server.
				 */
				case ESTABLISH_CONNECT: //create client sockets
					WSAStartup(MAKEWORD(2,2), &wsaData);

					if (multicastFlag) {
						client = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_MULTIPOINT_C_LEAF  
              | WSA_FLAG_MULTIPOINT_D_LEAF| WSA_FLAG_OVERLAPPED);

						addr.sin_family = AF_INET; 
						addr.sin_addr.s_addr = INADDR_ANY; 
						addr.sin_port = htons (gPort);

						addr2.sin_family = AF_INET; 
						addr2.sin_addr.s_addr = inet_addr(MULTICAST_IP);; 
						addr2.sin_port = htons (gPort);

						gaddr = &addr;

						fFlag = TRUE;
						//set the socket opetions, yo
						setsockopt(client, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag));
						//bind the socket
						bind(client, (struct sockaddr*) &addr, sizeof(addr));
						memset(&mcAddr, 0, sizeof(mcAddr));
						//join the multicast group
						mcAddr.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP);
						mcAddr.imr_interface.s_addr = INADDR_ANY;
						//setsockopt(client, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mcAddr, sizeof(mcAddr));

						
						WSAIoctl(client,SIO_MULTICAST_SCOPE, &nIP_TTL,sizeof(nIP_TTL),NULL,0,&cbRet,NULL,NULL);


						bFlag=FALSE;
						WSAIoctl(client,SIO_MULTIPOINT_LOOPBACK, &bFlag,sizeof(bFlag),NULL,0,&cbRet,NULL,NULL);

						

						WSAAsyncSelect(client, hwnd, WM_SOCKET, FD_WRITE | FD_READ | FD_CLOSE);

						WSAJoinLeaf(client, (SOCKADDR *)&addr2,sizeof(addr2),NULL,NULL,NULL,NULL,JL_BOTH);

						WSABUF buffer;
						buffer.buf = 0;
						buffer.len = 0;
						//WSASendTo(client, &buffer, 1, NULL, NULL, (const sockaddr *)&addr, sizeof(addr), 0, 0);
					} else {
						client = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);

						addr.sin_family = AF_INET; 
						addr.sin_addr.s_addr = inet_addr(gIP.c_str()); 
						addr.sin_port = htons (gPort);
						gaddr = &addr;
						WSAAsyncSelect(client, hwnd, WM_SOCKET, FD_CONNECT | FD_WRITE | FD_READ | FD_CLOSE);

						WSABUF buffer;
						buffer.buf = 0;
						buffer.len = 0;
						WSASendTo(client, &buffer, 1, NULL, NULL, (const sockaddr *)&addr, sizeof(addr), 0, 0);
					}
					error = GetLastError();
				break;
				/*
				 *	UNUSED
				 */
				case IDM_SENDPACKETS: //send packet thread start
					if (sData != 0) {
						free(sData);
					}
					sData = (struct SocketData *)malloc(sizeof(struct SocketData));
					sData->hwnd = hwnd;
					sData->s = &client;
					if (SENDFLAG && client != INVALID_SOCKET) {
						//SendPacketThrd = CreateThread(NULL, 0, SendPacketsThread, (LPVOID)sData, 0, &SendPacketThrdID);
					}
				break;
				/*
				 *	Creates the "SendFileThread", this is currently used for streaming data.
				 */
				case IDM_SENDFILEDATA: //send file thread start
					if (sFileData != 0) {
						free(sFileData);
					}
					sFileData = (struct SocketData *)malloc(sizeof(struct SocketData));
					sFileData->hwnd = hwnd;
					sFileData->s = &client;
					if (SENDFLAG && client != INVALID_SOCKET) {
						SendFileThrd = CreateThread(NULL, 0, SendFileThread, (LPVOID)sFileData, 0, &SendFileThrdID);
					}
				break;
				/*
				 *	Sets the program into "CLIENT" mode.
				 *  Changes the menu.
				 */
				case IDM_CLIENT:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CLIENT_MC_OFF));
					SetMenu(hwnd, menu);
					mode = MODE_CLIENT;
					InvalidateRect(hwnd, NULL, TRUE);
				break;
				/*
				 *	Sets the program into "SERVER" mode.
				 *  Changes the menu.
				 */
				case IDM_SERVER:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_SERVER_MC_OFF));
					SetMenu(hwnd, menu);
					mode = MODE_SERVER;
					InvalidateRect(hwnd, NULL, TRUE);
				break;
				/*
				 *	Creates a dialog box for creating the protocol and port.
				 *  Protocol is now only UDP so that does not matter.
				 *  Port still matters.
				 */
				case IDM_PROTPORT:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_PORTNPROT), hwnd, ProtocolAndPort);
				break;
				/*
				 *  Client menu option
				 *	Used for creating the dialog box that allows us to connect to a server.
				 */
				case IDM_CONNECT:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_IPCONNECT), hwnd, IPConnect);
				break;
				/*
				 *	UNUSED
				 */
				case IDM_SENDTEST:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_SENDTEST), hwnd, SendTestPackets);
				break;
				/*
				 *	Creates the send file dialog box. No file necessary, this is the current trigger for
				 *  sending the song. Just press send. Need to create a real send thing from the server.
				 */
				case IDM_SENDFILE:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_SENDFILE), hwnd, SendFile);
				break;
				/*
				 *	Test menu option for controlling the list box of songs.
				 */
				case IDM_LISTBOX:
					//hInst = GetModuleHandle(NULL);
					//CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOGLB), hwnd, SendFile);
					lbIndex = SendMessage(listbox, LB_GETCURSEL, 0, 0);
					lbCount = SendMessage(listbox, LB_GETTEXTLEN, 0, 0);

					SendMessage(listbox, LB_GETTEXT, lbIndex, (LPARAM)songname);

					printf("");
				break;
				/*
				 *	The disconnect menu option.
				 *  Goes back to the initial programs state of having no connections.
				 */
				case IDM_DISCONNECT:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MAIN));
					SetMenu(hwnd, menu);
					if (mode == MODE_CLIENT) {
						if (client != INVALID_SOCKET) {
							closesocket(client);
						}
					} else if (mode == MODE_SERVER) {
						if (server != INVALID_SOCKET) {
							closesocket(server);
						}
						if (connection != INVALID_SOCKET) {
							closesocket(connection);
						}
					}
					Streaming = false;
					MicFlag = false;
					multicastFlag = false;
					mode = MODE_NONE;
					InvalidateRect(hwnd, NULL, TRUE);
				break;
				/*
				 *	Menu option for starting the server.
				 *  Currently the server receives data and doesn't send.
				 *  We need to change that.
				 */
				case IDM_STARTSERVER: //creates server sockets
					WSAStartup(MAKEWORD(2,2), &wsaData);

					if (multicastFlag) {
						server = socket(AF_INET, SOCK_DGRAM, 0);
					
						addr.sin_family = AF_INET; 
						addr.sin_addr.s_addr = inet_addr(INADDR_ANY); 
						addr.sin_port = htons (gPort);

						//bind
						bind(server, (struct sockaddr*) &addr, sizeof(addr));

						mcAddr.imr_multiaddr.s_addr = inet_addr("235.255.255.255");
						mcAddr.imr_interface.s_addr = INADDR_ANY;
						setsockopt(server, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mcAddr, sizeof(mcAddr));
						WSAAsyncSelect(server, hwnd, WM_SOCKET, FD_READ | FD_CLOSE); //binds reading to the WM_SOCKET message number.
						gaddr = &addr;
					} else {
						server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
						
						WSAAsyncSelect(server, hwnd, WM_SOCKET, FD_WRITE | FD_READ | FD_CLOSE); //binds reading to the WM_SOCKET message number.

						addr.sin_family = AF_INET; 
						addr.sin_addr.s_addr = htonl(INADDR_ANY); 
						addr.sin_port = htons (gPort);

						gaddr = &addr;
						//binds the socket descriptor to the port
						if (bind(server, (PSOCKADDR) &addr, sizeof(addr)) == SOCKET_ERROR)
						{
							printf("bind() failed with error %d\n", WSAGetLastError());
							return 0;
						}
					}
					firstRecv = true;
					initRecv = true;
					pktsRecv = 0;
					//free(buttons);
				break;
			}
		break;
		/*
		 *	WM_SOCKET is triggered whenever there is a read event.
		 *  This lets the program know that there is data in the winsock buffer that can be read.
		 *  WM_SOCKET is a defined number (at the top of the program) that is bound to the WSAAsyncSelect function.
		 */
		case WM_SOCKET:
			switch(WSAGETSELECTEVENT(lParam)) {
				case FD_CONNECT:
				break;
				case FD_WRITE: //lets us know that it is safe to start writing
					SENDFLAG = true;
				break;
				case FD_CLOSE:
				break;
				case FD_READ: //triggers when there is data to read
					if (mode == MODE_CLIENT) {
						flags = 0;
						buffer.buf = dataBuffer;
						buffer.len = DATA_BUFSIZE;
						addr_len = sizeof(*gaddr);
						WSARecvFrom(client, &buffer, 1, &bytesReceived, &flags, (sockaddr*)gaddr, &addr_len, NULL, NULL);
						error = GetLastError();
						if (error != 0) {
							printf("");
						}
						if (bytesReceived > 0) {
							BASS_StreamPutData(streamHandle, dataBuffer, bytesReceived);
						}
						if (!started && bytesReceived > 0) {
							started = true;
							BASS_ChannelPlay(streamHandle, FALSE);
						}

						if (initRecv) {
							initRecv = false;
						} else if (firstRecv) {
							firstRecv = false;
							if (bytesReceived > 0) {
								pktsRecv += 1;
							}
						} else {
							pktsRecv += 1;
						}
						totalBytesRecv += bytesReceived;
					} else {
						flags = 0;
						buffer.buf = dataBuffer;
						buffer.len = DATA_BUFSIZE;
						addr_len = sizeof(*gaddr);
						if (multicastFlag) {
							
						} else {
							WSARecvFrom(server, &buffer, 1, &bytesReceived, &flags, (sockaddr*)gaddr, &addr_len, NULL, NULL);
						
							strcpy(mbsongname, inet_ntoa(gaddr->sin_addr));
							printf("IP address is: %s\n", mbsongname);
						}
					}
				break;
				default:
				break;
			}
		break;
		case WM_TIMER:
			SetEvent(TimerEvent);
		break;
		/*	
		 *	Handles cleaning up sockets and malloced memory.
		 */
		case WM_DESTROY:	// Terminate program and freeing data and sockets
			if (client != INVALID_SOCKET) {
				closesocket(client); //close client socket
			}
			if (server != INVALID_SOCKET) {
				closesocket(server);
			}
			if (file.is_open()) {
				file.close();
			}
			if (connection != INVALID_SOCKET) {
				closesocket(connection);
			}
			if (sData != 0) {
				free(sData);
			}
			if (sFileData != 0) {
				free(sFileData);
			}
      		PostQuitMessage (0);
			free(songdata);
			BASS_Free();
		break;
		case WM_PAINT: //prints statistics
			hdc = BeginPaint(hwnd, &ps);

			ReleaseDC(hwnd, hdc);
		break;
		default:
			return DefWindowProc (hwnd, Message, wParam, lParam);
	}
	return 0;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SendFileThread
--
-- DATE: February 9, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- PARAM: LPVOID n - pointer to the SocketData structure containing the socket descriptor and the handle to the parent
                     window that needs to be refreshed.
--
-- RETURNS: DWORD
--
-- NOTES:
-- Thread for sending a file via TCP or UDP
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI SendFileThread(LPVOID n) {
	struct SocketData * sData = (struct SocketData *)n;
	SOCKET *s = sData->s;
	HWND hwnd = sData->hwnd;
	DWORD SendBytes = 0;
	DWORD EventTotal = 0;
	DWORD BytesTransferred = 0;
	WSABUF buffer;
	std::string data = "";
	WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
	EventArray[EventTotal] = WSACreateEvent();
	WSAOVERLAPPED ov;
	ZeroMemory(&ov, sizeof(WSAOVERLAPPED));
	DWORD Flags = 0;
	ov.hEvent = EventArray[EventTotal];
	EventTotal++;
	BOOL Result;
	std::ofstream inputFile;
	inputFile.open("input.txt", std::ios::out | std::ios::trunc);
	std::string line;
	gBytesSent = 0;
	struct sockaddr_in addr;
	bool multicastFlag = *(sData->MulticastFlag);
	bool micFlag = *(sData->MicFlag);
	bool running = true;


	if (sData->MulticastFlag) {
		addr.sin_family =      AF_INET;
		addr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
		addr.sin_port =        htons(gPort);
	}

	char streamDataBuffer[4096];
	HSTREAM streamBuffer;
	streamBuffer = BASS_StreamCreateFile(FALSE, sData->songname, 0, 0, BASS_STREAM_DECODE);
	//BASS_RecordInit(-1);
	//streamBuffer = BASS_RecordStart(44100, 2, 0, 0, 0); // start recording
	bool started = false;
	int tPacketsSent = 0;
	HANDLE songEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SONGSTARTED"));
	HANDLE TimerEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("LIMITER_EVENT"));

	while(BASS_ChannelIsActive(streamBuffer) && running && (*(sData->Streaming))) {
		if (micFlag == true) {
			if (*(sData->MicFlag) == false) {
				running = false;
				break;
			}
		}

		if (multicastFlag == true) {
			if (*(sData->MulticastFlag) == false) {
				running = false;
				break;
			}
		}

		DWORD readLength = BASS_ChannelGetData(streamBuffer, streamDataBuffer, 4096);

		if (readLength > 0) {
			inputFile << streamDataBuffer;
		}

		buffer.len = readLength;
		buffer.buf = (CHAR*)streamDataBuffer;
		if (multicastFlag) {
			int errorcode;
			errorcode = WSASendTo(*s, &buffer, 1, &SendBytes, 0, (const sockaddr *)&addr, sizeof(addr), &ov, 0);
			printf("");
		} else {
			WSASendTo(*s, &buffer, 1, &SendBytes, 0, (const sockaddr *)gaddr, sizeof((*gaddr)), &ov, 0);
		}

		tPacketsSent++;
		WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE); //waits for send to finish
		gPktsSent += 1;
		GetSystemTime(&endTime);
		Result = WSAGetOverlappedResult(*s, &ov, &BytesTransferred, FALSE, &Flags);
		WSAResetEvent(EventArray[0]); //reset event
		ZeroMemory(&ov, sizeof(WSAOVERLAPPED)); //reset overlapped structure
		ov.hEvent = EventArray[0];
		
		gBytesSent += SendBytes;
		
		SetTimer(hwnd, NULL, 9, NULL);
		WaitForSingleObject(TimerEvent, 25);
	}
 	inputFile.close();

	closesocket(*s);
	return 0;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: getDelay
--
-- DATE: February 9, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- PARAM: SYSTEMTIME start - start of receive/transmission
--		  SYSTEMTIME end - end of receive/transmission
--
-- RETURNS: long
--
-- NOTES:
-- Function for calculating the difference between a start time and a end time
----------------------------------------------------------------------------------------------------------------------*/
long getDelay (SYSTEMTIME start, SYSTEMTIME end)
{
	long d;

	d = (end.wMinute - start.wMinute) * 1000 * 1000;
	d += (end.wSecond - start.wSecond) * 1000;
	d += (end.wMilliseconds - start.wMilliseconds);
	return(d);
}

/*Creates a new button such as Pause, Play, etc.*/
size_t Create_Button(HWND &hwnd, LPARAM lParam, HWND ** buttons, size_t &buttonCount, 
	size_t x, size_t y, size_t width, size_t height, LPCWSTR name, size_t buttonID) {
	if (buttonCount < MAX_BUTTONS) {
		(*buttons)[buttonCount] = CreateWindow ( TEXT("button"),//type of child window 
							name,//text displayed on button
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,//type of button
                            x, y,
                            width, height,
                            hwnd, //parent handle i.e. main window handle
							(HMENU) buttonID,//child ID – any unique number
                            ((LPCREATESTRUCT) lParam)->hInstance,
							NULL) ;
		return buttonID;
	}

	return BUTTON_ERROR;
}

void GetSongList(std::string songfile, HWND lbHwnd) {
	std::ifstream songlist;
	songlist = std::ifstream(songfile);
	std::string line;
	wchar_t *lpwstr;
	while(!songlist.eof()) {
		std::getline(songlist, line); 
		line += '\0';
		lpwstr = (wchar_t *)malloc(sizeof(wchar_t) * line.size());
		memset(lpwstr, 0, line.size());
		MultiByteToWideChar(CP_UTF8, NULL, line.c_str(), line.size(), lpwstr, line.size());
		//mbstowcs(lpwstr, line.c_str(), line.size());
		SendMessage(lbHwnd, LB_ADDSTRING, 0, (LPARAM)lpwstr);
		free(lpwstr);
	}
}