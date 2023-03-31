
// ThicketView.cpp : implementation of the CThicketView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Thicket.h"
#endif

#include "ThicketDoc.h"
#include "ThicketView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CThicketView

IMPLEMENT_DYNCREATE(CThicketView, CScrollView)

BEGIN_MESSAGE_MAP(CThicketView, CScrollView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CThicketView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_DESTROY()
	ON_WM_CHAR()
	ON_WM_ACTIVATE()
	ON_WM_SHOWWINDOW()
	ON_WM_CREATE()
END_MESSAGE_MAP()

// CThicketView construction/destruction

CThicketView::CThicketView() noexcept
{
}

CThicketView::~CThicketView()
{
	auto pDoc = GetDocument();
	if (pDoc)
	{
		pDoc->m_demo.endWorkerTask();
	}
}

BOOL CThicketView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}

// CThicketView drawing

void CThicketView::OnDraw(CDC* pDC)
{
	CThicketDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	auto& demo = pDoc->m_demo;
	m_mat = demo.canvas.getImage();

	if (m_mat.empty())
	{
	}
	else
	{
		BITMAPINFOHEADER bi = { sizeof(bi) };
		bi.biWidth = m_mat.cols;
		bi.biHeight = -m_mat.rows;
		bi.biBitCount = (WORD)(m_mat.channels() * 8);
		bi.biPlanes = 1;

		CRect clip;
		GetClipBox(*pDC, &clip);

		//CBrush brush(0x555555);
		//pDC->FillRect(&clip, &brush);

		//SetDIBitsToDevice(*pDC, 0, 0, m_mat.cols, m_mat.rows, 0, 0, 0, m_mat.rows,
		//    m_mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

		StretchDIBits(*pDC,
			0, 0, m_mat.cols, m_mat.rows,
			0, 0, m_mat.cols, m_mat.rows,
			m_mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY);

		pDC->SelectClipRgn(nullptr);
	}
}

void CThicketView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	auto pDoc = GetDocument();
	if (pDoc)
	{
		auto pTree = pDoc->m_demo.getTree();
		if (pTree != nullptr)
		{
			m_mat = pDoc->m_demo.canvas.getImage();
			CSize sz(m_mat.cols, m_mat.rows);
			SetScrollSizes(MM_TEXT, sz);
		}
	}
}


// CThicketView printing


void CThicketView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CThicketView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CThicketView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CThicketView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CThicketView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CThicketView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CThicketView diagnostics

#ifdef _DEBUG
void CThicketView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CThicketView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CThicketDoc* CThicketView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CThicketDoc)));
	return (CThicketDoc*)m_pDocument;
}
#endif //_DEBUG


// CThicketView message handlers


int CThicketView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here

	return 0;
}


void CThicketView::OnDestroy()
{
	auto pDoc = GetDocument();
	if (pDoc)
	{
		pDoc->m_demo.endWorkerTask();
	}

	CScrollView::OnDestroy();

	// TODO: Add your message handler code here
}


void CThicketView::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CScrollView::OnShowWindow(bShow, nStatus);

	auto pDoc = GetDocument();
	if (pDoc)
	{
		if (bShow)
		{
			pDoc->m_demo.m_progressCallback = [&](int w, int l) {
				Invalidate(0);
				//::PostMessage(m_hWnd, WM_RUN_PROGRESS, w, l);
				return 0; };
		}
		else
		{
			pDoc->m_demo.m_progressCallback = nullptr;
		}
	}
}


void CThicketView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	auto pDoc = GetDocument();
	if (pDoc)
	{
		bool processed = false;
		//{
		//	//std::unique_lock<std::mutex> lock(m_mutex);

		//	BYTE scanCode = (pMsg->lParam >> 16) & 0xFF;
		//	BYTE keyboardState[256];
		//	::GetKeyboardState(keyboardState);
		//	WORD ascii = 0;
		//	int len = ::ToAscii(virtKey, scanCode, keyboardState, &ascii, 0);
		//	processed = (
		//		(len > 0 && m_demo.processKey(ascii))
		//		|| m_demo.processKey(virtKey << 16));
		//}

		processed = pDoc->m_demo.processKey(nChar);
		if (processed) return;
	}

	CScrollView::OnChar(nChar, nRepCnt, nFlags);
}
