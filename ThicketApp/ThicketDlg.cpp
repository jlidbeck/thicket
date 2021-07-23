
// ThicketDlg.cpp : implementation file
//

#include "framework.h"
#include "ThicketApp.h"
#include "ThicketDlg.h"
#include "afxdialogex.h"
#include <future>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#pragma region CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

#pragma endregion


// CThicketDlg dialog



CThicketDlg::CThicketDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_THICKETAPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CThicketDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TRANSFORMS, m_transformsList);
	DDX_Control(pDX, IDC_IMAGE, m_matView);

	if (pDX->m_bSaveAndValidate)
	{
		// update data from view
	}
	else
	{
		// update view from data
	}

}


enum                                        // Custom windows messages
{
	WM_RUN_STARTED = WM_APP,                // 0x8000
	WM_RUN_COMPLETE,                        // 
	WM_RUN_PROGRESS
};


BEGIN_MESSAGE_MAP(CThicketDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_GETMINMAXINFO()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_FILE_OPENPREVIOUS, OnFileOpenPrevious)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_BN_CLICKED(IDC_RANDOMIZE, &CThicketDlg::OnBnClickedRandomize)
	ON_BN_CLICKED(IDC_STEP, &CThicketDlg::OnBnClickedStep)
	ON_BN_CLICKED(IDC_START, &CThicketDlg::OnBnClickedStart)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_TRANSFORMS, &CThicketDlg::OnLvnGetdispinfoTransforms)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TRANSFORMS, &CThicketDlg::OnLvnItemchangedTransforms)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_TRANSFORMS, &CThicketDlg::OnNMCustomdrawTransforms)
	ON_NOTIFY(NM_CLICK, IDC_TRANSFORMS, &CThicketDlg::OnNMClickTransforms)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_TRANSFORMS, &CThicketDlg::OnLvnColumnclickTransforms)
	ON_MESSAGE(WM_RUN_PROGRESS, &CThicketDlg::OnRunProgress)
	ON_STN_CLICKED(IDC_IMAGE, &CThicketDlg::OnStnClickedImage)
END_MESSAGE_MAP()


// CThicketDlg message handlers

#pragma region boilerplate

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CThicketDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CThicketDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CThicketDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch (nID & 0xFFF0)
	{
	case IDM_ABOUTBOX:
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
		return;
	}

	default:
		printf("SYSCMD %04x %04x    %d %d\n", nID, lParam, nID, lParam);
		CDialogEx::OnSysCommand(nID, lParam);
	}
}


#pragma endregion

void CThicketDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}

void CThicketDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize = m_minDialogSize;

	CDialog::OnGetMinMaxInfo(lpMMI);
}


BOOL CThicketDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_transformsList.SetExtendedStyle(m_transformsList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_transformsList.InsertColumn(0, L"", LVCFMT_LEFT, 60);
	m_transformsList.InsertColumn(1, L"freq", LVCFMT_RIGHT, 60);
	m_transformsList.InsertColumn(2, L"gest", LVCFMT_RIGHT, 100);
	m_transformsList.InsertColumn(3, L"key", LVCFMT_LEFT, 100);
	m_transformsList.InsertColumn(4, L"color", LVCFMT_LEFT | LVCFMT_FILL, 100);

	demo.m_progressCallback = [&](int w, int l) {
		::PostMessage(m_hWnd, WM_RUN_PROGRESS, w, l);
		return 0; };

	CRect rc;
	GetWindowRect(&rc);
	ScreenToClient(&rc);
	m_minDialogSize = rc.Size();

	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CThicketDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
}


void CThicketDlg::OnFileOpenPrevious()
{
	demo.openPrevious();
	UpdateData(0);
}

void CThicketDlg::OnFileSave()
{
	demo.save();
}


BOOL CThicketDlg::PreTranslateMessage(MSG *pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		auto virtKey = pMsg->wParam;

		switch (virtKey)
		{
		case VK_F5:
			UpdateData(0);
			Invalidate();
			return true;
		}

		std::lock_guard lock(demo_mutex);

		bool processed = false;
		{
			//std::unique_lock<std::mutex> lock(demo_mutex);

			BYTE scanCode = (pMsg->lParam >> 16) & 0xFF;
			BYTE keyboardState[256];
			::GetKeyboardState(keyboardState);
			WORD ascii = 0;
			int len = ::ToAscii(virtKey, scanCode, keyboardState, &ascii, 0);
			processed = (
				(len > 0 && demo.processKey(ascii))
				|| demo.processKey(virtKey << 16));
		}

		if (processed)
		{
			Invalidate();
			//UpdateWindow();

			if (demo.m_quit)
			{
				EndDialog(IDOK);
				return true;
			}

			return true;
		}
		else 
		{
			//if (len)
			//{
			//	demo.showCommands();
			//}
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


LRESULT CThicketDlg::OnRunProgress(WPARAM w, LPARAM l)
{
	std::lock_guard lock(demo_mutex);

	// update view

	if (demo.pTree != nullptr)
	{
		CString name(demo.pTree->name.c_str());
		SetWindowText(name);

		m_sortOrder.resize(demo.pTree->transforms.size());
		std::iota(m_sortOrder.begin(), m_sortOrder.end(), 0);
		m_sortColumn = -1;
		m_transformsList.SetItemCount(demo.pTree->transforms.size());

		CString str;
		str.Format(L"Q:%zu", demo.pTree->nodeQueue.size());
		if (demo.m_randomize) str += " randomize";
		if (demo.m_restart) str += " restart";
		if (demo.m_stepping) str += " step";
		if (demo.isWorkerTaskRunning()) str += " RUNNING";
		SetDlgItemText(IDC_STATUS, str);

		m_matView.SetImage(demo.canvas.image);
	}
	else
	{
		SetDlgItemText(IDC_STATUS, L"Nothing");
	}
	//UpdateWindow();

	return 0;
}


void CThicketDlg::OnBnClickedRandomize()
{
	demo.restart(true);
	UpdateData(0);
	Invalidate();

}


void CThicketDlg::OnBnClickedStep()
{
	demo.beginStepMode();
	UpdateData(0);
	Invalidate();
}


void CThicketDlg::OnBnClickedStart()
{
	demo.endStepMode();
	UpdateData(0);
	Invalidate();
}


#pragma region Transforms listctrl


void CThicketDlg::OnLvnGetdispinfoTransforms(NMHDR *pNMHDR, LRESULT *pResult)
{
	std::lock_guard lock(demo_mutex);

	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	LV_ITEM* pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_PARAM)
	{
		pItem->lParam = 0;
	}

	if (pItem->mask & LVIF_TEXT)
	{
		if (pItem->iItem < 0 || pItem->iItem >= demo.pTree->transforms.size())
		{
			::_tcscpy_s(pItem->pszText, pItem->cchTextMax, L"");
		}
		else
		{
			auto const& t = demo.pTree->transforms[m_sortOrder[pItem->iItem]];

			switch (pItem->iSubItem)
			{
			case 0: // checkbox
				break;

			case 1:
				::wnsprintf(pItem->pszText, pItem->cchTextMax, L"%d", demo.pTree->transformCounts[t.transformMatrixKey]);
				break;

			case 2: // gestation
			{
				CString str;
				str.Format(L"%.3f", t.gestation);
				::wnsprintf(pItem->pszText, pItem->cchTextMax, str);
				break;
			}

			case 3:
				::wnsprintf(pItem->pszText, pItem->cchTextMax, CString(t.transformMatrixKey.c_str()));
				break;

			case 4:
				::wnsprintf(pItem->pszText, pItem->cchTextMax, CString(t.colorTransform.description().c_str()));
				break;

			}
		}
	}

	if (pItem->mask & LVIF_IMAGE)
	{
		if (pItem->iItem < 0 || pItem->iItem >= demo.pTree->transforms.size())
		{
		}
		else
		{
			//To enable check box, we have to enable state mask...
			pItem->mask |= LVIF_STATE;
			pItem->stateMask = LVIS_STATEIMAGEMASK;
			auto const& t = demo.pTree->transforms[pItem->iItem];
			// 0=no checkbox, 1=unchecked, 2=checked
			pItem->state = INDEXTOSTATEIMAGEMASK(1 ? (t.gestation > 0 ? 2 : 1) : 0);
		}
	}

	*pResult = 0;
}


void CThicketDlg::OnLvnItemchangedTransforms(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


void CThicketDlg::OnNMCustomdrawTransforms(NMHDR *pNMHDR, LRESULT *pResult)
{
	std::lock_guard lock(demo_mutex);

	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)pNMHDR;

	*pResult = CDRF_DODEFAULT;

	static COLORREF oldBkColor = 0xFFFFFF;

	int item = (int)lplvcd->nmcd.dwItemSpec;
	auto const *t = (item >= 0 && item < m_sortOrder.size() ? &demo.pTree->transforms[m_sortOrder[item]] : nullptr);

	switch (pNMCD->dwDrawStage)
	{
	case CDDS_PREPAINT:
		//if (lplvcd->iSubItem == 4)
		if(demo.pTree && item<demo.pTree->transforms.size())
		{
			float h, l, s, a;
			if (t && t->colorTransform.asHlsSink(h, l, s, a))
			{
				auto bgr = util::cvtColor(cv::Scalar(h, l, s), cv::ColorConversionCodes::COLOR_HLS2BGR);
				COLORREF color = RGB(bgr[2], bgr[1], bgr[0]);

				CDC* pdc = CDC::FromHandle(pNMCD->hdc);
				CRect rc = pNMCD->rc;
				rc.DeflateRect(0, 1);
				pdc->FillSolidRect(&rc, color);
				rc.DeflateRect(1, 1);
				pdc->FillSolidRect(&rc, 0xffffff);
			}
		}
		*pResult |= CDRF_NOTIFYITEMDRAW; // request item draw notify 
		break;

	case CDDS_ITEMPREPAINT:
		*pResult |= CDRF_NOTIFYSUBITEMDRAW; // request sub-item draw notify
		break;

	case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // subitem pre-paint
		*pResult |= CDRF_NOTIFYPOSTPAINT; // request post-paint notify
		break;

	case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM: // subitem post-paint
		if (lplvcd->iSubItem == 4)
		{
			float h, l, s, a;
			if( t && t->colorTransform.asHlsSink(h, l, s, a))
			{
				CDC* pdc = CDC::FromHandle(pNMCD->hdc);
				CRect rc = pNMCD->rc;
				rc.DeflateRect(0, 1);
				pdc->FillSolidRect(&rc, 0x000000);
				rc.DeflateRect(1, 1);
				auto bgr = util::cvtColor(cv::Scalar(h, l, s), cv::ColorConversionCodes::COLOR_HLS2BGR);
				COLORREF color = RGB(bgr[2], bgr[1], bgr[0]);
				pdc->FillSolidRect(&rc, color);
			}
		}
		break;

	default:
		printf("%08x", pNMCD->dwDrawStage);
	}
	*pResult = 0;
}


void CThicketDlg::OnNMClickTransforms(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	int i = pNMLV->iItem;
	if (i >= 0 && i < demo.pTree->transforms.size())
	{
		CPoint pt;
		if (::GetCursorPos(&pt))
		{
			m_transformsList.ScreenToClient(&pt);
			CRect itemRect;
			m_transformsList.GetItemRect(i, &itemRect, 0);
			switch (pNMLV->iSubItem)
			{
			case 0:     // checkbox, Defect Type Group
				if (pt.x < 22)
				{
					//// checkbox clicked
					//m_listEntries[i].checked = !m_listEntries[i].checked;
					//m_transformsList.RedrawItems(i, i);
					//OnApply();
					//SetCapture();
				}
				break;

			case 2:     // colorbox
				// colorbox clicked
				//if (m_listEntries[i].color != CToolsDefectColorDialog::COLOR_DISABLED)
				//{
				//	m_listEntries[i].color = DoColorPickerModal(m_listEntries[i].color);
				//	m_transformsList.RedrawItems(i, i);
				//	OnApply();
				//}
				break;

			}
		}
	}

	*pResult = 0;
}


void CThicketDlg::OnLvnColumnclickTransforms(NMHDR *pNMHDR, LRESULT *pResult)
{
	std::lock_guard lock(demo_mutex);

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (!demo.pTree) return;
	auto const &t = demo.pTree->transforms;

	if (pNMLV->iSubItem == m_sortColumn)
	{
		std::reverse(m_sortOrder.begin(), m_sortOrder.end());
	}
	else
	{
		m_sortColumn = pNMLV->iSubItem;
		switch (m_sortColumn)
		{
		case 0:
			std::sort(m_sortOrder.begin(), m_sortOrder.end(),
				[&](size_t i1, size_t i2) { return i1 < i2; });
			break;

		case 1: // freq
			std::sort(m_sortOrder.begin(), m_sortOrder.end(),
				[&](size_t i1, size_t i2) { return demo.pTree->transformCounts[t[i1].transformMatrixKey] < demo.pTree->transformCounts[t[i2].transformMatrixKey]; });
			break;

		case 2: // gestation
			std::sort(m_sortOrder.begin(), m_sortOrder.end(),
				[&](size_t i1, size_t i2) { return t[i1].gestation < t[i2].gestation; });
			break;

		case 3: // key
			std::sort(m_sortOrder.begin(), m_sortOrder.end(),
				[&](size_t i1, size_t i2) { return t[i1].transformMatrixKey < t[i2].transformMatrixKey; });
		}
	}

	m_transformsList.RedrawItems(0, (int)m_transformsList.GetItemCount() - 1);

	*pResult = 0;
}


#pragma endregion

void CThicketDlg::OnStnClickedImage()
{
	// TODO: Add your control notification handler code here
}

