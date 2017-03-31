#include "pch.h"
#include "ActivationFactory.h"

namespace com {

static ActivationFactory	*g_pActivationFactory = NULL;
HINSTANCE					g_instance = NULL;

HINSTANCE GetInstance() { return g_instance; }

extern "C" {
	BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
	{
		if(dwReason == DLL_PROCESS_ATTACH)	g_instance = hInstance;
		return TRUE;
	}
	STDAPI DllGetActivationFactory(HSTRING activatibleClassId, IActivationFactory** factory)
	{
		return g_pActivationFactory ? g_pActivationFactory->GetFactory(activatibleClassId, factory) : E_NOINTERFACE;
	}
	STDAPI DllCanUnloadNow() { return S_FALSE;	}
}

ActivationFactory::ActivationFactory(const wchar_t *className) : _next(g_pActivationFactory), _className(className) {
	g_pActivationFactory = this;
}

}

#if defined(_M_IX86)
#pragma comment(linker, "/EXPORT:DllGetActivationFactory=_DllGetActivationFactory@8,PRIVATE")
#elif defined(_M_ARM) || defined(_M_AMD64)
#pragma comment(linker, "/EXPORT:DllGetActivationFactory,PRIVATE")
#endif
