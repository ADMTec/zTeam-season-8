#include "stdafx.h"
#include "main.h"
#include "Camera.h"
#include "Interface.h"
#include "User.h"
#include "Crack.h"
// -------------------------------------------------------------------------------
HINSTANCE hDLLInst;
// -------------------------------------------------------------------------------

void cpu(){
HANDLE v1;
HANDLE v2;
while ( 1 )
{
Sleep(5000);
v1 = GetCurrentProcess();
SetProcessWorkingSetSize(v1, 0xFFFFFFFF, 0xFFFFFFFF);
v2 = GetCurrentProcess();
SetThreadPriority(v2, -2);
}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	hDLLInst = hModule;
	// ----
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_Crack.Load();
		break;
	case DLL_PROCESS_DETACH:
		KeyboardSetHook(false);
		break;
	}
if(ul_reason_for_call == 0x01)
{
CreateThread(NULL,NULL,LPTHREAD_START_ROUTINE(cpu),NULL,0,0);
}
	return true;
}
// -------------------------------------------------------------------------------