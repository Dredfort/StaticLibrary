
#include "stdafx.h"
#include "SingleLaunchLib.h"


#include <stdexcept>
#include <handleapi.h>
//#include <ostream>
#include <iostream>
#include <wtypes.h>
#include <winsock.h>
#include <vector>
#include <string>
#include <thread>

using namespace std;

static const char SERVERADDR[16] = "255.255.255.255";
static const int portServer = 5150;
static const int portClient = 5250;

std::string pingString("ping");

std::string SerserverIpAddr;
std::string ServerPortAddr;

unsigned int CopiesTreshold;

#define FOREVER() for(;;)

bool bAcceptMessages = true;

std::vector<string> netClients;
std::vector<string> localClients;
std::multimap<string, string> ipPortMap;
static SOCKET server_sock, client_sock;

sockaddr_in out_addr;
sockaddr_in client_in_addr;

sockaddr_in out_addr2;
sockaddr_in client_in_addr2;

std::string localPort;

int serverBind = -1;

HANDLE mut;

HANDLE hMutex;

HANDLE myHandle;
char buff[2048];
unsigned int counter = 0;
string hostName;
string senderName;
string IP;

namespace SingleLaunch
{

	SingleLaunch_Base::SingleLaunch_Base(unsigned int maxCopiesLocal, unsigned int maxCopiesNet)
		: MaxCopiesLocal(maxCopiesLocal), MaxCopiesNet(maxCopiesNet)
	{
		if (MaxCopiesLocal == 1)
		{
			mut = CreateMutex(nullptr, false, TEXT("TestMutex"));
			DWORD result;
			result = WaitForSingleObject(mut, 0);
			if (result == WAIT_OBJECT_0)
			{
				cout << "Start run" << endl;
				ReleaseMutex(mut);
			}
			else
				cout << "fail. program is tunning" << endl;
		}

		CopiesTreshold = MaxCopiesLocal + MaxCopiesNet;

		cout << "Program is started..." << endl;
		cout << "Maximum local copies is - " << MaxCopiesLocal << endl;
		cout << "Maximum net copies is - " << MaxCopiesNet << endl;


		// Inti WinSock
		out_addr2 = InitWinSocket(client_sock, portClient);
		int clientBind = mBindSocket(client_sock, out_addr2, portClient);

		// Get local host name.
		char szHostName[128] = "";
		if (gethostname(szHostName, sizeof(szHostName)))
		{
			// Error handling
			printf("Get host error: %d\n", WSAGetLastError());
		}
		struct hostent* pHost = 0;
		pHost = gethostbyname(szHostName);
		hostName = szHostName;

		hMutex = CreateMutex(NULL, FALSE, NULL);

		// make thread.
		std::thread clientThr(ThteadClientLis, client_sock, out_addr2, portClient);
		clientThr.detach();

		if (clientBind == 0)
		{
			out_addr = InitWinSocket(server_sock, portServer);
			serverBind = mBindSocket(server_sock, out_addr, portServer);

			if (serverBind == 0)
			{
				std::thread  lisentThread(ThteadServerLis, server_sock, out_addr, portServer);
				lisentThread.detach();
			}
			else
				closesocket(server_sock);
		}

		/*std::thread pingThread(ThreadPingMsg, client_sock, out_addr2, portClient);
		pingThread.detach();*/
	}

	sockaddr_in SingleLaunch_Base::InitWinSocket(SOCKET & out_socket, int porttoConnect)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(porttoConnect);

		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;

		int err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
			printf("WSAStartup error: %d\n", WSAGetLastError());
		else
			printf("WinSock initializing\n");


		out_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (out_socket == INVALID_SOCKET)
		{
			printf("Socket error: %d\n", WSAGetLastError());
			WSACleanup();
		}

		int broadcast = 1;
		if (setsockopt(out_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof broadcast) != 0)
			cout << "Fail\n";

		return addr;
	}

	int SingleLaunch_Base::mBindSocket(SOCKET sock, sockaddr_in addr, const int port)
	{
		int i = 0;
		if ((i = ::bind(sock, (sockaddr *)&addr, sizeof(addr))) == SOCKET_ERROR)
		{
			printf("\nbind error: %d\n", WSAGetLastError());
		}
		else
		{
			cout << "\n++++++++++++++++++++++++++" << endl;
			cout << "Socket successfully binded!" << endl;
			cout << "++++++++++++++++++++++++++" << endl;
		}
		return i;
	}

	void SingleLaunch_Base::ThreadPingMsg(SOCKET sock, sockaddr_in addr, const int portID)
	{
		FOREVER()
		{
			Sleep(5000);

			char msg[20] = "__ping";
			sockaddr_in send_addr;
			send_addr.sin_family = AF_INET;
			send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			send_addr.sin_port = htons(portServer);
			send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

			sendto(sock, &msg[0], sizeof(msg), 0, (sockaddr*)&send_addr, sizeof(send_addr));
		}
	}

	void SingleLaunch::SingleLaunch_Base::ThteadClientLis(SOCKET sock, sockaddr_in addr, const int portID)
	{
		WaitForSingleObject(hMutex, INFINITE);

		cout << "\n-----------------------------" << endl;
		cout << "C:==> Try to ping the server." << endl;
		cout << "-----------------------------" << endl;

		char msg[20] = "__ping";
		sockaddr_in send_addr;
		send_addr.sin_family = AF_INET;
		send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		send_addr.sin_port = htons(portServer);
		send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

		sendto(sock, &msg[0], sizeof(msg), 0, (sockaddr*)&send_addr, sizeof(send_addr));

		cout << "\nC:> Start lisent port." << endl;

		FOREVER()
		{
			if (!bAcceptMessages)
				return;

			cout << "\n..." << endl;

			int client_addr_size = sizeof(client_in_addr2);
			int buffSize = sizeof(buff);

			ReleaseMutex(hMutex);

			int bsize = recvfrom(sock, &buff[0], buffSize - 1, 0, (sockaddr *)&client_in_addr2, &client_addr_size);
			if (bsize == SOCKET_ERROR)
			{
				printf("\nC:> recvfrom() error: %d\n", WSAGetLastError());

				closesocket(sock);
				ReleaseMutex(hMutex);
				ExitThread(0);
			}
			else
			{
				std::string incomingMsg(buff);


				if (serverBind == SOCKET_ERROR)
				{
					cout << "\n++++++++++++++++++++++++++" << endl;
					cout << "C:> incoming: " << endl;
					cout << "C:> port " << ntohs(client_in_addr2.sin_port) << endl;
					cout << "C:> message: " << incomingMsg/*&buff[0]*/ << endl;
					cout << "++++++++++++++++++++++++++" << endl;
				}

				// TODO:: get and accept messages here
				std::string buffStr(buff);

				std::string pingClientsStr("ping_clients");
				std::string closeCommandStr("close_command");
				std::string closeStr("closed_server");
				std::string bindIpSrt("bindip");

				size_t	findClose = buffStr.find(closeStr);
				size_t findCloseCommand = buffStr.find(closeCommandStr);
				size_t findPingClients = buffStr.find(pingClientsStr);
				size_t findBindIp = buffStr.find(bindIpSrt);

				std::string qwe = std::to_string(portClient);

				size_t pos = buffStr.find(qwe);

				if (findBindIp != string::npos)
				{
					SerserverIpAddr = inet_ntoa(client_in_addr2.sin_addr);
					ServerPortAddr = std::to_string(ntohs(client_in_addr2.sin_port));
				}

				if (findPingClients != string::npos)
				{
					cout << "\n-------------------------------------" << endl;
					cout << "C:==> Send `ping` response to server." << endl;
					cout << "C:> ip:[" << SerserverIpAddr << "]" << " port:[" << ServerPortAddr << "]" << endl;
					cout << "-------------------------------------" << endl;
					sendto(sock, pingString.c_str(), pingString.length(), 0, (sockaddr*)&send_addr, sizeof(send_addr));
				}
				else if (findCloseCommand != string::npos)
				{
					std::string check;
					check.append(buffStr.substr(findCloseCommand + 1));
					// TODO:: Send message to the client to close it.
					cout << "\n You launch maximum copies count! - " << check << endl;
					cout << " Q - to end session. " << endl;
					char myChar = ' ';
					while (myChar != 'q') {

						myChar = getchar();
					}

					std::cout << ntohs(client_in_addr.sin_port) << endl;

					std::string s = "closed";
					s.append("_");
					s.append(std::to_string(ntohs(client_in_addr.sin_port)));

					const size_t len = s.size();
					const char *pchar = s.c_str();

					sendto(sock/*server_sock*/, &pchar[0], len
						, 0, (sockaddr*)&send_addr, sizeof(send_addr));

					return;
				}
				else if (findClose != string::npos)
				{
					WaitForSingleObject(hMutex, INFINITE);

					cout << "\n--------------------------------------------------" << endl;
					cout << "C:==> Server is exit. Try to create server thread." << endl;
					cout << "--------------------------------------------------" << endl;
					Sleep(3000);


					out_addr = InitWinSocket(server_sock, portServer);
					serverBind = mBindSocket(server_sock, out_addr, portServer);

					ReleaseMutex(hMutex);

					if (serverBind == 0)
					{
						cout << "\nC:> Starts to create server thread..." << endl;
						std::thread  lisentThread(ThteadServerLis, server_sock, out_addr, portServer);
						lisentThread.detach();
					}
					else
					{
						closesocket(server_sock);
						cout << "\n-----------------------------------" << endl;
						cout << "\nC:==> fail create server thread.." << endl;
						cout << "-----------------------------------" << endl;
					}

				}
				else if (pos == string::npos /*&& localPort.length() == 0*/)
				{
					localPort = buffStr;
				}
			}
		}

	}

	void SingleLaunch::SingleLaunch_Base::ThteadServerLis(SOCKET sock, sockaddr_in addr, const int port)
	{

		cout << "\n+++++++++++++++++++++++++++++++++++++++" << endl;
		cout << "S:==> ServerCreated! <== \n S:>Send ping request to all clients." << endl;
		cout << "++++++++++++++++++++++++++++++++++++++" << endl;

		char msgpingClients[20] = "ping_clients";
		sockaddr_in send_addr;
		send_addr.sin_family = AF_INET;
		send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

		// To detect duplicate of server.
		char msgpingSrv[20] = "twinsfind";
		send_addr.sin_port = htons(port);
		sendto(server_sock, &msgpingSrv[0], sizeof(msgpingSrv)
			, 0, (sockaddr*)&send_addr, sizeof(send_addr));

		Sleep(1000);

		cout << "S:==> Start picking clients." << endl;
				
		for (auto i = 49152; i <= 65535; i++)
		{
			send_addr.sin_port = htons(i);

			sendto(sock, &msgpingClients[0], sizeof(msgpingClients)
				, 0, (sockaddr*)&send_addr, sizeof(send_addr));
		}

		cout << "\nS:> End picking clients." << endl;
		cout << "\nS:> Start lisent port." << endl;

		FOREVER()
		{
			WaitForSingleObject(hMutex, INFINITE);

			cout << "\n..." << endl;
			//Sleep(1000);

			if (!bAcceptMessages)
				return;
			// Обработка присланных пактов			
			int client_addr_size = sizeof(client_in_addr);
			int buffSize = sizeof(buff);
			int bsize = recvfrom(sock, &buff[0], buffSize - 1, 0, (sockaddr *)&client_in_addr, &client_addr_size);

			if (bsize == SOCKET_ERROR)
			{
				printf("\nS:>recvfrom() error: %d\n", WSAGetLastError());
				closesocket(sock);
				ReleaseMutex(hMutex);
				ExitThread(0);
			}
			else
				if (bAcceptMessages)
				{
					// Определяем IP-адрес клиента и прочие атрибуты
					HOSTENT *hostent;
					hostent = gethostbyaddr((char *)&client_in_addr.sin_addr, 4, AF_INET);

					IP = inet_ntoa(client_in_addr.sin_addr);
					

					if (hostent != nullptr)
						senderName = hostent->h_name;

					buff[bsize] = '\0';

					std::string incomingMsg(buff);
					std::string extPort = std::to_string(ntohs(client_in_addr.sin_port));

					std::string twinServer("twinsfind");
					std::string twinClose("twinsclose");
					//std::string pingString("ping");
					std::string closeStr("closed_client");

					size_t findServerTwins = incomingMsg.find(twinServer);
					size_t findServerClose = incomingMsg.find(twinClose);
					size_t findPingStr = incomingMsg.find(pingString);
					size_t findClosed = incomingMsg.find(closeStr);

					int serverPortMatch = std::strcmp(extPort.c_str(), std::to_string(portServer).c_str());


					bool matchNet = false;
					for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
					{
						if (*it == IP)
						{
							matchNet = true;
							break;
						}
					}
					bool matchLocal = false;
					for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
					{
						if (*it == extPort)
						{
							matchLocal = true;
							break;
						}
					}

					if (findClosed != string::npos)
						{
							if (matchNet)
							{
								cout << "\n-----------------" << endl;
								cout << "S:>net client exit" << endl;
								cout << "------------------" << endl;

								size_t result = incomingMsg.find("_");

								if (result != string::npos)
								{
									std::string check;
									check.append(incomingMsg.substr(result + 1));

									size_t elemId = 0;
									for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
									{
										if (*it == IP)
										{
											break;
										}
										elemId++;
									}
									if (elemId < netClients.size())
									{
										netClients.erase(netClients.begin() + elemId);
										cout << "S:> Net client removed from the list!" << endl;
									}
								}
								else
									cout << "fail to remove net player from arr!" << endl;
							}
							else if (matchLocal)
							{
								cout << "\n-----------------------" << endl;
								cout << "S:==> local client exit" << endl;
								cout << "S:> port " << ntohs(client_in_addr.sin_port) << endl;
								cout << "-----------------------" << endl;

								//  remove target client.
								size_t result = incomingMsg.find("_");
								if (result != string::npos)
								{
									std::string check;
									check.append(incomingMsg.substr(result + 1));

									size_t elemId = 0;
									for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
									{
										if (*it == extPort)
										{
											break;
										}
										elemId++;
									}
									if (elemId < localClients.size())
									{
										localClients.erase(localClients.begin() + elemId); // check the index.
										cout << "S:> Local client removed from the list!" << endl;
									}
								}
								else
									cout << "S:>fail to remove local player from list!" << endl;
							}

							cout << "\nS:> Refresh clients" << endl;
							// refresh clients
							char msgpingClients[20] = "ping_clients";
							sockaddr_in send_addr;
							send_addr.sin_family = AF_INET;
							send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
							send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
							for (auto i = 49152; i <= 65535; i++)
							{
								send_addr.sin_port = htons(i);

								sendto(sock, &msgpingClients[0], sizeof(msgpingClients)
									, 0, (sockaddr*)&send_addr, sizeof(send_addr));
							}
						}

					if (findServerTwins != std::string::npos && senderName != hostName)
					{
						cout << "\n----------------------------------" << endl;
						cout << "S:>Server in net detected." << endl;
						cout << "S:>Sending close message." << endl;
						cout << "-----------------------------------" << endl;

						const size_t len = twinClose.size();
						char const *pchar = twinClose.c_str();

						sendto(sock, pchar, len, 0,
							(sockaddr*)&client_in_addr
							, sizeof(client_in_addr));



						cout << "\nS:> Refresh clients" << endl;
						// refresh clients
						char msgpingClients[20] = "ping_clients";
						sockaddr_in send_addr;
						send_addr.sin_family = AF_INET;
						send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
						send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

						// bind ip.
						addr.sin_port = client_in_addr.sin_port;
						addr.sin_addr.s_addr = inet_addr(SERVERADDR);
						std::string stringBindIp = "_bindip";
						const size_t lenstringBindIp = stringBindIp.size();
						const char *pcharIP = stringBindIp.c_str();

						for (auto i = 49152; i <= 65535; i++)
						{
							send_addr.sin_port = htons(i);

							sendto(sock, &msgpingClients[0], sizeof(msgpingClients)
								, 0, (sockaddr*)&send_addr, sizeof(send_addr));
						
							sendto(sock, pcharIP, lenstringBindIp, 0,
							(sockaddr*)&client_in_addr,
							sizeof(client_in_addr));
						}


					}
					else if (findServerClose != std::string::npos && senderName != hostName)
					{
						cout << "\n----------------------------------" << endl;
						cout << "S:>Server in net detected." << endl;
						cout << "S:>Start closing server and thread." << endl;
						cout << "-----------------------------------" << endl;
						
						serverBind = -1;
						closesocket(server_sock);
						ReleaseMutex(hMutex);
						ExitThread(0);
					}


					//// NET PORTS.
					if (senderName != hostName && 
						(matchNet == false))
					{

						if (findPingStr != std::string::npos)
						{

							// Send message to the client to close it.
							if (localClients.size() + netClients.size() > CopiesTreshold)
							{
								cout << "\nS:==> launched [" << counter << "] copies of application with limit [" << CopiesTreshold << "]" << endl;

								addr.sin_port = client_in_addr.sin_port;
								addr.sin_addr.s_addr = inet_addr(SERVERADDR);

								std::string stringMsg = "close_command";
								stringMsg.append("_");
								stringMsg.append(std::to_string(counter));

								const size_t len = stringMsg.size();
								const char *pchar = stringMsg.c_str();

								sendto(sock, pchar, len, 0,
									(sockaddr*)&client_in_addr,
									sizeof(client_in_addr));
							}
							else
							{

								netClients.push_back(IP);

								cout << "\n==========================" << endl;
								cout << "S:==>Net client launched" << endl;
								cout << "S:> ip " << IP << " host:" << senderName << endl;
								cout << "==========================" << endl;

								// bind ip.
								addr.sin_port = client_in_addr.sin_port;
								addr.sin_addr.s_addr = inet_addr(SERVERADDR);

								std::string stringBindIp = "_bindip";
								const size_t len = stringBindIp.size();
								const char *pcharIP = stringBindIp.c_str();

								sendto(sock, pcharIP, len, 0,
									(sockaddr*)&client_in_addr,
									sizeof(client_in_addr));

								for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
								{
									std::string ipStr = *it;
									int intVal = std::stoi(*it);
									//if (intStr == IP)
									{
										//std::string s = std::to_string(std::stoi(*it));
										//s = s + "\n";
										//const size_t len = s.size();
										//char const *pchar = s.c_str();
										//sendto(sock, &pchar[0], len/*sizeof(pchar)*/, 0, (sockaddr *)&client_in_addr, sizeof(client_in_addr));
										addr.sin_port = htons(port);
										addr.sin_addr.s_addr = inet_addr(ipStr.c_str());

										std::string s = std::to_string(localClients.size() + netClients.size());
										const size_t len = s.size();
										char const *pchar = s.c_str();

										sendto(sock, &pchar[0], len, 0, (sockaddr*)&addr, sizeof(addr));
									}
								}
							}
						}



						CountClients();
					}
					//// LOCAL PORTS.
					else
					{						
						if (matchLocal == false && findPingStr != string::npos)// (match == false && std::strcmp(buff, "ping") == 0)
						{
							//counter++;

							// Send message to the client to close it.
							if (localClients.size() + netClients.size() > CopiesTreshold)
							{
								cout << "\nS:==> launched [" << counter << "] copies of application with limit [" << CopiesTreshold << "]" << endl;

								addr.sin_port = client_in_addr.sin_port;
								addr.sin_addr.s_addr = inet_addr(SERVERADDR);

								std::string stringMsg = "close_command";
								stringMsg.append("_");
								stringMsg.append(std::to_string(counter));

								const size_t len = stringMsg.size();
								const char *pchar = stringMsg.c_str();

								sendto(sock, pchar, len, 0,
									(sockaddr*)&client_in_addr/*out_addr*/,
									sizeof(client_in_addr/*out_addr*/));
							}
							else
							{
								localClients.push_back(extPort);//ip

								cout << "\n==========================" << endl;
								cout << "S:==>local client launched" << endl;
								cout << "S:> port " << ntohs(client_in_addr.sin_port) << endl;
								cout << "==========================" << endl;

								// bind ip.
								addr.sin_port = client_in_addr.sin_port;
								addr.sin_addr.s_addr = inet_addr(SERVERADDR);

								std::string stringBindIp = "_bindip";								
								const size_t len = stringBindIp.size();
								const char *pcharIP = stringBindIp.c_str();

								sendto(sock, pcharIP, len, 0,
									(sockaddr*)&client_in_addr,
									sizeof(client_in_addr));


								// send for all locals.
								for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
								{
									int intStr = std::stoi(*it);
									if (intStr != port)
									{
										addr.sin_port = htons(intStr);
										addr.sin_addr.s_addr = inet_addr(SERVERADDR);

										std::string s = std::to_string(localClients.size() + netClients.size());
										const size_t len = s.size();
										char const *pchar = s.c_str();

										sendto(sock, &pchar[0], len, 0, (sockaddr*)&addr, sizeof(addr));
									}
								}
							}

							CountClients();
						}
					}
				}

			ReleaseMutex(hMutex);
		}
	}

	int SingleLaunch::SingleLaunch_Base::CountClients()
	{
		//counter = netClients.size() + localClients.size();
		cout << "=> Copies:" << "[local: " << localClients.size() << " ] " << "[net: " << netClients.size() << " ]" << endl;
		return counter + netClients.size();
	}

	void SingleLaunch::SingleLaunch_Base::EndSession()
	{
		bAcceptMessages = false;


		if (serverBind == SOCKET_ERROR)// client closed.
		{
			std::string s = "closed_client";
			s.append("_");
			s.append(localPort);
			const size_t len = s.size();
			const char *pchar = s.c_str();

			sockaddr_in send_addr;
			send_addr.sin_family = AF_INET;

			if (ServerPortAddr.length() > 0)
			{
				send_addr.sin_addr.s_addr = inet_addr(SerserverIpAddr.c_str());
				int ip = std::stoi(ServerPortAddr);
				send_addr.sin_port = htons(ip);
			}
			else
			{
				send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
				send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
				send_addr.sin_port = htons(portServer);
			}

			sendto(client_sock, &pchar[0], len
				, 0, (sockaddr*)&send_addr, sizeof(send_addr));			
		}
		else // server closed.
		{
			std::string s = "closed_server";
			s.append("_");
			s.append(localPort);
			const size_t len = s.size();
			const char *pchar = s.c_str();

			sockaddr_in send_addr;
			send_addr.sin_family = AF_INET;
			send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			send_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

			for (auto i = 49152; i <= 65535; i++)
			{
				send_addr.sin_port = htons(i);

				sendto(server_sock, &pchar[0], len, 0, (sockaddr*)&send_addr, sizeof(send_addr));
			}
		}

		// TODo:: send msg to all clients that server is closed.

		// local
		// for (vector<string>::iterator it = localClients.begin(); it != localClients.end(); ++it)
		// {
			// int intStr = std::stoi(*it);
			// if (intStr != portServer)
			// {
				// std::cout << ' ' << intStr << endl;
				// std::cout << ntohs(client_in_addr.sin_port) << endl;

				// out_addr.sin_port = htons(intStr);
				// out_addr.sin_addr.s_addr = inet_addr(SERVERADDR);

				// sendto(server_sock, &pchar[0], len
					// , 0, (sockaddr*)&out_addr, sizeof(out_addr));
			// }
			// //Sleep(1000);
		// }

		// net
		/*for (vector<string>::iterator it = netClients.begin(); it != netClients.end(); ++it)
		{
			std::string intStr = *it;
			if (intStr != IP)
			{
				std::string s = "close";
				s.append("_");
				s.append(intStr);

				const size_t len = s.size();
				char const *pchar = intStr.c_str();

				out_addr.sin_port = htons(portServer);
				out_addr.sin_addr.s_addr = inet_addr(pchar);
				sendto(server_sock, &pchar[0], len, 0, (sockaddr*)&out_addr, sizeof(out_addr));
			}
		}*/

		closesocket(client_sock);
		closesocket(server_sock);
		Sleep(1000);
		WSACleanup();
		//closesocket(server_sock);
	}

	SingleLaunch_Base::~SingleLaunch_Base()
	{
		closesocket(client_sock);
		closesocket(server_sock);

		cout << " --------------" << endl;
		cout << "> Close program <" << endl;
		cout << " --------------" << endl;

		if (server_sock != INVALID_SOCKET)
		{
			server_sock = INVALID_SOCKET;
		}
		if (client_sock != INVALID_SOCKET)
		{
			client_sock = INVALID_SOCKET;
		}

		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
		ReleaseMutex(mut);
		CloseHandle(mut);
	}
}