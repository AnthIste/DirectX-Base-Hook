//
// Author: AnthIste
// Contact: anthiste.anthiste@gmail.com
// Last updated: 07/04/2010
//

#ifndef INC_SYSTEM
#define INC_SYSTEM

#include <windows.h>
#include <string>

namespace System {

std::wstring GetSystemError();
int SetDebugPrivilege();

}

#endif