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

		 /* 
		 * Constructor
		 @note: maxCopiesLocal - maximum quantity of clients launched on local computer.
		 @note: maxCopiesNet - maximum quantity of clients launched on net.
		 */
		SingleLaunch_Base(unsigned int maxCopiesLocal, unsigned int maxCopiesNet);


		static void EndSession();

		virtual	~SingleLaunch_Base();

	protected:
		 int MaxCopiesLocal;
		 int MaxCopiesNet;

	private:
		static sockaddr_in InitWinSocket(SOCKET & out_socket, int porttoConnect);
		static int mBindSocket(SOCKET sock, sockaddr_in addr, const int port);

		static void ThteadClientLis(SOCKET sock, sockaddr_in addr, const int portID);
		static void ThreadPingMsg(SOCKET sock, sockaddr_in addr, const int portID);

		static void ThteadServerLis(SOCKET sock, sockaddr_in addr, const int portID);

		static int CountClients();
		SingleLaunch_Base();
	};
}

