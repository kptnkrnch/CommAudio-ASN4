#include "resource.h"
#include "windows.h"
#include <fstream>
#include <string>

extern std::string gIP;
extern bool gTCP;
extern bool gUDP;
extern int gPort;
extern int gNumPackets;
extern int gPacketSize;
extern std::string fileToSend;


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ProtocolAndPort
--
-- DATE: February 1, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- RETURNS: Boolean
--
-- NOTES:
-- Dialog box for setting the protocol and port to use.
----------------------------------------------------------------------------------------------------------------------*/
BOOL CALLBACK ProtocolAndPort(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	HWND tcp = 0, udp = 0;
	static bool lTCP, lUDP;
	int lPort = 5150;
	TCHAR port[50];
	char sPort[50];
	HWND parent;
    switch(Message)
    {
        case WM_INITDIALOG:
			tcp = GetDlgItem(hwnd, IDC_TCP);
			SendMessage(tcp, BM_SETCHECK, BST_CHECKED, 0);
			lTCP = true;
			lUDP = false;
        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDC_TCP:
					/* Handles turning TCP radio button on
					   and UDP radio button off*/
					tcp = GetDlgItem(hwnd, IDC_TCP);
					udp = GetDlgItem(hwnd, IDC_UDP);
					SendMessage(tcp, BM_SETCHECK, BST_CHECKED, 0);
					SendMessage(udp, BM_SETCHECK, BST_UNCHECKED, 0);
					lTCP = true;
					lUDP = false;
				break;
				case IDC_UDP:
					/* Handles turning TCP radio button off
					   and UDP radio button on*/
					udp = GetDlgItem(hwnd, IDC_UDP);
					tcp = GetDlgItem(hwnd, IDC_TCP);
					SendMessage(udp, BM_SETCHECK, BST_CHECKED, 0);
					SendMessage(tcp, BM_SETCHECK, BST_UNCHECKED, 0);
					lTCP = false;
					lUDP = true;
				break;
                case IDOK:
					/*storing radio button flags in globals and retrieving and storing port
					  info into a global*/
					gTCP = lTCP;
					gUDP = lUDP;
					GetDlgItemText(hwnd, IDC_PORT, port, 50);
					wcstombs(sPort, port, 50);
					gPort = atoi(sPort);
					parent = GetParent(hwnd);
					InvalidateRect(parent, NULL, TRUE);
                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: IPConnect
--
-- DATE: February 1, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- RETURNS: Boolean
--
-- NOTES:
-- Dialog box for entering an IP address and initializing a connection.
----------------------------------------------------------------------------------------------------------------------*/
BOOL CALLBACK IPConnect(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	HWND parent;
	TCHAR IP[50];
	char cIP[50];
    switch(Message)
    {
        case WM_INITDIALOG:

        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					parent = GetParent(hwnd); //gets parent window handle

					/*getting and converting the ip edit box to a usable format and then
					storing that data in a global*/
					GetDlgItemText(hwnd, IDC_IP, IP, 50);
					wcstombs(cIP, IP, 50);
					if (cIP != NULL) {
						gIP = cIP;
					}

					//sends a message to the parent to tell it to try and establish a TCP connection
					//or initiate a UDP transfer
					SendMessage(parent, WM_COMMAND, MAKEWPARAM(ESTABLISH_CONNECT, ESTABLISH_CONNECT), 0);
                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SendTestPackets
--
-- DATE: February 1, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- RETURNS: Boolean
--
-- NOTES:
-- Dialog box for entering the number of packets to send and the size of said packets.
-- Packets are sent via UDP or TCP.
----------------------------------------------------------------------------------------------------------------------*/
BOOL CALLBACK SendTestPackets(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	TCHAR numPackets[50];
	TCHAR packetSize[50];
	char cNumPackets[50];
	char cPacketSize[50];
	HWND parent;
    switch(Message)
    {
        case WM_INITDIALOG:
        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					/*converting edit boxes to a usable format and storing the data in global variables*/
					GetDlgItemText(hwnd, IDC_NUMPACKETS, numPackets, 50);
					GetDlgItemText(hwnd, IDC_PACKETSIZE, packetSize, 50);
					wcstombs(cNumPackets, numPackets, 50);
					wcstombs(cPacketSize, packetSize, 50);
					gNumPackets = atoi(cNumPackets);
					gPacketSize = atoi(cPacketSize);

					parent = GetParent(hwnd);//retrieves the parent window handle
					//sends a message to the parent to tell it to create the send packets thread 
					SendMessage(parent, WM_COMMAND, MAKEWPARAM(IDM_SENDPACKETS, IDM_SENDPACKETS), 0); 

                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SendFile
--
-- DATE: February 1, 2014
--
-- REVISIONS: none
--
-- DESIGNER: Joshua Campbell
--
-- PROGRAMMER: Joshua Campbell
--
-- RETURNS: Boolean
--
-- NOTES:
-- Dialog box for selecting a file to send via TCP or UDP to the server
----------------------------------------------------------------------------------------------------------------------*/
BOOL CALLBACK SendFile(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	std::fstream ifs;
	OPENFILENAME ofn;
	static TCHAR szFilter[] = TEXT ("All Files (*.*)\0*.*\0\0") ;
	static TCHAR szFileName[MAX_PATH], szTitleName[MAX_PATH] ;
	TCHAR fileName[MAX_PATH];
	char cFileName[MAX_PATH];
	HWND parent;
    switch(Message)
    {
        case WM_INITDIALOG:

        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					GetDlgItemText(hwnd, IDC_FILENAME, fileName, MAX_PATH);
					wcstombs(cFileName, fileName, MAX_PATH);
					fileToSend = cFileName;

					parent = GetParent(hwnd);

					SendMessage(parent, WM_COMMAND, MAKEWPARAM(IDM_SENDFILEDATA, IDM_SENDFILEDATA), 0);

                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
				case IDC_OPENFILE:
					ofn.lStructSize       = sizeof (OPENFILENAME) ;
					ofn.hwndOwner         = hwnd ;
					ofn.hInstance         = NULL ;
					ofn.lpstrFilter       = szFilter ;
					ofn.lpstrCustomFilter = NULL ;
					ofn.nMaxCustFilter    = 0 ;
					ofn.nFilterIndex      = 0 ;
					ofn.lpstrFile         = NULL ;          // Set in Open and Close functions
					ofn.nMaxFile          = MAX_PATH ;
					ofn.lpstrFileTitle    = NULL ;          // Set in Open and Close functions
					ofn.nMaxFileTitle     = MAX_PATH ;
					ofn.lpstrInitialDir   = NULL ;
					ofn.lpstrTitle        = NULL ;
					ofn.Flags             = 0 ;             // Set in Open and Close functions
					ofn.nFileOffset       = 0 ;
					ofn.nFileExtension    = 0 ;
					ofn.lpstrDefExt       = TEXT ("txt") ;
					ofn.lCustData         = 0L ;
					ofn.lpfnHook          = NULL ;
					ofn.lpTemplateName    = NULL ;

					ofn.hwndOwner         = hwnd ;
					ofn.lpstrFile         = szFileName ;
					ofn.lpstrFileTitle    = szTitleName ;
					ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;

					if (GetOpenFileName(&ofn)) {
						
						std::wstring wfilename(szFileName);
						std::string filename;
						HWND filenameBox;
						filenameBox = GetDlgItem(hwnd, IDC_FILENAME);

						SetWindowText(filenameBox, szFileName);

						for(int i = 0; i < wfilename.size(); i++) {
							filename += wfilename[i];
						}
					}
				break;
				case IDM_OPENFILE:
				break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}