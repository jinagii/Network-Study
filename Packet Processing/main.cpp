// 최신 VC++ 컴파일러에서 경고 및 오류 방지
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <stdio.h>
#include <queue>
#include <vector>
#include <string>

// 윈속2 라이브러리
#pragma comment( lib, "ws2_32" )

const unsigned short    BUFSIZE = 512;
const unsigned short    PORT = 9000;

using namespace std;

queue<char> g_ChatBuffer;
bool indexFlag = true;


// 소켓 함수 오류 출력
void err_display(const char* const cpcMSG)
{
	LPVOID lpMsgBuf = nullptr;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /*
														 오류 메시지를 어떤 언어로 표시할 것인지를 나타내며,
														 이렇게 입력하면 사용자가 제어판에서 설정한 기본언어로 오류 메시지를 얻을 수 있다.
													 */
		reinterpret_cast<LPSTR>(&lpMsgBuf),
		0,
		nullptr);

	printf_s("[%s] %s", cpcMSG, reinterpret_cast<LPSTR>(lpMsgBuf));

	LocalFree(lpMsgBuf);
}

int main(void)
{
	// 윈속 초기화( WinSock 2.2 )
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		err_display("WSAStartup()");
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	// socket() : 소켓 생성
	////////////////////////////////////////////////////////////////////////////
	// listen() 담당 소켓 생성
	//                                          TCP 소켓 생성
	//                                    ( 2번째 인자인 SOCK_STREAM 이 TCP 를 뜻하므로,
	//                              IPv4,   3번째 인자는 0 을 넣어주더라도 디폴트로 IPPROTO_TCP 가 동작 )
	////////////////////////////////////////////////////////////////////////////
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listen_sock)
	{
		err_display("socket()");
		WSACleanup();
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	// bind() : 연결을 대기할때 필요할 정보들을 소켓에 셋팅
	////////////////////////////////////////////////////////////////////////////
	// listen() 호출 전에 사용
	////////////////////////////////////////////////////////////////////////////
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

	////////////////////////////////////////////////////////////////////////////
	// listen() : 들어오는 TCP 연결 요청( connect() )의 연결 대기열( Queue )을 만듬
	////////////////////////////////////////////////////////////////////////////
	// 1. 이 함수는 블럭킹 모드 소켓이라도 블럭되지 않음
	// 2. 2번째 인자인 backlog 는 연결 대기열의 갯수를 의미
	// 3. 연결 대기열의 수치가 MAX 까지 다 차지 않았다면,
	//    상대측( 클라이언트 )의 connect() 는 이 시점에서 성공 응답을 받고 리턴된다.
	//    만약에 다 찼다면, connect() 는 ECONNREFUSED 에러를 받고 리턴된다.
	////////////////////////////////////////////////////////////////////////////
	if (SOCKET_ERROR == listen(listen_sock, 5))
	{
		err_display("listen()");
		closesocket(listen_sock);
		listen_sock = 0;
		WSACleanup();
		return -1;
	}

	printf_s("[TCP 서버] accept 시작\n");

	////////////////////////////////////////////////////////////////////////////
	// accept() : 대기열에 있는 연결 요청에 대한 수락 및 연결된 소켓 생성
	////////////////////////////////////////////////////////////////////////////
	// 이 시점 이 후부터는 실제로 정상적인 연결이 되었으므로 상대측과 통신을 할 수 있다.
	////////////////////////////////////////////////////////////////////////////
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	ZeroMemory(&clientaddr, addrlen);
	SOCKET client_sock = accept(listen_sock, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

	if (INVALID_SOCKET == client_sock)
	{
		err_display("accept()");
		closesocket(listen_sock);
		listen_sock = 0;
		WSACleanup();
		return -1;
	}

	short _chatIndex = -1;


	// 접속한 클라이언트 정보 출력
	printf_s("[TCP 서버] accept() 성공 : IP 주소 = %s, 포트 번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	// 클라이언트와 통신
	while (TRUE)
	{
		char szBuffer[BUFSIZE] = { 0, };

		////////////////////////////////////////////////////////////////////////////
		// recv() : 소켓의 패킷 수신( 커널단에 있는 소켓의 ReceiveBuffer 에 대해서 공부 필요 )
		////////////////////////////////////////////////////////////////////////////
		// 1. 소켓이 블럭킹 소켓이므로 패킷을 수신할때까지 무한정 대기한다.
		// 2. 소켓이 여기서 closesocket() 이 되거나, 상대방측에서 비정상 접속 종료를 한다면 SOCKET_ERROR 가 발생된다.
		// 3. 리턴값은 수신된 패킷 바이트를 뜻하나, 만약에 0 이 리턴된다면
		//    상대방 측에서 정상적인 접속 종료처리( closesocket() 등 )를 진행했다는 의미이다.
		// 4. 커널단에 소켓의 RecvBuffer 가 존재하며, 해당 버퍼에서 szBuffer[BUFSIZE] 로 복사를 해온다.
		////////////////////////////////////////////////////////////////////////////
		int iRecvPacketSize = recv(client_sock, szBuffer, BUFSIZE, 0);

		if (SOCKET_ERROR == iRecvPacketSize)
		{
			err_display("recv()");
			closesocket(client_sock);
			client_sock = 0;
			break;
		}
		else if (0 == iRecvPacketSize)
		{
			printf_s("[TCP 서버] 클라이언트 종료 : IP 주소 = %s, 포트 번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			closesocket(client_sock);
			client_sock = 0;
			break;
		}

		printf_s("[TCP 서버] 패킷 수신 : [%15s:%5d] -> ", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		for (int i = 0; i < iRecvPacketSize; i++)
		{
			printf_s("%d ", szBuffer[i]);
		}

		printf_s("( %d 바이트 ) \n", iRecvPacketSize);	// 아스키 코드로 변환해서 출력

		for (int i = 0; i < iRecvPacketSize; i++)
		{
			g_ChatBuffer.push(szBuffer[i]);
		}

		// IndexFlag true로 시작함
		// 앞의 숫자를 찾아서 더한다 
		// g_chatbuffer의 크기가 2이상이면
		// front를 두개 pop하고 더한다
		// 이것이 패킷의 index
		// index의 크기보다 chatbuffer의 크기가 커지면 
		// send한 패킷을 출력할 준비가 됨
		// index 크기 만큼 프린트하고 pop해준다
		while (true)
		{
			if (indexFlag)
			{
				if (g_ChatBuffer.size() >= 2)
				{
					short _temp1 = g_ChatBuffer.front();
					g_ChatBuffer.pop();
					short _temp2 = g_ChatBuffer.front();
					g_ChatBuffer.pop();

					_chatIndex = _temp1 + _temp2;
					indexFlag = false;
				}
				else
				{
					break;
				}
			}

			if (g_ChatBuffer.size() >= _chatIndex)
			{
				
				for (int i = 0; i < _chatIndex; i++)
				{
					printf_s("%c ", g_ChatBuffer.front());
					g_ChatBuffer.pop();
				}
				printf_s("\n");
				_chatIndex = -1;
				indexFlag = true;
			}
			else
			{
				break;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// closesocket() : 소켓 닫기
	////////////////////////////////////////////////////////////////////////////
	// 1. listen_sock 닫기
	// 2. 소켓의 TCP 연결 요청을 기다리던 소켓을 종료한다.
	////////////////////////////////////////////////////////////////////////////
	closesocket(listen_sock);
	listen_sock = 0;

	// 윈속 종료
	WSACleanup();

	printf_s("[TCP 서버] 종료\n");

	return 0;
}