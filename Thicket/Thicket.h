
// Thicket.h : main header file for the Thicket application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


class CThicketDoc;


// CThicketApp:
// See Thicket.cpp for the implementation of this class
//

class CThicketApp : public CWinAppEx
{
public:
	CThicketApp() noexcept;

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	CThicketDoc* GetActiveDocument();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppAbout();
	afx_msg void OnFileSaveSettings();
	afx_msg void OnFileOpenSettings();
	afx_msg void OnFileOpenImage();
};

extern CThicketApp theApp;
