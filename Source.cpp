/*
  FlexDiscovery
   Len Koppl, KDØRC
   9/23/20
   Visual Studio 2019 C++ console
   Uses Flex API without .net or FlexLib (i.e. uses native text-based API commands)

   Utility to test Flex API commands and to show TCP/IP traffic from the radio.
   Displays the first Flex discovery UDP packet that it finds.  Other Flex UDP traffic is not read.

   The commented out commands are what I have been experimenting with.  
   There are many many more that can be added here.  Most are commented out so that
   you can focus on just what you are interested in.

   The main goal is to provide a way to try out commands before putting them into your
   program.  To get the proper parameter name (may vary slightly from the wiki...), 
   uncomment the appropriate subscription(s), run this program, then activate the control
   in SmartSDR, or on your Maestro.  The response from the radio will be displayed on 
   the console screen.  The parameters have good instruction set reciprocity,
   so this will lead you to the correct syntax.

   A good example is VOX.  The wiki says:
   C41|transmit set vox=1

   It doesn't work...

   Run this utility with "sub tx all" uncommented and turn VOX on and off.  The radio
   responds with:
	S1F5C6BC|transmit vox_enable=1 vox_level=68 vox_delay=16
	S1F5C6BC|transmit vox_enable=0 vox_level=68 vox_delay=16

   So now you can deduce that the actual command is: 
   C41|transmit set vox_enable=1

   Note that "s" and "set" are interchangable.  In my commands, I always use CD instead of C
   so that I get the verbose debug version of the radio responses.
*/

#include <iostream>
#include <string>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <conio.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Constants
const int BufLen = 1024;
const unsigned short port = 4992;

int main()
{
	string ipAddress;
	char RecvBuf[BufLen];
	int wsResult;

	// Initialize WinSock
	WSADATA data;
	SOCKET RecvSocket;
	struct sockaddr_in RecvAddr;
	struct sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	WORD ver = MAKEWORD(2, 2);
	wsResult = WSAStartup(ver, &data);

	if (wsResult != 0)
	{
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
		return 1;
	}

	// UDP: get IP and radio info
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	int set_option_on = 1;
	wsResult = setsockopt(RecvSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&set_option_on,
		sizeof(set_option_on));

	if (RecvSocket == INVALID_SOCKET) 
	{
		wprintf(L"socket failed with error %d\n", WSAGetLastError());
		return 1;
	}

	// Bind the socket to any address and the specified port.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(port);
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	wsResult = bind(RecvSocket, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
	if (wsResult != 0)
	{
		wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return 1;
	}

	cout << "Looking for a Flex on this network..." << endl;

	int errCount = 0;

	while (true)	// Wait for a Flex discovery packet to be broadcast
	{
		wsResult = recvfrom(RecvSocket,
			RecvBuf, BufLen, 0, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
		if (wsResult == SOCKET_ERROR)
		{
			wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
			errCount += 1;
			if (errCount > 10)
			{
				return 1;
			}
		}
		else if (wsResult > 0)
		{
			string strRecvBuf;
			char FlexIP[256];
			ZeroMemory(FlexIP, 256);
			inet_ntop(AF_INET, &SenderAddr.sin_addr, FlexIP, 256);

			for (int i = 0; i < 256; i++)
			{
				ipAddress += FlexIP[i];
				if (FlexIP[i] == 0x0)
				{
					break;
				}
			}
			
			for (int i = 28; i < BufLen; i++)	// bypass 28 bytes of header
			{
				strRecvBuf += RecvBuf[i];
				if (RecvBuf[i] == 0x0)
				{
					break;
				}
			}
			cout << "Message received from " << FlexIP << " : " << endl;
			cout << strRecvBuf << endl << endl;

			if (strRecvBuf.find("discovery_protocol_version=") >= 0)
			{
				cout << "Flex Discovery packet found" << endl << endl;
				break;
			}
		}
	}

	// TCP/IP communicate with radio
	DWORD iOptVal = 100;
	int iOptLen = sizeof(DWORD);

	// Create Socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &iOptVal, iOptLen);

	// Hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// connect to radio
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		cerr << "Can't connect to radio, Err #" << WSAGetLastError() << endl;
		closesocket(sock); WSACleanup();
		return 1;
	}

	// Data loop
	int sendResult;

	//
	sendResult = NULL;

	sendResult = send(sock, "CD0 | info\n", 12, 0);

	// Subscriptions
//	sendResult = send(sock, "CD1 | sub audio_stream all\n", 28, 0);
	sendResult = send(sock, "CD2 | sub client all\n", 22, 0);
//	sendResult = send(sock, "CD3 | sub cwx all\n", 19, 0);
//	sendResult = send(sock, "CD4 | sub dax all\n", 19, 0);
//	sendResult = send(sock, "CD5 | sub daxiq all\n", 21, 0);
//	sendResult = send(sock, "CD6 | sub foundation all\n", 26, 0);
//	sendResult = send(sock, "CD7 | sub memories all\n", 24, 0);
//	sendResult = send(sock, "CD8 | sub meter all\n", 21, 0);
	sendResult = send(sock, "CD9 | sub pan all\n", 19, 0);
//	sendResult = send(sock, "CD10 | sub radio added\n", 24, 0);
	sendResult = send(sock, "CD11 | sub radio all\n", 22, 0);
//	sendResult = send(sock, "CD12 | sub scu all\n", 20, 0);
	sendResult = send(sock, "CD13 | sub slice all\n", 22, 0);
//	sendResult = send(sock, "CD14 | sub spot all\n", 21, 0);
//	sendResult = send(sock, "CD15 | sub tnf all\n", 20, 0);
 	sendResult = send(sock, "CD16 | sub tx all\n", 19, 0);
//	sendResult = send(sock, "CD17 | sub usb_cable all\n", 26, 0);
//	sendResult = send(sock, "CD18 | sub xvtr all\n", 21, 0);

	// Audio Client
//	sendResult = send(sock, "CD30 | audio client 0 slice 1 mute 0\n", 38, 0);
//	sendResult = send(sock, "CD31 | audio client 0 slice 0 mute 1\n", 38, 0);
//	sendResult = send(sock, "CD32 | audio client 1234 slice 1 gain 0.2\n", 43, 0);
//	sendResult = send(sock, "CD33 | audio client 0 slice 0 gain 0.2 mute 0\n", 47, 0);

	// Client
//	sendResult = send(sock, "CD40 | client gui 1234\n", 24, 0);
//	sendResult = send(sock, "CD41 | client disconnect 0x43837CBE\n", 37, 0);
//	sendResult = send(sock, "CD42 | client gui client_id=E08F5BB7-F75E-4940-8352-624F0BC84106\n", 66, 0);
//	sendResult = send(sock, "CD43 | client bind client_id=E08F5BB7-F75E-4940-8352-624F0BC84106\n", 67, 0);
//                                                        4Y9fEIIs-VYyQ-o5u6-jr7y-BWc4lV5ugC2m

	// CW
//	sendResult = send(sock, "CD51 | cw key on time=0x003C index=0 client_handle=0x1AA8837D\n", 63, 0);
//	sendResult = send(sock, "CD52 | cw key off time=0x0078 index=1 client_handle=0x1AA8837D\n", 64, 0);
//	sendResult = send(sock, "CD53 | cw sidetone on\n", 23, 0);
//	sendResult = send(sock, "CD54 | cw break_in off\n", 24, 0);
//	sendResult = send(sock, "CD55 | cw wpm 15\n", 18, 0);

	// DAX IQ - I never got much of this to work
//	sendResult = send(sock, "CD60 |dax iq s 1 pan=0x40000000 daxiq_rate=24000 client_handle=\n", 75, 0);
//	sendResult = send(sock, "CD61 |dax iq s 1 pan=0x40000000 daxiq_rate=24000\n", 50, 0);
//	sendResult = send(sock, "CD62 |dax iq s 1 0 24\n", 23, 0);
//	sendResult = send(sock, "CD63 |dax iq s 1 0 24\n", 23, 0);

	// Display pan
//	sendResult = send(sock, "CD70 | display pan s 0x40000000 center=14.250\n", 47, 0);
//	sendResult = send(sock, "CD71 | display pan rfgain_info 0x40000000\n", 43, 0);
//	sendResult = send(sock, "CD72 | display pan s 0x40000000 auto_center=0\n", 47, 0);
//	sendResult = send(sock, "CD73 | display pan rfgain_info 0x40000000\n", 43, 0);
//	sendResult = send(sock, "CD74 | display pan s 0x40000000 band=80\n", 41, 0);
// 	sendResult = send(sock, "CD75 | display pan s 0x40000000 rxant=ANT2\n", 44, 0);
//	sendResult = send(sock, "CD76 | display pan s 0x40000000 tx_a_ant=ANT2\n", 47, 0);

	// Profile
//	sendResult = send(sock, "CD80 | profile global load \"KD0RC -  20 SSB\"\n", 46, 0);
//	sendResult = send(sock, "CD81 | profile global load \"x\"\n", 32, 0);
//	sendResult = send(sock, "CD82 | profile global info\n", 28, 0);
//	sendResult = send(sock, "CD83 | profile global load \"\"\n", 31, 0);

	// Ping
//	sendResult = send(sock, "C90 | ping\n", 12, 0);
//	sendResult = send(sock, "C91 | keepalive disable\n", 25, 0);

	// Slice
//	sendResult = send(sock, "CD100 | slice r 0\n", 19, 0);
//	sendResult = send(sock, "CD101 | slice r 1\n", 19, 0);
//	sendResult = send(sock, "CD102 | slice s 0 rit_on=1 rit_freq=40\n", 40, 0);
//	sendResult = send(sock, "CD103 | slice s 0 rit_freq=40\n", 31, 0);
//	sendResult = send(sock, "CD104 | slice list\n", 20, 0);
//	sendResult = send(sock, "CD105 | slice s 0 rxant=ANT2\n", 30, 0);
//	sendResult = send(sock, "CD106 | slice s 0 squelch=1 squelch_level=40\n", 46, 0);
//	sendResult = send(sock, "CD107 | slice s 0 tx=1\n", 24, 0);
//	sendResult = send(sock, "CD108 | slice t 0 14.2\n", 24, 0); // Kills Flex with a software error - FIXED!
//	sendResult = send(sock, "CD109 | slice s 0 active=1\n", 28, 0);
//	sendResult = send(sock, "CD110 | slice create freq=14.200 pan=0 ant=ANT1 mode=usb\n", 58, 0);

	// Spots
	
//	sendResult = send(sock, "CD200 | spot add rx_freq=146.700 callsign=W0DK_BARC_100\n", 57, 0);
//	sendResult = send(sock, "CD201 | spot add rx_freq=145.310 callsign=KB0VJJ_Conn_88.5\n", 60, 0);
//	sendResult = send(sock, "CD202 | spot add rx_freq=146.520 callsign=Simplex\n", 51, 0);
//	sendResult = send(sock, "CD203 | spot add rx_freq=144.200 callsign=144.200\n", 51, 0);
//	sendResult = send(sock, "CD204 | spot add rx_freq=145.340 callsign=N0PQV_Evgn_103.5\n", 60, 0);
//	sendResult = send(sock, "CD205 | spot add rx_freq=146.760 callsign=W0IA_BCARES_100\n", 59, 0);
//	sendResult = send(sock, "CD206 | spot add rx_freq=145.145 callsign=W0CRA_CRA_107.2\n", 59, 0);
//	sendResult = send(sock, "CD207 | spot add rx_freq=147.225 callsign=W0CRA_CRA_107.2\n", 59, 0);
//	sendResult = send(sock, "CD208 | spot add rx_freq=145.090 callsign=145.090_Packet\n", 58, 0);
//	sendResult = send(sock, "CD209 | spot add rx_freq=145.050 callsign=145.050_Packet\n", 58, 0);
//	sendResult = send(sock, "CD210 | spot add rx_freq=14.329 callsign=Sunday_Flex_Net\n", 58, 0);
//	sendResult = send(sock, "CD211 | spot add rx_freq=146.640 callsign=WA0KBT_DRL\n", 54, 0);
//	sendResult = send(sock, "CD212 | spot add rx_freq=146.940 callsign=W0WYZ_RMRL_103.5\n", 60, 0);
//	sendResult = send(sock, "CD213 | spot add rx_freq=146.910 callsign=W0JZ_BCARES_123\n", 59, 0);
//	sendResult = send(sock, "CD214 | spot add rx_freq=146.610 callsign=W0DK_BARC_100\n", 57, 0);
//	sendResult = send(sock, "CD215 | spot add rx_freq=7.220 callsign=KE5DTO mode=USB color=#FF5733 source=N1MM spotter_callsign=N5AC timestamp=1533196800 lifetime_seconds=3600 priority=4 comment=thanks trigger_action=tune\n", 194 ,0);
//	sendResult = send(sock, "CD216 | spot add rx_freq=14.328 callsign=**Out_Of_Band** color=#FFFFFFFF lifetime_seconds=30 priority=1 comment=Out_Of_Band\n", 125, 0);
	

	// Transmit
//	sendResult = send(sock, "CD300 | transmit tune on\n", 26, 0);
//	sendResult = send(sock, "CD301 | transmit tune off\n", 27, 0);
//	sendResult = send(sock, "CD302 | transmit s rfpower=26\n", 31, 0);
//	sendResult = send(sock, "CD303 | transmit s vox_enable=1\n", 33, 0);
//	sendResult = send(sock, "CD304 | transmit s tune_mode=single_tone\n", 42, 0);
//	sendResult = send(sock, "CD305 | transmit s tune_mode=two_tone\n", 39, 0);

	// Misc
//	sendResult = send(sock, "CD400 | message severity=warning \"Out of Band\"\n", 48, 0);
//	sendResult = send(sock, "CD401 | meter list\n", 20, 0);
//	sendResult = send(sock, "CD402 | memory apply 1\n", 24, 0);
//	sendResult = send(sock, "CD403 | filt 1 -200 200\n", 25, 0);

/*
	// Can't remember what this experiment was for...
	string MyHandle = "0x"; // fill this in based on a previous run to get the handle
	string cmd = "CD26 |dax iq s 1 pan=0x40000000 daxiq_rate=24000 client_handle=" + MyHandle + "\n";
	char cmdch[74];
	for (int x = 0; x < cmd.length(); x++)
	{
		cmdch[x] = cmd[x];
	}
	cout << string(cmdch, 0, 74) << endl;
	sendResult = send(sock, cmdch, 74, 0);
*/

	Sleep(100);

	// Receive and display data
	while (true)
	{
		if (sendResult != SOCKET_ERROR)
		{
			ZeroMemory(RecvBuf, BufLen);
			for (int i = 0; i < 100; i++)
			{
				int bytesReceived = recv(sock, RecvBuf, BufLen, MSG_PEEK);
				if (bytesReceived > 0)
				{
					int bytesReceived = recv(sock, RecvBuf, BufLen, 0);
					//cout << "bytesReceived: " << bytesReceived << endl;
					cout << string(RecvBuf, 0, bytesReceived) << endl;
				}
				else
				
				if(_kbhit())
				{
					cout << "Exiting..." << endl;
					closesocket(sock);
					WSACleanup();
					Sleep(5000);
					return 0;
				}
			}
		}
		else
		{
			cerr << "Socket error " << SOCKET_ERROR << endl;
			break;
		}
	}
	
	sendResult = send(sock, "CD1000 | client disconnect", 27, 0);
	cout << "Exiting..." << endl;
	closesocket(sock);
	WSACleanup();
	return 0;
}