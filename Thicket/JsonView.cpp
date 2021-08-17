
#include "pch.h"
#include "JsonView.h"
#include "framework.h"
#include "MainFrm.h"
#include "ThicketDoc.h"
#include "Resource.h"
#include "Thicket.h"


class CClassViewMenuButton : public CMFCToolBarMenuButton
{
	friend class CJsonView;

	DECLARE_SERIAL(CClassViewMenuButton)

public:
	CClassViewMenuButton(HMENU hMenu = nullptr) noexcept : CMFCToolBarMenuButton((UINT)-1, hMenu, -1)
	{
	}

	virtual void OnDraw(CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz = TRUE,
		BOOL bCustomizeMode = FALSE, BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE)
	{
		pImages = CMFCToolBar::GetImages();

		CAfxDrawState ds;
		pImages->PrepareDrawImage(ds);

		CMFCToolBarMenuButton::OnDraw(pDC, rect, pImages, bHorz, bCustomizeMode, bHighlight, bDrawBorder, bGrayDisabledButtons);

		pImages->EndDrawImage(ds);
	}
};

IMPLEMENT_SERIAL(CClassViewMenuButton, CMFCToolBarMenuButton, 1)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CJsonView::CJsonView() noexcept
{
	m_nCurrSort = ID_SORTING_GROUPBYTYPE;
}

CJsonView::~CJsonView()
{
}

BEGIN_MESSAGE_MAP(CJsonView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_CLASS_ADD_MEMBER_FUNCTION, OnClassAddMemberFunction)
	ON_COMMAND(ID_CLASS_ADD_MEMBER_VARIABLE, OnClassAddMemberVariable)
	ON_COMMAND(ID_CLASS_DEFINITION, OnClassDefinition)
	ON_COMMAND(ID_CLASS_PROPERTIES, OnClassProperties)
	ON_COMMAND(ID_NEW_FOLDER, OnNewFolder)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_COMMAND_RANGE(ID_SORTING_GROUPBYTYPE, ID_SORTING_SORTBYACCESS, OnSort)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SORTING_GROUPBYTYPE, ID_SORTING_SORTBYACCESS, OnUpdateSort)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJsonView message handlers

int CJsonView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create views:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_treeControl.Create(dwViewStyle, rectDummy, this, 2))
	{
		TRACE0("Failed to create Class View\n");
		return -1;      // fail to create
	}

	// Load images:
	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_SORT);
	m_wndToolBar.LoadToolBar(IDR_SORT, 0, 0, TRUE /* Is locked */);

	OnChangeVisualStyle();

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	CMenu menuSort;
	menuSort.LoadMenu(IDR_POPUP_SORT);

	m_wndToolBar.ReplaceButton(ID_SORT_MENU, CClassViewMenuButton(menuSort.GetSubMenu(0)->GetSafeHmenu()));

	CClassViewMenuButton* pButton =  DYNAMIC_DOWNCAST(CClassViewMenuButton, m_wndToolBar.GetButton(0));

	if (pButton != nullptr)
	{
		pButton->m_bText = FALSE;
		pButton->m_bImage = TRUE;
		pButton->SetImage(GetCmdMgr()->GetCmdImage(m_nCurrSort));
		pButton->SetMessageWnd(this);
	}

	// Fill in some static tree view data (dummy code, nothing magic here)
	PopulateView();

	return 0;
}


void CJsonView::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	// never received
	CDockablePane::OnActivate(nState, pWndOther, bMinimized);

	PopulateView();
}

void CJsonView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

HTREEITEM jsonToTree(json const& val, CTreeCtrl& tree, HTREEITEM hParent)
{
	//for (auto const& val : j)
	{
		CString str (to_string(val).c_str());
		auto type = val.type();
		switch (type)
		{
		case nlohmann::detail::value_t::object:
		{
			hParent = tree.InsertItem(_T("object"), hParent, 0);
			for (auto const& p : val)
			{
				jsonToTree(p, tree, hParent);
				//CString str(to_string(p).c_str());
				//tree.InsertItem(str, hParent, 0);
			}
		}
		return hParent;

		case nlohmann::detail::value_t::array:
		{
			hParent = tree.InsertItem(_T("array"), hParent, 0);
			for (auto const& p : val)
			{
				jsonToTree(p, tree, hParent);
				//CString str(to_string(p).c_str());
				//tree.InsertItem(str, hParent, 0);
			}
		}
		return hParent;

		case nlohmann::detail::value_t::string:
		{
			//val.get<nlohmann::detail::value_t::string>();
			return tree.InsertItem(str, hParent, 0);
		}

		case nlohmann::detail::value_t::binary:
		{
			return tree.InsertItem(str, hParent, 0);
		}

		case nlohmann::detail::value_t::boolean:
		{
			//         if (val.m_value.boolean)
			//         {
					 //	return tree.InsertItem(_T("true"), 0, 0);
					 //}
			//         else
			{
				return tree.InsertItem(str, hParent, 0);
			}
		}

		case nlohmann::detail::value_t::number_integer:
		{
			//auto i = val.m_value.number_integer;
			return tree.InsertItem(str, hParent, 0);
		}

		case nlohmann::detail::value_t::number_unsigned:
		{
			//dump_integer(val.m_value.number_unsigned);
			return tree.InsertItem(str, hParent, 0);
		}

		case nlohmann::detail::value_t::number_float:
		{
			//dump_float(val.m_value.number_float);
			return tree.InsertItem(str, hParent, 0);
		}

		case nlohmann::detail::value_t::discarded:
		{
			return tree.InsertItem(_T("<discarded>"), hParent, 0);
		}

		case nlohmann::detail::value_t::null:
		{
			return tree.InsertItem(_T("null"), hParent, 0);
		}
		}

		return NULL;
	}
}

void CJsonView::PopulateView()
{
	m_treeControl.LockWindowUpdate();

	m_treeControl.DeleteAllItems();

	auto* pDoc = theApp.GetActiveDocument();
	if (!pDoc)
	{
		m_treeControl.UnlockWindowUpdate();
		return;
	}

	auto pTree = pDoc->m_demo.getTree();

	CString strName(pTree->name.c_str());
	if (strName.IsEmpty()) strName = _T("No name");
	HTREEITEM hRoot = m_treeControl.InsertItem(strName, 0, 0);
	m_treeControl.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);

	json j;
	pTree->to_json(j);

	hRoot = jsonToTree(j, m_treeControl, 0);
	m_treeControl.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);
	m_treeControl.Expand(hRoot, TVE_EXPAND);

	hRoot = m_treeControl.InsertItem(_T("Transforms"), 0, 0);
	m_treeControl.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);

	for (auto const& t : pTree->transforms)
	{
		CString sz(t.transformMatrixKey.c_str());
		HTREEITEM hSrc = m_treeControl.InsertItem(sz, 0, 0, hRoot);
	}

	m_treeControl.Expand(hRoot, TVE_EXPAND);

	m_treeControl.UnlockWindowUpdate();
}

void CJsonView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	PopulateView();

	CTreeCtrl* pWndTree = &m_treeControl;
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
	CMenu menu;
	menu.LoadMenu(IDR_POPUP_SORT);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}
}

void CJsonView::AdjustLayout()
{
	if (GetSafeHwnd() == nullptr)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_treeControl.SetWindowPos(nullptr, rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

BOOL CJsonView::PreTranslateMessage(MSG* pMsg)
{
	return CDockablePane::PreTranslateMessage(pMsg);
}

void CJsonView::OnSort(UINT id)
{
	if (m_nCurrSort == id)
	{
		return;
	}

	m_nCurrSort = id;

	CClassViewMenuButton* pButton =  DYNAMIC_DOWNCAST(CClassViewMenuButton, m_wndToolBar.GetButton(0));

	if (pButton != nullptr)
	{
		pButton->SetImage(GetCmdMgr()->GetCmdImage(id));
		m_wndToolBar.Invalidate();
		m_wndToolBar.UpdateWindow();
	}
}

void CJsonView::OnUpdateSort(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pCmdUI->m_nID == m_nCurrSort);
}

void CJsonView::OnClassAddMemberFunction()
{
	AfxMessageBox(_T("Add member function..."));
}

void CJsonView::OnClassAddMemberVariable()
{
	// TODO: Add your command handler code here
}

void CJsonView::OnClassDefinition()
{
	// TODO: Add your command handler code here
}

void CJsonView::OnClassProperties()
{
	// TODO: Add your command handler code here
}

void CJsonView::OnNewFolder()
{
	PopulateView();
}

void CJsonView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectTree;
	m_treeControl.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CJsonView::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_treeControl.SetFocus();
}

void CJsonView::OnChangeVisualStyle()
{
	m_ClassViewImages.DeleteImageList();

	UINT uiBmpId = theApp.m_bHiColorIcons ? IDB_CLASS_VIEW_24 : IDB_CLASS_VIEW;

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

	m_ClassViewImages.Create(16, bmpObj.bmHeight, nFlags, 0, 0);
	m_ClassViewImages.Add(&bmp, RGB(255, 0, 0));

	m_treeControl.SetImageList(&m_ClassViewImages, TVSIL_NORMAL);

	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_SORT_24 : IDR_SORT, 0, 0, TRUE /* Locked */);
}

