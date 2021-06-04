// �ֽ� VC++ �����Ϸ����� ��� �� ���� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <queue>

using namespace std;

// ����2 ���̺귯��
#pragma comment(lib, "ws2_32")

const unsigned short BUFSIZE = 512;
const unsigned short PORT = 9000;
const string IP = "127.0.0.1";

HWND hWnd;

// ���� �Լ� ���� ���
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

// ���� �޼���
#define WM_SOCKET (WM_USER + 10)

// ���Ǵ� ���� ����
vector< SOCKET > g_vecClients;                 // �� �Ű����� �ϳ���, �� ���� Ŭ���̾�Ʈ( ���� ) �� �� ��.

// wouldblock �޼��� ó���� ����ü 
struct WBMessageInfo
{
	 const char* Message;
	 SOCKET Socket;
};

queue<WBMessageInfo> g_WBQueue;
//queue<const char*> g_WBMessageQueue;
//queue<SOCKET> g_WBSocketQueue;

// ���� ���� ������
CRITICAL_SECTION    g_CS;
SOCKET              g_server_sock = INVALID_SOCKET;
BOOL                bExit = FALSE;

// ���Ǵ� �Լ���
void BroadcastPacket(const char* const cpcPacket); // ����� ��� Ŭ���̾�Ʈ���� ��Ŷ�� ����
BOOL AddSocket(SOCKET socket);                    // �ش� ������ vector �� ���
BOOL RemoveSocket(SOCKET socket);                 // �ش� ������ vector ���� ���� ó���ϰ� ����
void DrawMessage(const char* msg);

// ������ ���ν���
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// ������ ��Ʈ�� ����Ʈ
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	 UNREFERENCED_PARAMETER(hPrevInstance);
	 UNREFERENCED_PARAMETER(lpCmdLine);

	 char szBuffer[BUFSIZE] = { 0, };

	 // ���� �ʱ�ȭ
	 WSADATA wsa;
	 if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	 {
		  sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSAStartup() :");
		  err_display(szBuffer);
		  return -1;
	 }

	 // ������ Ŭ���� ���
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

	 // ������ ����

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

	 DrawMessage("[TCP Ŭ���̾�Ʈ] ����");

	 MSG msg;
	 while (GetMessage(&msg, nullptr, 0, 0))
	 {
		  TranslateMessage(&msg);
		  DispatchMessage(&msg);
	 }

	 // ���� ����
	 WSACleanup();

	 sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ����\n");
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
					 sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] �����߻� -- WSAStartup() : ");
					 err_display(szBuffer);
					 return -1;
				}

				printf_s("============================\n");
				printf_s(" ���α׷� ���� Ű���� -> exit\n");
				printf_s("============================\n");


				// 1. socket()
				server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if (INVALID_SOCKET == server_sock)
				{
					 sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- socket() :");
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
					 sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- connect() :");
					 err_display(szBuffer);

					 closesocket(server_sock);
					 server_sock = INVALID_SOCKET;
					 WSACleanup();
					 return -1;
				}

				printf_s("[TCP Ŭ���̾�Ʈ] ������ ������ �Ǿ����ϴ�.\n");

				break;
		  }

		  switch (WSAGETSELECTEVENT(lParam))
		  {
				case FD_READ:
				{
					 printf_s("������ ���ڿ� �Է� : ");

					 // ����� �Է� �ޱ�
					 string strBuffer;
					 getline(cin, strBuffer);
					 size_t nLength = strBuffer.size();

					 // �Էµ� ���ڿ��� ����
					 if (0 == nLength)
					 {
						  printf_s("-> �Էµ� ���ڿ��� �����ϴ�.\n");
						  //continue;
					 }
					 // �ִ� �Է� ���ڿ��� 256 ����Ʈ�� ����
					 else if (256 < nLength)
					 {
						  printf_s("-> �ִ� ���ڿ� ����Ʈ�� �ʰ��Ͽ����ϴ�.\n");
						  //continue;
					 }
					 // ���� Ű���� "exit"
					 else if (strBuffer.compare("exit") == 0)
					 {
						  printf_s("-> ���α׷��� �����մϴ�.\n");
						  break;
					 }

					 int iSendPacketSize = send(server_sock, strBuffer.c_str(), nLength, 0);

					 if (SOCKET_ERROR == iSendPacketSize)
					 {
						  sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- send() :");
						  err_display(szBuffer);
						  break;
					 }

					 printf_s("[TCP Ŭ���̾�Ʈ] ��Ŷ ���� <- %s( %d ����Ʈ )\n", szBuffer, iSendPacketSize);

					 break;
				}
				case FD_CLOSE:
				{
					 // server_sock �ݱ�
					 closesocket(server_sock);
					 server_sock = INVALID_SOCKET;

					 // ���� ����
					 WSACleanup();

					 printf_s("[TCP Ŭ���̾�Ʈ] ����\n");

					 return 0;
				}
		  }

		  default:
				return DefWindowProc(hWnd, message, wParam, lParam);
	 }

	 // ���� �ʱ�ȭ

	 return 0;
}

// �����쿡 ����� �޼��� �Լ�
void DrawMessage(const char* msg)
{
	 static int msgline = 1;

	 TextOut(GetDC(hWnd), 10, msgline, msg, lstrlen(msg));

	 msgline += 25;
}