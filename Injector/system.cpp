#include "System.h"

namespace System {

std::wstring GetSystemError()
{
	std::wstring result;

	wchar_t lpMsgBuf[500] = {0};
	FormatMessageW( 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		lpMsgBuf,
		sizeof(lpMsgBuf),
		NULL 
	);

	result = lpMsgBuf;
	LocalFree(lpMsgBuf);

	return result;
}

}