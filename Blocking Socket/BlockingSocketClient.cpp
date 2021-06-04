#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일러에서 경고 및 오류 방지

#include <WinSock2.h>
#include <iostream>
#include <string>
#include <thread>
#pragma comment( lib, "ws2_32" )  // 윈속2 라이브러리

using namespace std;

const unsigned short    BUFSIZE = 512;
const unsigned short    PORT = 9000;
const std::string       IP = "127.0.0.1";
//const std::string       IP      = "본인 컴퓨터의 사설 IP";

// 전역
SOCKET server_sock;

thread ThreadForBroadcasting;

bool isRun = true;

// 소켓 함수 오류 출력
void err_display(const char* const cpcMSG)
{
	LPVOID lpMsgBuf = nullptr;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&lpMsgBuf),
		0,
		nullptr);

	printf_s("[%s] %s", cpcMSG, reinterpret_cast<LPSTR>(lpMsgBuf));

	LocalFree(lpMsgBuf);
}

/// 쓰레드로 전체 채팅 보여주기
void Thread_BroadCast()
{
	while (isRun)
	{
		// 에코
		char szBuffer[BUFSIZE] = { 0, };
		int iEcoRecvPacketSize = recv(server_sock, szBuffer, BUFSIZE, 0);
			
		string _tempStr(szBuffer);

		if (_tempStr.compare("exit")==0)	// 서버 종료시 나가기
		{
			printf_s("-> 서버가 종료됐습니다. 프로그램을 종료합니다.\n");
			closesocket(server_sock);
			server_sock = 0;
			isRun = false;
			return;
		}

		if (server_sock == INVALID_SOCKET || server_sock == SOCKET_ERROR) // 소켓 종료시 나가기
		{
			return;
		}

		if (iEcoRecvPacketSize != SOCKET_ERROR)
		{
			printf_s("\n[TCP 클라이언트] 패킷 수신 : %s( %d 바이트 )\n", szBuffer, iEcoRecvPacketSize);
		}
		
	}
}

///클라이언트 main//////////////////////////////////////////////////////////////////
int main(void)
{
	// 윈속 초기화( WinSock 2.2 )
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		err_display("WSAStartup()");
		return -1;
	}

	/// socket() : 소켓 생성 - listen() 담당 소켓 생성                                
	server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == server_sock)
	{
		err_display("socket()");
		WSACleanup();
		return -1;
	}

	/// connect() : IP 와 PORT 를 가지고 목적지에 연결을 시도
	// 서버 listen() 함수에 의한 연결 대기열에 들어가게 되면 성공 리턴
	// 실패시 ECONNREFUSED 에러를 받고 리턴
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(PORT);
	serveraddr.sin_addr.s_addr = inet_addr(IP.c_str());
	if (SOCKET_ERROR == connect(server_sock, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
	{
		err_display("connect()");
		closesocket(server_sock);
		server_sock = 0;
		WSACleanup();
		return -1;
	}

	printf_s("[TCP 클라이언트] connect() 성공\n");

	printf_s("============================\n");
	printf_s("프로그램 종료 키워드 -> exit\n");
	printf_s("============================\n");

	/// 작업중
	ThreadForBroadcasting = thread(Thread_BroadCast);

	// 서버와 통신
	while (isRun)
	{
		if (isRun == false)
		{
			break;
		}

		printf_s("전송할 문자열 입력 : ");

		// 사용자 입력 받기
		std::string strBuffer;
		std::getline(std::cin, strBuffer);
		size_t nLength = strBuffer.size();

		if (0 == nLength) // 입력된 문자열이 없음
		{
			printf_s("-> 입력된 문자열이 없습니다.\n");
			continue;
		}
		else if (256 < nLength) // 최대 입력 문자열은 256 바이트로 제한
		{
			printf_s("-> 최대 문자열 바이트를 초과하였습니다.\n");
			continue;
		}
		else if (strBuffer.compare("exit") == 0) // 종료 키워드 "exit"
		{
			printf_s("-> 프로그램을 종료합니다.\n");
			closesocket(server_sock);
			server_sock = 0;
			isRun = false;

			break;
		}

		/// send() : 소켓에 패킷 송신( 커널단에 있는 소켓의 SendBuffer에 대해서 공부 필요 )
		// 1. 소켓이 블럭킹 소켓이라도 패킷을 전송할때까지 무한정 대기하지 않는다.
		// 2. 패킷을 전송해주나, 이 함수가 리턴되었다고해서 전송을 보장하는 것은 아니다.
		//    블럭킹 소켓에서 이 함수가 리턴되었다는 것은,
		//    커널단의 SendBuffer에 패킷을 성공적으로 쌓아놓았다는 뜻이다.
		//    SendBuffer는 운영체제에 의해서 패킷을 모았다가 실제 전송을 하게 된다.
		//    만약에 SendBuffer버퍼가 꽉 찬 상황에서 이 함수( send() )를 호출했다면,
		//    SendBuffer가 비워질때까지 블럭킹 되게 된다.
		// 3. 리턴값은 SendBuffer에 패킷 복사를 성공한 바이트의 수이다.
		// 4. 소켓이 이 프로그램이나 상대방측( 서버 )에서 closesocket()등을 통해서
		//    연결이 끊겼다면 SOCKET_ERROR가 발생된다.
		int iSendPacketSize = send(server_sock, strBuffer.c_str(), nLength, 0);

		// 송신 패킷 크기 출력
		printf_s("[TCP 클라이언트] 패킷 송신 : %s( %d 바이트 )\n", strBuffer.c_str(), iSendPacketSize);

		if (SOCKET_ERROR == iSendPacketSize)
		{
			err_display("send()");
			break;
		}
	}

	ThreadForBroadcasting.join();

	/// closesocket() : 소켓 닫기
	// 1. server_sock 닫기
	// 2. 상대측과 연결이 되어 있는 소켓을 종료한다.
	// 3. 상대측( 서버 )에서 종료 등의 처리를 먼저해서
	//    send() 시에 오류로 인해 closesocket() 이 되는 것이 아니라,
	//    종료 키워드인 "exit" 에 의해서 연결중에 closesocket() 을 호출하는 것이라면
	//    이 시점에 상대측의 recv() 함수는 0 을 리턴할 것이다.
	closesocket(server_sock);
	server_sock = 0;

	WSACleanup();  // 윈속 종료

	printf_s("[TCP 클라이언트] 종료\n");

	return 0;
}
