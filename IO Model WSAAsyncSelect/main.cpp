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
const string IP = "127.0.0.1";

HWND hWnd;

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

// 사용될 전역 변수들
CRITICAL_SECTION    g_CS;
SOCKET              g_server_sock = INVALID_SOCKET;
BOOL                bExit = FALSE;

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
		  sprintf_s(szBuffer, "[TCP 클라이언트] 에러 발생 -- WSAStartup() :");
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
	 wcex.hIcon = NULL;//LoadIcon(hInstance, IDI_APPLICATION);
	 wcex.hCursor = NULL;//LoadCursor(nullptr, IDC_ARROW);
	 wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	 wcex.lpszClassName = "AsyncSelectClient";
	 ATOM ret = RegisterClassEx(&wcex);
	 assert(0 != ret);

	 // 윈도우 생성

	 hWnd = CreateWindow("AsyncSelectClient",
		  "AsyncSelectClient",
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

	 DrawMessage("[TCP 클라이언트] 생성");

	 MSG msg;
	 while (GetMessage(&msg, nullptr, 0, 0))
	 {
		  TranslateMessage(&msg);
		  DispatchMessage(&msg);
	 }

	 // 윈속 종료
	 WSACleanup();

	 sprintf_s(szBuffer, "[TCP 클라이언트] 종료\n");
	 DrawMessage(szBuffer);
	 OutputDebugString(szBuffer);

	 return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	 char szBuffer[BUFSIZE] = { 0, };
	 SOCKET server_sock;

	 switch (message)
	 {
		  case WM_CREATE:
		  {
				WSADATA wsa;
				if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
				{
					 sprintf_s(szBuffer, "[TCP 클라이언트] 에러발생 -- WSAStartup() : ");
					 err_display(szBuffer);
					 return -1;
				}

				printf_s("============================\n");
				printf_s(" 프로그램 종료 키워드 -> exit\n");
				printf_s("============================\n");


				// 1. socket()
				server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if (INVALID_SOCKET == server_sock)
				{
					 sprintf_s(szBuffer, "[TCP 클라이언트] 에러 발생 -- socket() :");
					 err_display(szBuffer);

					 WSACleanup();
					 return -1;
				}

				// 2. connect()
				SOCKADDR_IN serveraddr;
				ZeroMemory(&serveraddr, sizeof(serveraddr));
				serveraddr.sin_family = AF_INET;
				serveraddr.sin_port = htons(PORT);
				serveraddr.sin_addr.s_addr = inet_addr(IP.c_str());
				if (SOCKET_ERROR == connect(server_sock, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
				{
					 sprintf_s(szBuffer, "[TCP 클라이언트] 에러 발생 -- connect() :");
					 err_display(szBuffer);

					 closesocket(server_sock);
					 server_sock = INVALID_SOCKET;
					 WSACleanup();
					 return -1;
				}

				printf_s("[TCP 클라이언트] 서버와 연결이 되었습니다.\n");

				break;
		  }

		  switch (WSAGETSELECTEVENT(lParam))
		  {
				case FD_READ:
				{
					 printf_s("전송할 문자열 입력 : ");

					 // 사용자 입력 받기
					 string strBuffer;
					 getline(cin, strBuffer);
					 size_t nLength = strBuffer.size();

					 // 입력된 문자열이 없음
					 if (0 == nLength)
					 {
						  printf_s("-> 입력된 문자열이 없습니다.\n");
						  //continue;
					 }
					 // 최대 입력 문자열은 256 바이트로 제한
					 else if (256 < nLength)
					 {
						  printf_s("-> 최대 문자열 바이트를 초과하였습니다.\n");
						  //continue;
					 }
					 // 종료 키워드 "exit"
					 else if (strBuffer.compare("exit") == 0)
					 {
						  printf_s("-> 프로그램을 종료합니다.\n");
						  break;
					 }

					 int iSendPacketSize = send(server_sock, strBuffer.c_str(), nLength, 0);

					 if (SOCKET_ERROR == iSendPacketSize)
					 {
						  sprintf_s(szBuffer, "[TCP 클라이언트] 에러 발생 -- send() :");
						  err_display(szBuffer);
						  break;
					 }

					 printf_s("[TCP 클라이언트] 패킷 수신 <- %s( %d 바이트 )\n", szBuffer, iSendPacketSize);

					 break;
				}
				case FD_CLOSE:
				{
					 // server_sock 닫기
					 closesocket(server_sock);
					 server_sock = INVALID_SOCKET;

					 // 윈속 종료
					 WSACleanup();

					 printf_s("[TCP 클라이언트] 종료\n");

					 return 0;
				}
		  }

		  default:
				return DefWindowProc(hWnd, message, wParam, lParam);
	 }

	 // 윈속 초기화

	 return 0;
}

// 윈도우에 띄워줄 메세지 함수
void DrawMessage(const char* msg)
{
	 static int msgline = 1;

	 TextOut(GetDC(hWnd), 10, msgline, msg, lstrlen(msg));

	 msgline += 25;
}