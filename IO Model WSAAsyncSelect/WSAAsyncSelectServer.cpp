// 최신 VC++ 컴파일러에서 경고 및 오류 방지
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <queue>

using namespace std;

// 윈속2 라이브러리
#pragma comment(lib, "ws2_32")

const unsigned short BUFSIZE = 512;
const unsigned short PORT = 9000;

HWND hWnd;

//char szRecvBuffer[BUFSIZE] = { 0, };

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

	 char szText[BUFSIZE * 2] = { 0, };
	 sprintf_s(szText, "%s %s\n", cpcMSG, reinterpret_cast<LPSTR>(lpMsgBuf));
	 OutputDebugString(szText);

	 LocalFree(lpMsgBuf);
}

// 소켓 메세지
#define WM_SOCKET (WM_USER + 10)

// 사용되는 전역 변수
vector< SOCKET > g_vecClients;                 // 이 매개변수 하나는, 한 명의 클라이언트( 유저 ) 를 뜻 함.

// wouldblock 메세지 처리용 구조체 
struct WBMessageInfo
{
	 const char* Message;
	 SOCKET Socket;
};

queue<WBMessageInfo> g_WBQueue;
//queue<const char*> g_WBMessageQueue;
//queue<SOCKET> g_WBSocketQueue;

// 사용되는 함수들
void BroadcastPacket(const char* const cpcPacket); // 연결된 모든 클라이언트에게 패킷을 전송
BOOL AddSocket(SOCKET socket);                    // 해당 소켓을 vector 에 등록
BOOL RemoveSocket(SOCKET socket);                 // 해당 소켓을 vector 에서 종료 처리하고 제거
void DrawMessage(const char* msg);

// 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// 윈도우 엔트리 포인트
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	 UNREFERENCED_PARAMETER(hPrevInstance);
	 UNREFERENCED_PARAMETER(lpCmdLine);

	 char szBuffer[BUFSIZE] = { 0, };

	 // 윈속 초기화
	 WSADATA wsa;
	 if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	 {
		  sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- WSAStartup() :");
		  err_display(szBuffer);
		  return -1;
	 }

	 // 윈도우 클래스 등록
	 WNDCLASSEX wcex;
	 ZeroMemory(&wcex, sizeof(wcex));
	 wcex.cbSize = sizeof(wcex);
	 wcex.style = CS_HREDRAW | CS_VREDRAW;
	 wcex.lpfnWndProc = WndProc;
	 wcex.hInstance = hInstance;
	 wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	 wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	 wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	 wcex.lpszClassName = "AsyncSelectServer";
	 ATOM ret = RegisterClassEx(&wcex);
	 assert(0 != ret);

	 // 윈도우 생성
	 hWnd = CreateWindow("AsyncSelectServer",
		  "AsyncSelectServer",
		  WS_OVERLAPPEDWINDOW,
		  CW_USEDEFAULT,
		  0,
		  CW_USEDEFAULT,
		  0,
		  nullptr,
		  nullptr,
		  hInstance,
		  nullptr);
	 assert(nullptr != hWnd);

	 ShowWindow(hWnd, SW_SHOWNORMAL);
	 UpdateWindow(hWnd);

	 DrawMessage("[TCP 서버] 생성");

	 MSG msg;
	 while (GetMessage(&msg, nullptr, 0, 0))
	 {
		  TranslateMessage(&msg);
		  DispatchMessage(&msg);
	 }

	 // 윈속 종료
	 WSACleanup();

	 sprintf_s(szBuffer, "[TCP 서버] 종료\n");
	 DrawMessage(szBuffer);
	 OutputDebugString(szBuffer);

	 return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	 static SOCKET listen_sock = INVALID_SOCKET;

	 char szBuffer[BUFSIZE] = { 0, };

	 switch (message)
	 {
		  case WM_CREATE:
		  {
				// 1. socket()
				listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if (INVALID_SOCKET == listen_sock)
				{
					 sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- socket() :");
					 err_display(szBuffer);
					 DrawMessage(szBuffer);
					 return 0;
				}

				// 주요 이벤트들, 더 많은 값들은 MSDN( WSAAsyncSelect() ) 참조
				// ------------------------------------------------------
				// 이벤트       | 의미
				// ------------------------------------------------------
				// FD_ACCEPT    | 연결 요청이 들어왔을 때 통지 받음
				// FD_CONNECT   | 연결이 완료되었을 때 통지 받음
				// FD_READ      | 수신할 데이터가 있을 때 통지 받음
				// FD_WRITE     | 전송 가능 상태가 되었을때 통지 받음( 예를 들면 아래와 같은 케이스들 )
				//              | 1. connect() 함수를 호출하고 소켓이 처음 연결되었을 때 
				//              | 2. accept() 함수를 호출하고 소켓의 연결이 수락되었을 때
				//              | 3. send() 함수를 호출하여 WSAEWOULDBLOCK 이 리턴되었다가 전송 버퍼가 비워졌을 때
				// FD_CLOSE     | 소켓이 정상 종료되었을 때 통지 받음
				// ------------------------------------------------------
				// 2. listen_sock 의 WSAAsyncSelect 이벤트 등록,
				//    우리가 정의한 WM_SOCKET 메세지를 통해서 FD_ACCEPT 를 이벤트로 받는다.
				//    이렇게 등록된 이벤트는 차 후, 소켓 메시지가 발생했을때 lParam 에 담겨서 들어온다.
				//    lParam 을 보고 어떤 이벤트에 대한 처리를 해야할지 알 수 있다.
				if (SOCKET_ERROR == WSAAsyncSelect(listen_sock, hWnd, WM_SOCKET, FD_ACCEPT))
				{
					 sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- WSAAsyncSelect() :");
					 err_display(szBuffer);
					 DrawMessage(szBuffer);

					 closesocket(listen_sock);
					 listen_sock = INVALID_SOCKET;
					 return 0;
				}

				// 3. bind()
				SOCKADDR_IN serveraddr;
				ZeroMemory(&serveraddr, sizeof(serveraddr));
				serveraddr.sin_family = AF_INET;
				serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
				serveraddr.sin_port = htons(PORT);

				if (SOCKET_ERROR == bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
				{
					 sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- bind() :");
					 err_display(szBuffer);
					 DrawMessage(szBuffer);

					 closesocket(listen_sock);
					 listen_sock = INVALID_SOCKET;
					 return 0;
				}

				// 4. listen()
				if (SOCKET_ERROR == listen(listen_sock, 5))
				{
					 sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- listen() :");
					 err_display(szBuffer);
					 DrawMessage(szBuffer);

					 closesocket(listen_sock);
					 listen_sock = INVALID_SOCKET;
					 return 0;
				}

				break;
		  } // end of WM_CREATE

		  case WM_SOCKET:
		  {
				// 이벤트가 발생한 소켓이 wParam 에 담겨온다.
				SOCKET sock = static_cast<SOCKET>(wParam);

				// WSAGETSELECTERROR(lParam) 은 HIWORD(lParam) 와 같으며
				// 해당 소켓에 에러가 발생했을 경우, 에러값이 담겨온다.
				int iErr = static_cast<int>(WSAGETSELECTERROR(lParam));

				// 에러 체크
				if (iErr)
				{
					 sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- 소켓 에러 : %d\n", iErr);
					 DrawMessage(szBuffer);
					 OutputDebugString(szBuffer);

					 // vector 에서 소켓 제거 및 종료 처리
					 if (!RemoveSocket(sock))
					 {
						  sprintf_s(szBuffer, "[TCP 서버] 소켓 해제 실패\n");
						  DrawMessage(szBuffer);
						  OutputDebugString(szBuffer);

						  // 소켓 종료 처리
						  shutdown(sock, SD_BOTH);
						  closesocket(sock);
						  return 0;
					 }

					 return 0;
				}

				// WSAGETSELECTEVENT(lParam) 은 LOWORD(lParam) 와 같으며
				// 소켓에 발생한 이벤트 값이 담겨온다.
				// 이벤트 처리
				switch (WSAGETSELECTEVENT(lParam))
				{
					 case FD_ACCEPT:
					 {
						  // 5. accept()
						  SOCKADDR_IN clientaddr;
						  int addrlen = sizeof(clientaddr);
						  ZeroMemory(&clientaddr, addrlen);
						  SOCKET client_sock = accept(sock, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

						  if (INVALID_SOCKET == client_sock)
						  {
								sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- accept() :");
								err_display(szBuffer);
								DrawMessage(szBuffer);
								return 0;
						  }

						  // 6. client_sock 의 WSAAsyncSelect 이벤트 등록,
						  // 우리가 정의한 WM_SOCKET 메세지를 통해서 FD_READ | FD_WRITE | FD_CLOSE 세가지 이벤트를 받도록 한다.
						  if (SOCKET_ERROR == WSAAsyncSelect(client_sock, hWnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE))
						  {
								sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- WSAAsyncSelect() :");
								err_display(szBuffer);
								DrawMessage(szBuffer);

								// 소켓 종료 처리
								shutdown(client_sock, SD_BOTH);
								closesocket(client_sock);
								return 0;
						  }

						  // vector 에 소켓을 등록
						  if (!AddSocket(client_sock))
						  {
								sprintf_s(szBuffer, "[TCP 서버] 소켓 등록 실패\n");
								DrawMessage(szBuffer);
								OutputDebugString(szBuffer);

								// 소켓 종료 처리
								shutdown(client_sock, SD_BOTH);
								closesocket(client_sock);
								return 0;
						  }

						  break;
					 }

					 case FD_READ:
					 {
						  char szRecvBuffer[BUFSIZE] = { 0, };

						  // 7. 패킷 수신
						  int iRecvPacketSize = recv(sock, szRecvBuffer, BUFSIZE, 0);

						  if (SOCKET_ERROR == iRecvPacketSize)
						  {
								// 넌블로킹 소켓에서 WSAEWOULDBLOCK 는 에러를 의미하는 것이 아니다.
								// FD_READ 메시지 한번당 recv() 함수 한번을 호출해서 메시지를 처리해야한다.
								// recv() 함수를 호출해서 BUFSIZE 만큼 데이터를 처리했으나,
								// 아직 수신해야할 데이터가 더 남아있을때는 FD_READ 가 또 발생하게 되므로
								// 그때 이어서 처리하면 된다.
								// 만약 FD_READ 가 호출되었는데 recv() 를 여러번 호출한다면,
								// 내부의 수신 버퍼에 데이터가 더 이상 없을때, WSAEWOULDBLOCK 이 리턴될 것이다.
								// 물론 에러는 아니므로 무시해도 되나, 깔끔하지 못하다.
								if (WSAGetLastError() != WSAEWOULDBLOCK)
								{
									 sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- recv() :");
									 err_display(szBuffer);
									 DrawMessage(szBuffer);

									 // vector 에서 소켓 제거 및 종료 처리
									 if (!RemoveSocket(sock))
									 {
										  sprintf_s(szBuffer, "[TCP 서버] 소켓 해제 실패\n");
										  DrawMessage(szBuffer);
										  OutputDebugString(szBuffer);

										  // 소켓 종료 처리
										  shutdown(sock, SD_BOTH);
										  closesocket(sock);
										  return 0;
									 }

									 return 0;
								}
						  }

						  // getpeername() 으로 소켓의 원격 아이피, 포트 정보를 얻을 수도 있다.
						  SOCKADDR_IN clientaddr;
						  int addrlen = sizeof(clientaddr);
						  ZeroMemory(&clientaddr, addrlen);
						  getpeername(sock, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

						  sprintf_s(szBuffer, "[TCP 서버] [%15s:%5d] 패킷 수신 : %s( %d 바이트 )\n",
								inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port),
								szRecvBuffer, iRecvPacketSize);
						  DrawMessage(szBuffer);
						  OutputDebugString(szBuffer);

						  // 채팅 패킷 브로드 캐스팅
						  // 패킷 만들기 
						  SYSTEMTIME st;
						  GetLocalTime(&st);
						  char szPacket[BUFSIZE * 2] = { 0, };
						  sprintf_s(szPacket, "[%04d-%02d-%02d %02d:%02d:%02d] [%15s:%5d] 님의 말 : %s",
								st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
								inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port),
								szRecvBuffer);
						  // 패킷 브로드 캐스팅
						  BroadcastPacket(szPacket);

						  break;
					 }

					 // 과제
					 // 전송 가능 상태가 되었을때 통지 받음
					 // 현재 프로그램은 FD_WRITE 를 사용하지 않고 있다.
					 // 이는 잘 못 구현된 프로그램이므로,
					 // FD_WRITE 를 활용하여 프로그램을 완성하자.
					 // send() 시 WSAEWOULDBLOCK 에러에 대한 처리가 필요하다.
					 //-------------------------------------------------------------------------------------
					 // FD_WRITE     | 전송 가능 상태가 되었을때 통지 받음( 예를 들면 아래와 같은 케이스들 )
					 //              | 1. connect() 함수를 호출하고 소켓이 처음 연결되었을 때 
					 //              | 2. accept() 함수를 호출하고 소켓의 연결이 수락되었을 때
					 //              | 3. send() 함수를 호출하여 WSAEWOULDBLOCK 이 리턴되었다가 전송 버퍼가 비워졌을 때
					 case FD_WRITE:
					 {
						  // wouldblock으로 큐에 있는 것들을 순차적으로 처리
						  while (g_WBQueue.empty() == false)
						  {
								int iSendPacketSize = send(
									 g_WBQueue.front().Socket, g_WBQueue.front().Message,
									 static_cast<int>(strlen(g_WBQueue.front().Message)), 0);

								g_WBQueue.pop();
						  }

						  //// wouldblock으로 메세지큐에 있는 것을 처리
						  //while (g_WBMessageQueue.empty() == false)
						  //{
						  //	// 순차적으로 처리
						  //	BroadcastPacket(g_WBMessageQueue.front());

						  //	g_WBMessageQueue.pop();
						  //}
						  break;
					 }

					 case FD_CLOSE:
					 {
						  // vector 에서 소켓 제거 및 종료 처리
						  if (!RemoveSocket(sock))
						  {
								sprintf_s(szBuffer, "[TCP 서버] 소켓 해제 실패\n");
								DrawMessage(szBuffer);
								OutputDebugString(szBuffer);

								// 소켓 종료 처리
								shutdown(sock, SD_BOTH);
								closesocket(sock);
								return 0;
						  }

						  break;
					 }
				} //end of switch (WSAGETSELECTEVENT(lParam))

				break;
		  } // end of WM_SOCKET

		  case WM_DESTROY:
		  {
				// listen_sock 소켓 종료
				closesocket(listen_sock);
				listen_sock = INVALID_SOCKET;

				// 연결되어 있는 모든 소켓들 종료
				size_t nThreadCount = g_vecClients.size();
				for (size_t i = 0; i < nThreadCount; ++i)
				{
					 if (INVALID_SOCKET != g_vecClients[i])
					 {
						  // 소켓 종료 처리
						  shutdown(g_vecClients[i], SD_BOTH);
						  closesocket(g_vecClients[i]);
						  g_vecClients[i] = INVALID_SOCKET;
					 }
				}
				g_vecClients.clear();

				PostQuitMessage(0);

				break;
		  }

		  default:
				return DefWindowProc(hWnd, message, wParam, lParam);
	 } // switch (message)

	 return 0;
}

void BroadcastPacket(const char* const cpcPacket)
{
	 char szBuffer[BUFSIZE] = { 0, };

	 size_t nThreadCount = g_vecClients.size();
	 for (size_t i = 0; i < nThreadCount; ++i)
	 {
		  if (INVALID_SOCKET == g_vecClients[i])
		  {
				continue;
		  }

		  // 패킷 송신
		  int iSendPacketSize = send(g_vecClients[i], cpcPacket, static_cast<int>(strlen(cpcPacket)), 0);

		  if (SOCKET_ERROR == iSendPacketSize)
		  {
				// 넌블로킹 소켓에서 WSAEWOULDBLOCK 는 소켓의 에러를 의미하는 것이 아니다.
				// send() 함수 호출시, WSAEWOULDBLOCK 이 발생한다면,
				// 해당 소켓 내부의 송신 버퍼가 가득차서 더이상 패킷을 보낼 수 없는 상태임을 알려주는 것이다.
				// 이 경우, 내부의 송신 버퍼가 다 비워지게 되면, 송신 가능한 상태가 되므로
				// WM_SOCKET( FD_WRITE ) 가 발생하게 된다.
				// 이번에 실패했던 패킷을 그 FD_WRITE 가 발생한 시점에 전송해 주면된다.
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					 // getpeername() 으로 소켓의 원격 아이피, 포트 정보를 얻을 수도 있다.
					 SOCKADDR_IN clientaddr;
					 int addrlen = sizeof(clientaddr);
					 ZeroMemory(&clientaddr, addrlen);
					 getpeername(g_vecClients[i], reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

					 sprintf_s(szBuffer, "[TCP 서버] [%15s:%5d] 에러 발생 -- send() :",
						  inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
					 err_display(szBuffer);
					 DrawMessage(szBuffer);

					 // vector 에서 소켓 제거 및 종료 처리
					 if (!RemoveSocket(g_vecClients[i]))
					 {
						  sprintf_s(szBuffer, "[TCP 서버] 소켓 해제 실패\n");
						  DrawMessage(szBuffer);
						  OutputDebugString(szBuffer);

						  // 소켓 종료 처리
						  shutdown(g_vecClients[i], SD_BOTH);
						  closesocket(g_vecClients[i]);
					 }
				}
				else
				{
					 // send() 호출시 wouldblock발생하면 소켓송신 버퍼 가득차서 패킷송신이 밀림
					 // 버퍼 비우고 송신 재개
					 // queue에 집어넣고 나중에 차례로 pop하여 처리
					 //g_WBMessageQueue.push(cpcPacket);
					 // 소켓당 일어나는 wouldblock 이므로 다중소켓에서 발생할 경우를 위해 소켓도 push한다.
					 //g_WBSocketQueue.push(g_vecClients[i]);

					 WBMessageInfo wb = { cpcPacket, g_vecClients[i] };
					 g_WBQueue.push(wb);
				}
		  }
	 }
}

BOOL AddSocket(SOCKET socket)
{
	 char szBuffer[BUFSIZE] = { 0, };

	 size_t nThreadCount = g_vecClients.size();

	 // 중복 등록 검사
	 for (size_t i = 0; i < nThreadCount; ++i)
	 {
		  if (socket == g_vecClients[i])
		  {
				return FALSE;
		  }
	 }

	 // getpeername() 으로 소켓의 원격 아이피, 포트 정보를 얻을 수도 있다.
	 SOCKADDR_IN clientaddr;
	 int addrlen = sizeof(clientaddr);
	 ZeroMemory(&clientaddr, addrlen);
	 getpeername(socket, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

	 int iIndex = -1;

	 // 현재 vector 를 사용 중이므로, 중간 중간에 사용되지 않는 인덱스를 활용한다.
	 for (size_t i = 0; i < nThreadCount; ++i)
	 {
		  // 해당 인덱스는 사용중이지 않다.
		  if (INVALID_SOCKET == g_vecClients[i])
		  {
				// 새로운 연결이 해당 인덱스 위치를 사용하도록 함.
				iIndex = i;
				g_vecClients[iIndex] = socket;

				break;
		  }
	 }

	 // 빈 인덱스가 없을때는 맨 뒤에 추가
	 if (-1 == iIndex)
	 {
		  g_vecClients.push_back(socket);
	 }

	 sprintf_s(szBuffer, "[TCP 서버] [%15s:%5d] 클라이언트가 접속\n",
		  inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	 DrawMessage(szBuffer);
	 OutputDebugString(szBuffer);

	 return TRUE;
}

BOOL RemoveSocket(SOCKET socket)
{
	 char szBuffer[BUFSIZE] = { 0, };

	 size_t nThreadCount = g_vecClients.size();
	 for (size_t i = 0; i < nThreadCount; ++i)
	 {
		  if (socket == g_vecClients[i])
		  {
				// getpeername() 으로 소켓의 원격 아이피, 포트 정보를 얻을 수도 있다.
				SOCKADDR_IN clientaddr;
				int addrlen = sizeof(clientaddr);
				ZeroMemory(&clientaddr, addrlen);
				getpeername(g_vecClients[i], reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

				// 소켓 종료
				shutdown(g_vecClients[i], SD_BOTH);
				closesocket(g_vecClients[i]);
				g_vecClients[i] = INVALID_SOCKET;

				sprintf_s(szBuffer, "[TCP 서버] [%15s:%5d] 클라이언트와 종료\n",
					 inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				DrawMessage(szBuffer);
				OutputDebugString(szBuffer);

				return TRUE;
		  }
	 }

	 return FALSE;
}

// 윈도우에 띄워줄 메세지 함수
void DrawMessage(const char* msg)
{
	 static int msgline = 1;

	 TextOut(GetDC(hWnd), 10, msgline, msg, lstrlen(msg));

	 msgline += 25;
}
