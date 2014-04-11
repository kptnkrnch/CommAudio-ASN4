//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by ASN02.rc
//
#include "windows.h"

#define IDD_PORTNPROT                   101 //Protocol and port dialog
#define IDD_IPCONNECT                   102 //IP dialog
#define IDD_SENDTEST                    104 //Send specified packets dialog
#define IDD_SENDFILE                    105 //Send specified file dialog
#define IDD_DIALOGLB                    106 //Send specified file dialog
#define IDC_LIST1                       1001
#define IDC_COMBO1                      1002
#define IDC_PORT						1004 //port edit box
#define IDC_TCP							1005 //TCP radio button
#define IDC_UDP							1006 //UDP radio button
#define IDC_MFCEDITBROWSE1              1007
#define IDC_PACKETSIZE                  1008 //packet size edit box
#define IDC_BUTTON1                     1009 
#define IDC_IP							1010 //ip edit box
#define IDC_NUMPACKETS					1011 //num packets edit box
#define IDC_FILENAME					1012 //file name edit box
#define IDC_OPENFILE					1013 //open file button
#define IDM_PROTPORT					10001 //protocol and port menu button
#define IDM_CONNECT						10002 //IP connect menu button
#define IDM_SENDTEST					10003 //send specified packets menu button
#define IDM_SENDFILE					10004 //send file menu button
#define IDM_CLIENT						10005 //set client menu button
#define IDM_SERVER						10006 //set server menu button
#define IDM_DISCONNECT					10007 //disconnect as server or client menu button
#define IDM_STARTSERVER					10008 //start the server listener menu button
#define IDM_OPENFILE					10008 //menu button for opening send file dialog
#define IDM_SENDPACKETS					10009 //menu button for opening send packet dialog
#define IDR_CLIENT_MC_ON				10010 
#define IDR_SERVER_MC_ON				10011
#define IDR_MAIN						10012 
#define CREATE_CLIENT					10013 //unused
#define ESTABLISH_CONNECT				10014 //message to initiate a connection
#define START_SERVER					10015 //message to start the server
#define IDM_SENDFILEDATA				10016 //message to start send file thread
#define IDM_LISTBOX						10017
#define IDM_MULTICAST_ON				10018
#define IDM_MULTICAST_OFF				10019
#define IDR_CLIENT_MC_OFF				10020 
#define IDR_SERVER_MC_OFF				10021

#define BTN_PLAY						2001 //Play button
#define BTN_PAUSE						2002 //Pause button
#define BTN_REFRESH						2003 //Pause button
#define BTN_STREAM						2004 //Pause button
#define BTN_LOAD						2005 //Load a local song on client
#define BTN_START_MIC					2006 //Starts MIC recording
#define BTN_STOP_MIC					2007 //Stops MIC recording

/*dialog box procedure prototypes*/
BOOL CALLBACK ProtocolAndPort(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK IPConnect(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SendTestPackets(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SendFile(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);


// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        106
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1010
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
