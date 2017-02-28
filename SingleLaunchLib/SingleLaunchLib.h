#pragma once
#include <wtypes.h>
#include <winsock.h>
#include <vector>
#include <string>
#include <cstring>

using namespace std;


namespace SingleLaunch
{
	class SingleLaunch_Base
	{
	public:
		SingleLaunch_Base(unsigned int maxCopies);

		static sockaddr_in InitWinSocket(SOCKET & out_socket, int porttoConnect);
		static int mBindSocket(SOCKET sock, sockaddr_in addr, const int port);

		static void ThteadClientLis(SOCKET sock, sockaddr_in addr, const int portID);

		static void ThteadServerLis(SOCKET sock, sockaddr_in addr, const int portID);
		static int CountClients();

		static void EndSession();

		virtual	~SingleLaunch_Base();
	protected:

		int MaxCopies;
		
		/*std::vector<string> netClients;
		std::vector<string> localClients;
		SOCKET my_sock;
		sockaddr_in in_addr;
	
		HANDLE mut;
		HANDLE myHandle;
		char buff[2048];
		int counter = 1;
		string hostName;
		string senderName;*/
	};
}

