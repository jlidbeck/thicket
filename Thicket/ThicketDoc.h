
// ThicketDoc.h : interface of the CThicketDoc class
//


#pragma once

#include "..\tree\treedemo.h"


class CThicketDoc : public CDocument
{
public:
	TreeDemo m_demo;

protected: // create from serialization only
	CThicketDoc() noexcept;
	DECLARE_DYNCREATE(CThicketDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument() override;
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CThicketDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
protected:
	// file menu commands
	//afx_msg void OnFileSaveSettings();
	// non-virtual CDocument overrides
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileExportSVG();
};
