
#include "pch.h"
#include "TransformView.h"
#include "framework.h"
#include "mainfrm.h"
#include "Resource.h"
#include "Thicket.h"
#include "ThicketDoc.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CTransformView

CTransformView::CTransformView() noexcept
{
}

CTransformView::~CTransformView()
{
}

BEGIN_MESSAGE_MAP(CTransformView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_PROPERTIES, OnProperties)
	ON_COMMAND(ID_OPEN, OnFileOpen)
	ON_COMMAND(ID_OPEN_WITH, OnFileOpenWith)
	ON_COMMAND(ID_DUMMY_COMPILE, OnDummyCompile)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CTransformView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create view:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;

	if (!m_viewTree.Create(dwViewStyle, rectDummy, this, 4))
	{
		TRACE0("Failed to create transform view\n");
		return -1;      // fail to create
	}

	// Load view images:
	m_FileViewImages.Create(IDB_FILE_VIEW, 16, 0, RGB(255, 0, 255));
	m_viewTree.SetImageList(&m_FileViewImages, TVSIL_NORMAL);

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_EXPLORER);
	m_wndToolBar.LoadToolBar(IDR_EXPLORER, 0, 0, TRUE /* Is locked */);

	OnChangeVisualStyle();

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	// Fill in some static tree view data (dummy code, nothing magic here)
	FillViewTree();
	AdjustLayout();

	return 0;
}

void CTransformView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}


void CTransformView::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDockablePane::OnActivate(nState, pWndOther, bMinimized);

	// TODO: Add your message handler code here
}

void CTransformView::FillViewTree()
{
	m_viewTree.DeleteAllItems();

	HTREEITEM hRoot = m_viewTree.InsertItem(_T("Transforms"), 0, 0);
	m_viewTree.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);

	CFrameWnd* pFrameWnd = dynamic_cast<CFrameWnd*>(::AfxGetMainWnd());
	if (!pFrameWnd) return;
	auto f = pFrameWnd->GetActiveFrame();
	if (!f) return;
	auto *pDoc = dynamic_cast<CThicketDoc*>(f->GetActiveDocument());
	if (!pDoc) return;

	auto pTree = pDoc->m_demo.getTree();

	for (auto const& t : pTree->transforms)
	{
		CString sz(t.transformMatrixKey.c_str());
		HTREEITEM hSrc = m_viewTree.InsertItem(sz, 0, 0, hRoot);
	}

	m_viewTree.Expand(hRoot, TVE_EXPAND);
}

void CTransformView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CTreeCtrl* pWndTree = (CTreeCtrl*) &m_viewTree;
	ASSERT_VALID(pWndTree);

	if (pWnd != pWndTree)
	{
		CDockablePane::OnContextMenu(pWnd, point);
		return;
	}

	if (point != CPoint(-1, -1))
	{
		// Select clicked item:
		CPoint ptTree = point;
		pWndTree->ScreenToClient(&ptTree);

		UINT flags = 0;
		HTREEITEM hTreeItem = pWndTree->HitTest(ptTree, &flags);
		if (hTreeItem != nullptr)
		{
			pWndTree->SelectItem(hTreeItem);
		}
	}

	pWndTree->SetFocus();
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EXPLORER, point.x, point.y, this, TRUE);
}

void CTransformView::AdjustLayout()
{
	if (GetSafeHwnd() == nullptr)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_viewTree.SetWindowPos(nullptr, rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CTransformView::OnProperties()
{
	FillViewTree();
}

void CTransformView::OnFileOpen()
{
	// TODO: Add your command handler code here
	FillViewTree();
}

void CTransformView::OnFileOpenWith()
{
	// TODO: Add your command handler code here
}

void CTransformView::OnDummyCompile()
{
	// TODO: Add your command handler code here
}

void CTransformView::OnEditCut()
{
	// TODO: Add your command handler code here
}

void CTransformView::OnEditCopy()
{
	// TODO: Add your command handler code here
}

void CTransformView::OnEditClear()
{
	// TODO: Add your command handler code here
}

void CTransformView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectTree;
	m_viewTree.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CTransformView::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_viewTree.SetFocus();
}

void CTransformView::OnChangeVisualStyle()
{
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_EXPLORER_24 : IDR_EXPLORER, 0, 0, TRUE /* Locked */);

	m_FileViewImages.DeleteImageList();

	UINT uiBmpId = theApp.m_bHiColorIcons ? IDB_FILE_VIEW_24 : IDB_FILE_VIEW;

	CBitmap bmp;
	if (!bmp.LoadBitmap(uiBmpId))
	{
		TRACE(_T("Can't load bitmap: %x\n"), uiBmpId);
		ASSERT(FALSE);
		return;
	}

	BITMAP bmpObj;
	bmp.GetBitmap(&bmpObj);

	UINT nFlags = ILC_MASK;

	nFlags |= (theApp.m_bHiColorIcons) ? ILC_COLOR24 : ILC_COLOR4;

	m_FileViewImages.Create(16, bmpObj.bmHeight, nFlags, 0, 0);
	m_FileViewImages.Add(&bmp, RGB(255, 0, 255));

	m_viewTree.SetImageList(&m_FileViewImages, TVSIL_NORMAL);
}


