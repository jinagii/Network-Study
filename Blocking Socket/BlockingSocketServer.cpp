#define _WINSOCK_DEPRECATED_NO_WARNINGS  // 최신 VC++ 컴파일러에서 경고 및 오류 방지

#include <WinSock2.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <process.h>
#include <thread>
#include <vector>
#include <windows.h>

#pragma comment( lib, "ws2_32" )  // 윈속2 라이브러리

using namespace std;

const unsigned short    BUFSIZE = 512;
const unsigned short    PORT = 9000;

struct SocketInfo
{
	SOCKET* sock;
	SOCKADDR_IN* addr;

	bool operator==(SocketInfo tempInfo) // '==' 연산자가 없어서 오버로딩
	{
		if (sock == tempInfo.sock || addr == tempInfo.addr)
		{
			return true;
		}
		return false;
	};
};

// 전역
SOCKET listen_sock;

vector<SocketInfo> vec_clientSocketInfo;	// 서버 종료시 CloseSocket을 위한 벡터

//thread ClientsThread;
vector<thread> vec_clientThread;	// 다중 서버 접속시 각 쓰레드 join을 위한 벡터
//vector<thread>::iterator iter;

// 종료 플래그
bool isRun = true;

// 소켓 함수 오류 출력
void err_display(const char* cpcMSG)
{
	LPVOID lpMsgBuf = nullptr;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 오류 메시지 표기언어, 제어판에서 설정한 기본언어
		reinterpret_cast<LPSTR>(&lpMsgBuf),
		0,
		nullptr);

	printf_s("[%s] %s", cpcMSG, reinterpret_cast<LPSTR>(lpMsgBuf));

	LocalFree(lpMsgBuf);
}

/// 쓰레드로 처리할 클라이언트와 통신 
void Thread_Clients(SOCKET ClientSock, SOCKADDR_IN ClientAddr)
{
	int addrlen = sizeof(ClientAddr);

	SocketInfo _info;
	_info.sock = &ClientSock;
	_info.addr = &ClientAddr;

	// 종료를 위해 벡터에 넣음
	vec_clientSocketInfo.push_back(_info);

	// 클라이언트와 통신
	while (isRun)
	{
		/// recv() : 소켓의 패킷 수신( 커널단에 있는 소켓의 ReceiveBuffer에 대해서 공부 필요 )
		// 블럭킹 소켓으로 무한대기, closesocker되면 SOCKET_ERROR발생, 0이 리턴되면 정상접속 종료 처리했다라는 뜻
		// 커널단 소켓의 recvBuffer에서 szBuffer[]로 복사
		char szBuffer[BUFSIZE] = { 0, };
		int iRecvPacketSize = recv(ClientSock, szBuffer, BUFSIZE, 0);

		if (isRun == FALSE)
		{
			return;
		}

		if (listen_sock == INVALID_SOCKET/* && isRun == false*/) // listen 소켓이 꺼졌을 시
		{
			printf_s("[TCP 서버] 클라이언트 종료 : IP 주소 = %s, 포트 번호 = %d\n",
				inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));

			// 닫아버린 클라이언트 소켓을 벡터에서 제외해줌
			auto _temp = find(vec_clientSocketInfo.begin(), vec_clientSocketInfo.end(), _info);
			vec_clientSocketInfo.erase(_temp);

			closesocket(ClientSock);
			ClientSock = 0;

			break;
		}

		if (SOCKET_ERROR == iRecvPacketSize)   // 클라이언트 비정상 종료시 이부분으로 들어옴
		{
			printf_s("[TCP 서버] 클라이언트 강제 종료 : IP 주소 = %s, 포트 번호 = %d\n",
				inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));

			// 닫아버린 클라이언트 소켓을 벡터에서 제외해줌
			auto _temp = find(vec_clientSocketInfo.begin(), vec_clientSocketInfo.end(), _info);
			vec_clientSocketInfo.erase(_temp);

			closesocket(ClientSock);
			ClientSock = 0;

			return;
		}
		else if (0 == iRecvPacketSize)	// 클라이언트 정상 종료시 
		{
			printf_s("[TCP 서버] 클라이언트 종료 : IP 주소 = %s, 포트 번호 = %d\n",
				inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));

			// 닫아버린 클라이언트 소켓을 벡터에서 제외해줌
			auto _temp = find(vec_clientSocketInfo.begin(), vec_clientSocketInfo.end(), _info);
			vec_clientSocketInfo.erase(_temp);

			closesocket(ClientSock);
			ClientSock = 0;

			return;
		}
		// inet_ntoa: 인터넷 네트워크 주소를 인터넷 표준 10진 구두점 형태
		// (dotted decimal notation)로 값변경해줌 (big endian으로 변경)
		// ntohs: little endian인 호스트 바이트 순서로 2바이트로 변환

		// 수신 패킷 출력
		if (ClientSock != INVALID_SOCKET && iRecvPacketSize != 0)
		{
			printf_s("[TCP 서버] 패킷 수신 : [%15s:%5d] -> %s( %d 바이트 )\n",
				inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port), szBuffer, iRecvPacketSize);

			// 에코 send()
			for (int i = 0; i < vec_clientSocketInfo.size(); i++)
			{
				int iEcoSendPacketSize = send(*vec_clientSocketInfo[i].sock, szBuffer, iRecvPacketSize, 0);
				printf_s("[TCP 서버] 패킷 송신 : [%15s:%5d] -> %s( %d 바이트 )\n",
					inet_ntoa(vec_clientSocketInfo[i].addr->sin_addr), ntohs(vec_clientSocketInfo[i].addr->sin_port), szBuffer, iRecvPacketSize);
			}
		}
	}
}

/// 쓰레드 join 하기
void ThreadVectorJoin()
{
	for (auto& i : vec_clientThread)
	{
		i.join();
	}
}

/// 블럭킹 함수로부터 서버를 종료하기위한 쓰레드함수
void Thread_CloseFunction(SOCKET* pListenSocket)
{
	string exitOrder;

	while (true)
	{
		cin >> exitOrder;	// exit 입력받기

		if (exitOrder.compare("exit") == 0)
		{
			cout << "[TCP 서버] 서버를 종료합니다." << endl;

			isRun = false;

			for (auto k : vec_clientSocketInfo)	// 각 클라이언트로 서버 종료 메세지 전송
			{
				int iClosePacketSize = send(*k.sock, exitOrder.c_str(), exitOrder.size(), 0);
				printf_s("[TCP 서버] 패킷 송신 : [%15s:%5d] -> %s( %d 바이트 )\n",
					inet_ntoa(k.addr->sin_addr), ntohs(k.addr->sin_port), exitOrder.c_str(), exitOrder.size());

				closesocket(*k.sock);	// 클라이언트 소켓들을 닫아줌
				*k.sock = NULL;
			}

			ThreadVectorJoin();		// 쓰레드를 join 함

			closesocket(listen_sock);
			listen_sock = NULL;

			break;
		}
	}
}

///서버 main////////////////////////////////////////////////////////////////////////////////////
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
	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // IPv4, TCP, 0을 넣어도 디폴트로 IPPROTO_TCP동작

	if (INVALID_SOCKET == listen_sock)
	{
		err_display("socket()");
		WSACleanup();
		return -1;
	}

	/// bind() : 연결을 대기할때 필요할 정보들을 소켓에 셋팅
	// listen() 호출 전에 사용
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//serveraddr.sin_addr.s_addr  = inet_addr( "127.0.0.1" );
	//serveraddr.sin_addr.s_addr  = inet_addr( "본인 컴퓨터의 사설 IP" );
	serveraddr.sin_port = htons(PORT);

	if (SOCKET_ERROR == bind(listen_sock, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
	{
		err_display("bind()");
		closesocket(listen_sock);
		listen_sock = 0;
		WSACleanup();
		return -1;
	}

	// 서버 정상 종료 쓰레드 실행
	thread ThreadForClosing(Thread_CloseFunction, &listen_sock);

	/// listen() : 들어오는 TCP 연결 요청( connect() )의 연결 대기열( Queue )을 만듬
	// 1. 이 함수는 블럭킹 모드 소켓이라도 블럭되지 않음
	// 2. 2번째 인자(5)인 backlog 는 연결 대기열의 갯수를 의미
	// 3. 연결 대기열의 수치가 MAX 까지 다 차지 않았다면,
	//    상대측( 클라이언트 )의 connect() 는 이 시점에서 성공 응답을 받고 리턴된다.
	//    만약에 다 찼다면, connect() 는 ECONNREFUSED 에러를 받고 리턴된다.
	if (SOCKET_ERROR == listen(listen_sock, 5))
	{
		err_display("listen()");
		closesocket(listen_sock);
		listen_sock = 0;
		WSACleanup();
		return -1;
	}

	printf_s("[TCP 서버] accept 시작\n");

	while (isRun)
	{
		SOCKET client_sock;
		SOCKADDR_IN clientaddr;

		/// accept() : connect되고 client_sock만듬  
		int addrlen = sizeof(clientaddr);	// 주소 길이
		ZeroMemory(&clientaddr, addrlen);	// 메모리 영역을 0x00으로 채우는 매크로
		client_sock = accept(listen_sock, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

		if (isRun == false)
		{
			break;
		}

		if (INVALID_SOCKET == client_sock)	// 비정상 접속등의 예외 처리
		{
			err_display("accept()");
			closesocket(listen_sock);
			listen_sock = 0;
			WSACleanup();
			return -1;
		}

		// 접속한 클라이언트 정보 출력
		printf_s("[TCP 서버] accept() 성공 : IP 주소 = %s, 포트 번호 = %d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		thread clientThre = thread(Thread_Clients, client_sock, clientaddr);

		vec_clientThread.push_back(move(clientThre));
	}

	/// closesocket() : 소켓 닫기
	// 1. listen_sock 닫기
	// 2. 소켓의 TCP 연결 요청을 기다리던 소켓을 종료한다.
	closesocket(listen_sock);
	listen_sock = NULL;

	ThreadForClosing.join();	// 서버 정상종료 쓰레드 join

	WSACleanup();	// 윈소켓 종료

	printf_s("[TCP 서버] 종료\n");

	return 0;
}
