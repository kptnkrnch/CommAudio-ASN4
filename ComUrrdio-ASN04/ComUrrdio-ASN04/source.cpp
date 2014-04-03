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

#pragma comment(lib, "ws2_32.lib")

#define DEFPORT 5150
#define DEFPROT "TCP"
#define DEFSIZE 255
#define MAXSIZE 1024
#define DEFNUMP 10
#define DATA_BUFSIZE 65000

#define MAX_BUTTONS 2
#define BUTTON_ERROR 0

TCHAR Name[] = TEXT("Assignment 02");
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
int mode = 0;

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
	HWND hwnd;
};

struct SongData {
	HSTREAM * songHandle;
};

/*
	Asynchronous communication message
*/
#define WM_SOCKET (WM_USER + 1)

DWORD WINAPI SendPacketsThread(LPVOID); //Send specified packets thread prototype
DWORD WINAPI SendFileThread(LPVOID); //Send file thread prototype
DWORD WINAPI ReadSongThread(LPVOID n);
DWORD WINAPI PlaySongThread(LPVOID n);
long getDelay (SYSTEMTIME start, SYSTEMTIME end); //Function prototype for the start time end time difference calculator
size_t Create_Button(HWND &hwnd, LPARAM lParam, HWND ** buttons, size_t &buttonCount, 
	size_t x, size_t y, size_t width, size_t height, LPCWSTR name, size_t buttonID);

struct sockaddr_in * gaddr = 0; //global pointer to the socket info


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
   							600, 400, NULL, NULL, hInst, NULL);

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
	static struct sockaddr_in addr;
	char dataBuffer[2048];
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
	
	switch (Message)
	{
		case WM_CREATE:
			BASS_Init(device, freq, 0, 0, NULL);
			firstRecv = true;
			initRecv = true;
			pktsRecv = 0;
			gHwnd = &hwnd;
			sData = 0;
			totalBytesRecv = 0;
			buttons = (HWND *)malloc(sizeof(HWND) * MAX_BUTTONS);
			MUSIC_PLAYING = false;
			MUSIC_PAUSED = false;
			started = false;
			songdata = (struct SongData *)malloc(sizeof(struct SongData));

			Create_Button(hwnd, lParam, &buttons, buttonCount, 10, 10, 70, 50, TEXT("Play"), BTN_PLAY);
			Create_Button(hwnd, lParam, &buttons, buttonCount, 80, 10, 70, 50, TEXT("Pause"), BTN_PAUSE);
			songEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SONGSTARTED"));

			streamHandle = BASS_StreamCreate(freq, 2, 0, STREAMPROC_PUSH, 0);

			songdata->songHandle = &streamHandle;
		break;
		case WM_COMMAND:
			switch (LOWORD (wParam))
			{
				case BTN_PLAY:
					if (!MUSIC_PLAYING && !MUSIC_PAUSED) {
						MUSIC_PLAYING = true;
						//streamBuffer = BASS_StreamCreateFile(FALSE, "A Proper Story.mp3", 0, 0, BASS_STREAM_DECODE);
						//streamHandle = BASS_StreamCreate(freq, 2, BASS_SAMPLE_FLOAT, NULL ,NULL);
						//streamHandle = BASS_StreamCreateFileUser(STREAMFILE_BUFFERPUSH, BASS_SAMPLE_FLOAT, 0, 0);
						//song = std::ifstream("A Proper Story.mp3", std::ios::in | std::ios::binary);
						if (started) {
							//SetEvent(songEvent);
							BASS_ChannelPlay(streamHandle, FALSE);
						}
						/*song.seekg(0, song.end);
						songSize = song.tellg();
						song.seekg(0, song.beg);

						songmemory = (char*)malloc(songSize);*/

						/*while(song.read(streamDataBuffer, 2048)) {
							memcpy(songmemory + readPos, streamDataBuffer, 2048);
							readPos += 2048;
						}*/
						/*song.read(streamDataBuffer, 2048);
						memcpy(songmemory + readPos, streamDataBuffer, 2048);
						readPos += 2048;*/

						//streamHandle = BASS_StreamCreateFile(true, songmemory, 0, songSize, BASS_SAMPLE_FLOAT);
						//streamHandle = BASS_StreamCreate(freq, 2, 0, STREAMPROC_PUSH, 0);

						/*errorBass = BASS_ErrorGetCode();

						while(BASS_ChannelIsActive(streamBuffer)) {
							readLength = BASS_ChannelGetData(streamBuffer, streamDataBuffer, 2048);
							BASS_StreamPutData(streamHandle, streamDataBuffer, readLength);
						}*/
						//songEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SONGSTARTED"));
						//songdata->songHandle = &streamHandle;
						//SendPacketThrd = CreateThread(NULL, 0, ReadSongThread, (LPVOID)songdata, 0, &SendPacketThrdID);
						/*while(song.read(streamDataBuffer, 2048)) {
							BASS_StreamPutData(streamHandle, streamDataBuffer, 2048);
							//memcpy(songmemory + readPos, streamDataBuffer, 2048);
							//readPos += 2048;
						}*/

						//WaitForSingleObject(songEvent, INFINITE);

						//song.close();
						
						/*if (!BASS_ChannelPlay(streamHandle, FALSE)) {
							errorBass = BASS_ErrorGetCode();
						}*/
					} else if (MUSIC_PAUSED) {
						MUSIC_PAUSED = false;
						BASS_ChannelPlay(streamHandle, FALSE);
					}
				break;
				case BTN_PAUSE:
					if (MUSIC_PLAYING && !MUSIC_PAUSED) {
						MUSIC_PAUSED = true;
					
						BASS_ChannelPause(streamHandle);
					} else if (MUSIC_PAUSED) {
						MUSIC_PAUSED = false;
						BASS_ChannelPlay(streamHandle, FALSE);
					}
				break;
				case ESTABLISH_CONNECT: //create client sockets
					WSAStartup(MAKEWORD(2,2), &wsaData);

					if (gTCP && gUDP || !gTCP && !gUDP) {
						perror("Invalid protocol settings.");
						break;
					} else if (gTCP) {
						client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
					} else {
						client = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
					}

					addr.sin_family = AF_INET; 
					addr.sin_addr.s_addr = inet_addr(gIP.c_str()); 
					addr.sin_port = htons (gPort);
					if (gUDP) {
						gaddr = &addr;
					}
					WSAAsyncSelect(client, hwnd, WM_SOCKET, FD_CONNECT | FD_WRITE | FD_CLOSE);

					if (gTCP) {
						if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
							perror("Cannot connect to server");
							error = GetLastError();
							if (error != 6) {
								closesocket(client);
							}
						}
					} else {
						WSABUF buffer;
						buffer.buf = 0;
						buffer.len = 0;
						WSASendTo(client, &buffer, 1, NULL, NULL, (const sockaddr *)&addr, sizeof(addr), 0, 0);
					}
					
					error = GetLastError();
				break;
				case IDM_SENDPACKETS: //send packet thread start
					if (sData != 0) {
						free(sData);
					}
					sData = (struct SocketData *)malloc(sizeof(struct SocketData));
					sData->hwnd = hwnd;
					sData->s = &client;
					if (SENDFLAG && client != INVALID_SOCKET) {
						SendPacketThrd = CreateThread(NULL, 0, SendPacketsThread, (LPVOID)sData, 0, &SendPacketThrdID);
					}
				break;
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
				case IDM_CLIENT:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CLIENT));
					SetMenu(hwnd, menu);
					mode = 1;
					InvalidateRect(hwnd, NULL, TRUE);
				break;
				case IDM_SERVER:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_SERVER));
					SetMenu(hwnd, menu);
					mode = 2;
					InvalidateRect(hwnd, NULL, TRUE);
				break;
				case IDM_PROTPORT:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_PORTNPROT), hwnd, ProtocolAndPort);
				break;
				case IDM_CONNECT:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_IPCONNECT), hwnd, IPConnect);
				break;
				case IDM_SENDTEST:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_SENDTEST), hwnd, SendTestPackets);
				break;
				case IDM_SENDFILE:
					hInst = GetModuleHandle(NULL);
					CreateDialog(hInst, MAKEINTRESOURCE(IDD_SENDFILE), hwnd, SendFile);
				break;
				case IDM_DISCONNECT:
					hInst = GetModuleHandle(NULL);
					menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MAIN));
					SetMenu(hwnd, menu);
					if (mode == 1) {
						if (client != INVALID_SOCKET) {
							closesocket(client);
						}
					} else if (mode == 2) {
						if (server != INVALID_SOCKET) {
							closesocket(server);
						}
						if (connection != INVALID_SOCKET) {
							closesocket(connection);
						}
					}
					mode = 0;
					InvalidateRect(hwnd, NULL, TRUE);
				break;
				case IDM_STARTSERVER: //creates server sockets
					WSAStartup(MAKEWORD(2,2), &wsaData);

					if (gTCP && gUDP || !gTCP && !gUDP) {
						perror("Invalid protocol settings.");
					} else if (gTCP) {
						server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					} else {
						server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
						//file.open("data.txt", std::ios::out | std::ios::trunc);
					}

					if (gTCP) {
						WSAAsyncSelect(server, hwnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE);
					} else {
						WSAAsyncSelect(server, hwnd, WM_SOCKET, FD_READ | FD_CLOSE);
					}

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
					//starts the TCP listener
					if(gTCP && listen(server,(1))==SOCKET_ERROR)
					{
						MessageBox(hwnd,TEXT("Unable to listen!"),NULL,MB_OK);
						SendMessage(hwnd,WM_DESTROY,NULL,NULL);
						break;
					}
					firstRecv = true;
					initRecv = true;
					pktsRecv = 0;
					//free(buttons);
				break;
			}
		break;
		case WM_SOCKET:
			switch(WSAGETSELECTEVENT(lParam)) {
				case FD_ACCEPT: //accepts connections into a new socket
					connection = accept(wParam, NULL, NULL);
					WSAAsyncSelect(connection, hwnd, WM_SOCKET, FD_READ | FD_CLOSE);
					if (gTCP) {
						//file.open("data.txt", std::ios::out | std::ios::trunc);
					}
				break;
				case FD_CONNECT:
				break;
				case FD_WRITE: //lets us know that it is safe to start writing
					SENDFLAG = true;
				break;
				case FD_CLOSE:
				break;
				case FD_READ: //triggers when there is data to read
					if (gTCP) { //TCP read block
						flags = 0;
						//buffer.buf = dataBuffer;
						//buffer.len = DATA_BUFSIZE;
						WSARecv(connection, &buffer, 1, &bytesReceived, &flags, NULL, NULL);
						error = GetLastError();
						if (firstRecv) {
							GetSystemTime(&startTime);
							firstRecv = false;
							GetSystemTime(&endTime);
						} else {
							GetSystemTime(&endTime);
						}
						pktsRecv += 1;
						InvalidateRect(hwnd, NULL, TRUE);
					} else { //UDP read block
						flags = 0;
						buffer.buf = (CHAR*)dataBuffer;
						buffer.len = sizeof(char) * 10000;
						addr_len = sizeof(*gaddr);
						WSARecvFrom(server, &buffer, 1, &bytesReceived, &flags, (sockaddr*)gaddr, &addr_len, NULL, NULL);
						error = GetLastError();
						error = BASS_ErrorGetCode();
						if (error != 0) {
							printf("");
						}
						if (bytesReceived > 0) {
							BASS_StreamPutData(streamHandle, dataBuffer, bytesReceived);
						}
						if (!started && bytesReceived > 0) {
							//CreateThread(NULL, 0, PlaySongThread, (LPVOID)songdata, 0, &PlaySongThrdID);
							//BASS_ChannelPlay(streamHandle, FALSE);
							started = true;
						}
						if (initRecv) {
							initRecv = false;
						} else if (firstRecv) {
							firstRecv = false;
							GetSystemTime(&startTime);
							GetSystemTime(&endTime);
							if (bytesReceived > 0) {
								pktsRecv += 1;
							}
						} else {
							pktsRecv += 1;
							GetSystemTime(&endTime);
						}
						InvalidateRect(hwnd, NULL, TRUE);
					}
					totalBytesRecv += bytesReceived;
					//dataBuffer[bytesReceived] = '\0';
					/*if (file.is_open() && bytesReceived > 0) {
						file << dataBuffer;
					}*/
				break;
				default:
				break;
			}
		break;
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
			//free(songmemory);
			free(songdata);
			BASS_Free();
		break;
		case WM_PAINT: //prints statistics
			hdc = BeginPaint(hwnd, &ps);
			if (mode == 2) { //prints statistics for received data
				time = (startTime.wMinute * 1000 * 1000);
				time += (startTime.wSecond * 1000);
				time += (startTime.wMilliseconds);
				oss << "Start Time: " << time << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 10, outBuffer, output.size());
				oss.str("");
				
				time = (endTime.wMinute * 1000 * 1000);
				time += (endTime.wSecond * 1000);
				time += (endTime.wMilliseconds);
				oss << "End Time: " << time << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 30, outBuffer, output.size());
				oss.str("");

				time = getDelay(startTime, endTime);
				oss << "Time Difference: " << time << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 50, outBuffer, output.size());
				oss.str("");

				subTime = 0;
				if (pktsRecv > 0) {
					subTime = (double)time / pktsRecv;
				}
				oss << "Avg Delay: " << subTime << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 70, outBuffer, output.size());
				oss.str("");

				oss << "Pkts Received: " << pktsRecv;
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 90, outBuffer, output.size());
				oss.str("");

				oss << "Num Bytes Recv: " << totalBytesRecv;
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 110, outBuffer, output.size());
				oss.str("");

				if (gTCP) {
					output = "Protocol: TCP";
					MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
					TextOut(hdc, 10, 130, outBuffer, output.size());
				} else {
					output = "Protocol: UDP";
					MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
					TextOut(hdc, 10, 130, outBuffer, output.size());
				}
			} else if (mode == 1) { //Prints statistics for sending
				time = (startTime.wMinute * 1000 * 1000);
				time += (startTime.wSecond * 1000);
				time += (startTime.wMilliseconds);
				oss << "Start Time: " << time << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 10, outBuffer, output.size());
				oss.str("");
				
				time = (endTime.wMinute * 1000 * 1000);
				time += (endTime.wSecond * 1000);
				time += (endTime.wMilliseconds);
				oss << "End Time: " << time << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 30, outBuffer, output.size());
				oss.str("");

				time = getDelay(startTime, endTime);
				oss << "Time Difference: " << time << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 50, outBuffer, output.size());
				oss.str("");

				subTime = 0;
				if (gPktsSent > 0) {
					subTime = (double)time / gPktsSent;
				}
				oss << "Avg Delay: " << subTime << " ms";
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 70, outBuffer, output.size());
				oss.str("");

				oss << "Pkts Sent: " << gPktsSent;
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 90, outBuffer, output.size());
				oss.str("");

				oss << "Num Bytes Sent: " << gBytesSent;
				output = oss.str();
				MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
				TextOut(hdc, 10, 110, outBuffer, output.size());
				oss.str("");

				if (gTCP) {
					output = "Protocol: TCP";
					MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
					TextOut(hdc, 10, 130, outBuffer, output.size());
				} else {
					output = "Protocol: UDP";
					MultiByteToWideChar(CP_UTF8, NULL, output.c_str(), output.size(), outBuffer, output.size());
					TextOut(hdc, 10, 130, outBuffer, output.size());
				}
			}
			ReleaseDC(hwnd, hdc);
		break;
		default:
			return DefWindowProc (hwnd, Message, wParam, lParam);
	}
	return 0;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SendPacketsThread
--
-- DATE: February 7, 2014
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
-- Thread for sending a specified number of packets of a certain size.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI SendPacketsThread(LPVOID n) {
	struct SocketData * sData = (struct SocketData *)n;
	SOCKET *s = sData->s;
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

	gBytesSent = 0;

	for (int i = 0; i < gPacketSize - 1; i++) {
		data += "A";
	}
	data += "\n";

	/*setting send buffer*/
	buffer.len = data.size();
	buffer.buf = (CHAR*)data.c_str();
	GetSystemTime(&startTime);
	for (int i = 0; i < gNumPackets; i++) {
		if (gTCP) { //sends TCP packets
			WSASend(*s, &buffer, 1, &SendBytes, 0, &ov, NULL);
		} else { //sends UDP packets
			WSASendTo(*s, &buffer, 1, &SendBytes, 0, (const sockaddr *)gaddr, sizeof((*gaddr)), &ov, 0); 
		}
		WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE); //wait to finish sending
		gPktsSent += 1;
		GetSystemTime(&endTime);
		Result = WSAGetOverlappedResult(*s, &ov, &BytesTransferred, FALSE, &Flags);
		WSAResetEvent(EventArray[0]); //reset overlapped event
		ZeroMemory(&ov, sizeof(WSAOVERLAPPED)); //reset overlapped structure
		ov.hEvent = EventArray[0];
		
		gBytesSent += SendBytes;
		InvalidateRect(sData->hwnd, NULL, TRUE);
	}

	closesocket(*s);
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
	std::ifstream sendFile;
	sendFile.open(fileToSend.c_str());
	std::string line;
	gBytesSent = 0;

	char streamDataBuffer[2048];
	HSTREAM streamBuffer = BASS_StreamCreateFile(FALSE, "A Proper Story.mp3", 0, 0, BASS_STREAM_DECODE);
	bool started = false;

	HANDLE songEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SONGSTARTED"));

	while(BASS_ChannelIsActive(streamBuffer)) {
		DWORD readLength = BASS_ChannelGetData(streamBuffer, streamDataBuffer, 2048);
		//BASS_StreamPutData(*(songdata->songHandle), streamDataBuffer, readLength);

		buffer.len = readLength;
		buffer.buf = (CHAR*)streamDataBuffer;

		WSASendTo(*s, &buffer, 1, &SendBytes, 0, (const sockaddr *)gaddr, sizeof((*gaddr)), &ov, 0);

		WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE); //waits for send to finish
		gPktsSent += 1;
		GetSystemTime(&endTime);
		Result = WSAGetOverlappedResult(*s, &ov, &BytesTransferred, FALSE, &Flags);
		WSAResetEvent(EventArray[0]); //reset event
		ZeroMemory(&ov, sizeof(WSAOVERLAPPED)); //reset overlapped structure
		ov.hEvent = EventArray[0];
		
		gBytesSent += SendBytes;
		/*if (!started) {
			SetEvent(songEvent);
			started = true;
		}*/
	}

	/*GetSystemTime(&startTime);
	while(getline(sendFile, line)) {
		line += "\n";
		//setting send buffer
		buffer.len = line.size();
		buffer.buf = (CHAR*)line.c_str();
		if (gTCP) {
			WSASend(*s, &buffer, 1, &SendBytes, 0, &ov, NULL); //sends TCP packets
		} else {
			WSASendTo(*s, &buffer, 1, &SendBytes, 0, (const sockaddr *)gaddr, sizeof((*gaddr)), &ov, 0); //sends UDP packets
		}
		WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE); //waits for send to finish
		gPktsSent += 1;
		GetSystemTime(&endTime);
		Result = WSAGetOverlappedResult(*s, &ov, &BytesTransferred, FALSE, &Flags);
		WSAResetEvent(EventArray[0]); //reset event
		ZeroMemory(&ov, sizeof(WSAOVERLAPPED)); //reset overlapped structure
		ov.hEvent = EventArray[0];
		
		gBytesSent += SendBytes;
		InvalidateRect(sData->hwnd, NULL, TRUE);
	}*/

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

/*DWORD WINAPI ReadSongThread(LPVOID n) {
	struct SongData * songdata = (struct SongData *)n;
	char streamDataBuffer[2048];
	HSTREAM streamBuffer = BASS_StreamCreateFile(FALSE, "A Proper Story.mp3", 0, 0, BASS_STREAM_DECODE);
	bool started = false;

	HANDLE songEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SONGSTARTED"));

	while(BASS_ChannelIsActive(streamBuffer)) {
		DWORD readLength = BASS_ChannelGetData(streamBuffer, streamDataBuffer, 2048);
		BASS_StreamPutData(*(songdata->songHandle), streamDataBuffer, readLength);
		if (!started) {
			SetEvent(songEvent);
			started = true;
		}
	}

	return 0;
}*/

DWORD WINAPI PlaySongThread(LPVOID n) {
	struct SongData * songdata = (struct SongData *)n;
	char streamDataBuffer[2048];
	//HSTREAM streamBuffer = BASS_StreamCreateFile(FALSE, "A Proper Story.mp3", 0, 0, BASS_STREAM_DECODE);
	bool started = false;

	HANDLE songEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SONGSTARTED"));

	WaitForSingleObject(songEvent, INFINITE);

	/*while(BASS_ChannelIsActive(*(songdata->songHandle))) {
		//BASS_ChannelPlay(*(songdata->songHandle), FALSE);
	}*/

	/*while(BASS_ChannelIsActive(streamBuffer)) {
		DWORD readLength = BASS_ChannelGetData(streamBuffer, streamDataBuffer, 2048);
		BASS_StreamPutData(*(songdata->songHandle), streamDataBuffer, readLength);
		if (!started) {
			SetEvent(songEvent);
			started = true;
		}
	}*/

	return 0;
}