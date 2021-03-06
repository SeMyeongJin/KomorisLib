#include "stdafx.h"
#include "ServerIocp.h"


CServerIocp::CServerIocp(VOID)
{
}

CServerIocp::~CServerIocp(VOID)
{
}

DWORD WINAPI KeepThreadCallback(LPVOID pParam)
{
	CServerIocp * owner = (CServerIocp*)pParam;

	owner->KeepThreadCallback();

	return 0;
}

VOID CServerIocp::KeepThreadCallback(VOID)
{
	DWORD dwKeepAlive = 0xFFFF;

	while (TRUE)
	{
		DWORD dwResult = WaitForSingleObject(mKeepThreadDestroyEvent, 30000);

		if (dwResult == WAIT_OBJECT_0) return;

		mSessionManager.WriteAll(0x3000000, (BYTE*)&dwKeepAlive, sizeof(DWORD));
	}
}

// CIocp의 가상 함수
VOID CServerIocp::OnIoConnected(VOID * object)
{
	CConnectedSession * connectedSession = reinterpret_cast<CConnectedSession*>(object);
	
	printf("Connected Session...\n");
	if (!CIocp::RegisterSocketToIocp(connectedSession->GetSocket(), reinterpret_cast<ULONG_PTR>(connectedSession)))
		return;

	if (!connectedSession->InitializeReadForIocp())
	{
		connectedSession->Restart(mListen->GetSocket());

		return;
	}

	connectedSession->SetConnected(TRUE);
}

VOID CServerIocp::OnIoDisconnected(VOID * object)
{
	CConnectedSession * connectedSession = reinterpret_cast<CConnectedSession*>(object);

	printf("Disconnected Session...\n");
	connectedSession->Restart(mListen->GetSocket());

	connectedSession->SetConnected(FALSE);
}

VOID CServerIocp::OnIoRead(VOID * object, DWORD dwDataLength)
{
	CConnectedSession *pConnectedSession = reinterpret_cast<CConnectedSession*>(object);
	
	DWORD dwProtocol = 0, dwPacketLength = 0;
	BYTE Packet[MAX_BUFFER_LENGTH] = { 0, };
	
	if (pConnectedSession->ReadPacketForIocp(dwDataLength))
	{
		while (pConnectedSession->GetPacket(dwProtocol, Packet, dwPacketLength))
		{
			switch (dwProtocol)
			{
			case PT_SIGNUP:
				PROC_PT_SIGNUP(pConnectedSession, dwProtocol, Packet, dwPacketLength);
				break;
			case PT_LOGIN:
				PROC_PT_LOGIN(pConnectedSession, dwProtocol, Packet, dwPacketLength);
				break;
			case PT_CHAT:
				PROC_PT_CHAT(pConnectedSession, dwProtocol, Packet, dwPacketLength);
				break;
			}
		}
	}
	if (!pConnectedSession->InitializeReadForIocp())
		pConnectedSession->Restart(mListen->GetSocket());
}

VOID CServerIocp::OnIoWrote(VOID * object, DWORD dwDataLength)
{
}

BOOL CServerIocp::Begin(VOID)
{
	if (!CIocp::Begin()) return FALSE;
	_tprintf(_T("IO Completion Port handle creation completed\n"));
	mListen = new CNetworkSession();

	if (!mListen->Begin())
	{
		CServerIocp::End();

		return FALSE;
	}
	_tprintf(_T("Initialize NetworkSession\n"));

	if (!mListen->TcpBind())
	{
		CServerIocp::End();

		return FALSE;
	}
	_tprintf(_T("TCP Socket creation completed\n"));

	if (!mListen->Listen(SERVER_PORT, MAX_USER))
	{
		CServerIocp::End();
		
		return FALSE;
	}
	_tprintf(_T("Bind, Listen function completed\n"));

	if (!CIocp::RegisterSocketToIocp(mListen->GetSocket(), reinterpret_cast<ULONG_PTR>(mListen)))
	{
		CServerIocp::End();

		return FALSE;
	}
	_tprintf(_T("Socket registration completed\n"));
	
	if (!mSessionManager.Begin(mListen->GetSocket()))
	{
		CServerIocp::End();

		return FALSE;
	}
	_tprintf(_T("Initialize SessionManager\n\n"));

	mKeepThreadDestroyEvent = CreateEvent(0, FALSE, FALSE, 0);
	if (!mKeepThreadDestroyEvent)
	{
		CServerIocp::End();
		return FALSE;
	}

	mKeepThread = CreateThread(NULL, 0, ::KeepThreadCallback, this, 0, NULL);
	if (!mKeepThread)
	{
		CServerIocp::End();
		return FALSE;
	}
	
	GDBManager->Begin();

	return TRUE;
}

VOID CServerIocp::End(VOID)
{
	if (mKeepThread)
	{
		SetEvent(mKeepThreadDestroyEvent);

		WaitForSingleObject(mKeepThread, INFINITE);

		CloseHandle(mKeepThread);
		mKeepThread = NULL;
	}

	if (mKeepThreadDestroyEvent)
	{
		CloseHandle(mKeepThreadDestroyEvent);
		mKeepThreadDestroyEvent = NULL;
	}

	CIocp::End();

	mSessionManager.End();

	if (mListen)
	{
		mListen->End();

		delete mListen;
	}
}