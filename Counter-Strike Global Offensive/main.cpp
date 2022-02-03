#include "source.hpp"
#include <time.h>
#include "thread/threading.h"
#include "thread/shared_mutex.h"

#include <thread>

#include "misc.hpp"

#include <Shlobj.h>
#include <iostream>
#include <array>
#include "horizon.hpp"

DWORD installed;

class init_font
{
public:
	init_font(void* font, uint32_t length)
	{
		if (handle = AddFontMemResourceEx(font, length, nullptr, &installed); handle == nullptr)
			return;

		VirtualProtect(font, length, PAGE_READWRITE, 0);
		memset(font, 0, length);
	}

private:
	HANDLE handle;
};

static Semaphore dispatchSem;
static SharedMutex smtx;

using ThreadIDFn = int(_cdecl*)();

ThreadIDFn AllocateThreadID;
ThreadIDFn FreeThreadID;

int AllocateThreadIDWrapper() {
	return AllocateThreadID();
}

int FreeThreadIDWrapper() {
	return FreeThreadID();
}

template<typename T, T& Fn>
static void AllThreadsStub(void*) {
	dispatchSem.Post();
	smtx.rlock();
	smtx.runlock();
	Fn();
}

//TODO: Build this into the threading library
template<typename T, T& Fn>
static void DispatchToAllThreads(void* data, bool t = false) {
	smtx.wlock();

	for (size_t i = 0; i < Threading::numThreads; i++)
		Threading::QueueJobRef(AllThreadsStub<T, Fn>, data);

	if (t) {
		for (size_t i = 0; i < Threading::numThreads; i++)
			dispatchSem.Wait();
	}

	smtx.wunlock();

	Threading::FinishQueue(false);
}


void Entry(HMODULE hModule)
{
	while (!GetModuleHandleA("serverbrowser.dll"))
		Sleep(200);
	if( Source::Create() )
	{
		while( !GetAsyncKeyState( VK_F11 ) )
			Sleep( 200 );
		Source::Destroy();
		Sleep( 1000 );
	}
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason, LPVOID lpReserved )
{
	if( dwReason == DLL_PROCESS_ATTACH )
		CreateThread( nullptr, 0u, (LPTHREAD_START_ROUTINE)Entry, hModule, 0u, nullptr );
	return TRUE;
}