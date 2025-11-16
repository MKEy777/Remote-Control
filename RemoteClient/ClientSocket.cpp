#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = nullptr;
CClientSocket::CHelper CClientSocket::m_helper;
std::string GetErrInfo(int wsaErrcode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		wsaErrcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}