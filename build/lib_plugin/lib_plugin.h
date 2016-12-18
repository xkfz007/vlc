// lib_plugin.h : main header file for the lib_plugin DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Clib_pluginApp
// See lib_plugin.cpp for the implementation of this class
//

class Clib_pluginApp : public CWinApp
{
public:
	Clib_pluginApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
