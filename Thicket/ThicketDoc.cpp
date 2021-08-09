
// ThicketDoc.cpp : implementation of the CThicketDoc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Thicket.h"
#endif

#include "ThicketDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CThicketDoc

IMPLEMENT_DYNCREATE(CThicketDoc, CDocument)

BEGIN_MESSAGE_MAP(CThicketDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE_SETTINGS, &CThicketDoc::OnFileSave)
	ON_COMMAND(ID_FILE_SAVE, &CThicketDoc::OnFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, &CThicketDoc::OnFileSaveAs)
 	ON_COMMAND(ID_FILE_EXPORT_SVG, &CThicketDoc::OnFileExportSVG)
END_MESSAGE_MAP()


// CThicketDoc construction/destruction

CThicketDoc::CThicketDoc() noexcept
{
}

CThicketDoc::~CThicketDoc()
{
}


#ifdef SHARED_HANDLERS

// Support for thumbnails
void CThicketDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CThicketDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CThicketDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl* pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CThicketDoc diagnostics

#ifdef _DEBUG
void CThicketDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CThicketDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG



#pragma region files and serialization

//  Virtual override of CDocument::OnNewDocument.
BOOL CThicketDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // TODO: add reinitialization code here
    // (SDI documents will reuse this document)

    m_demo.restart(true);

    return TRUE;
}


//  Virtual override of CDocument::OnOpenDocument.
//  OnOpenDocument() is invoked by CWinAppEx::OpenDocumentFile.
//  Default implementation opens the file and passes it via CArchive to CThicketDoc::Serialize.
//  Override to allow TreeDemo to manage the files for now
BOOL CThicketDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	m_demo.openSettingsFile(fs::path(lpszPathName));
	CString title(m_demo.getName().c_str());
	SetTitle(title);
	SetModifiedFlag(1);
	UpdateAllViews(nullptr);

    return TRUE;
}


//  Virtual override of CDocument::OnSaveDocument.
//  OnSaveDocument is invoked by {?}, also to save backups in a temporary folder.
//  Default implementation opens the file and passes it via CArchive to CThicketDoc::Serialize.
//  Override to allow TreeDemo to manage the files for now
BOOL CThicketDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    fs::path path(lpszPathName);
    try
    {
        m_demo.save(path);
    }
    catch (std::exception& ex)
    {
        char szErr[250];
        ::strerror_s(szErr, errno);
        CStringA msg;
        msg.Format("saveSettings failed: %s\n\nError: %d\n%s", ex.what(), errno, szErr);

        ::MessageBoxA(*::AfxGetMainWnd(), msg, "Fail", MB_OK);
		return FALSE;
    }

    return TRUE;
}


//  virtual override of CDocument::Serialize
//  Normally this function is called by OnOpenDocument, OnSaveDocument with a CFile already open
void CThicketDoc::Serialize(CArchive& ar)
{
	throw std::exception("CThicketDoc::Serialize is not implemented");

    if (ar.IsStoring())
    {
		json j;
		m_demo.getTree()->to_json(j);
		std::ostringstream oss;
		oss << j;
		CStringA sz(oss.str().c_str());
		ar << sz;
    }
    else
    {
		CStringA sz;
		ar >> sz;
		json j;
		std::istringstream iss;
		iss >> j;
		m_demo.getTree()->from_json(j);
    }
}

#pragma endregion files and serialization

#pragma region File menu handlers

//void CThicketDoc::OnFileSaveSettings()
//{
//	OnFileSave();
//}


//  Handle ID_FILE_SAVE, ID_FILE_SAVE_SETTINGS
//  This is a non-virtual override of CDocument::OnFileSave.
//  The default implementation passes the path to OnSaveDocument
//  Override to customize the file dialog and file filters
void CThicketDoc::OnFileSave()
{
	CString sz = GetPathName();
	if (sz.IsEmpty())
	{
		OnFileSaveAs();
	}
	else
	{
		CDocument::OnFileSave();
	}

}


//  Handle ID_FILE_SAVE_AS
//  This is a non-virtual override of CDocument::OnFileSaveAs.
//  The default implementation opens a file dialog and passes the path to OnSaveDocument
//  Override to customize the file dialog and file filters
void CThicketDoc::OnFileSaveAs()
{
	auto pWnd = ::AfxGetMainWnd();

	CFileDialog dlg(TRUE, L"settings.json", nullptr, OFN_EXPLORER, L"settings.json|*.settings.json||", pWnd);

	CString sz = GetPathName();
	fs::path curPath = fs::absolute(L".");
	if (sz.IsEmpty())
	{
		dlg.m_ofn.lpstrInitialDir = curPath.c_str();
	}
	else
	{
		dlg.m_ofn.lpstrInitialDir = nullptr;
	}
	dlg.m_ofn.lpstrTitle = L"Save";
	dlg.m_ofn.lpstrFile = sz.GetBufferSetLength(MAX_PATH + 1);
	dlg.m_ofn.nMaxFile = sz.GetAllocLength();

	if (IDOK == dlg.DoModal())
	{
		CString path = dlg.GetPathName();
		//this->SetPathName(path, 1);
		this->OnSaveDocument(path);

		SetModifiedFlag(0);
	}
}


void CThicketDoc::OnFileExportSVG()
{
	auto pWnd = ::AfxGetMainWnd();

	CFileDialog dlg(FALSE, L"svg", nullptr, OFN_EXPLORER | OFN_OVERWRITEPROMPT, L"SVG (*.svg)|*.svg||", pWnd);

	fs::path filePath = (LPCTSTR)GetPathName();
	filePath.replace_extension(L"svg");
	if (filePath.empty())
	{
		fs::path curPath = fs::absolute(L".");
		dlg.m_ofn.lpstrInitialDir = curPath.c_str();
	}
	else
	{
		dlg.m_ofn.lpstrInitialDir = filePath.c_str();
	}
	dlg.m_ofn.lpstrTitle = L"Save SVG";
	CString sz(filePath.c_str());
	dlg.m_ofn.lpstrFile = sz.GetBufferSetLength(MAX_PATH + 1);
	dlg.m_ofn.nMaxFile = sz.GetAllocLength();

	if (IDOK == dlg.DoModal())
	{
		fs::path path((LPCTSTR)dlg.GetPathName());
		try
		{
			m_demo.saveSVG(path);
		}
		catch (std::exception& ex)
		{
			char szErr[250];
			::strerror_s(szErr, errno);
			CStringA msg;
			msg.Format("saveSVG failed: %s\n\nError: %d\n%s", ex.what(), errno, szErr);

			::MessageBoxA(*::AfxGetMainWnd(), msg, "Fail", MB_OK);
		}
	}
}


#pragma endregion

