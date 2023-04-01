
#include "pch.h"
#include "framework.h"

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "Thicket.h"
#include "ThicketDoc.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropertiesWnd

CPropertiesWnd::CPropertiesWnd() noexcept
{
	m_nComboHeight = 0;
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_COMMAND(ID_PROPERTIES1, OnProperties1)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
	ON_COMMAND(ID_PROPERTIES2, OnProperties2)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES2, OnUpdateProperties2)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_WM_CONTEXTMENU()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
	ON_WM_UPDATEUISTATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertiesWnd message handlers

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd () == nullptr || (AfxGetMainWnd() != nullptr && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndObjectCombo.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), m_nComboHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top + m_nComboHeight, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_propertyGrid.SetWindowPos(nullptr, rectClient.left, rectClient.top + m_nComboHeight + cyTlb, rectClient.Width(), rectClient.Height() -(m_nComboHeight+cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create combo:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndObjectCombo.Create(dwViewStyle, rectDummy, this, 1))
	{
		TRACE0("Failed to create Properties Combo \n");
		return -1;      // fail to create
	}

	m_wndObjectCombo.AddString(_T("Application"));
	m_wndObjectCombo.AddString(_T("Properties Window"));
	m_wndObjectCombo.SetCurSel(0);

	CRect rectCombo;
	m_wndObjectCombo.GetClientRect (&rectCombo);

	m_nComboHeight = rectCombo.Height();

	if (!m_propertyGrid.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, ID_THICKET_PROPERTY_GRID))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* Is locked */);
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_PROPERTIES_HC : IDR_PROPERTIES, 0, 0, TRUE /* Locked */);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	AdjustLayout();

	// does nothing here
	InitPropList();

	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::OnExpandAllProperties()
{
	m_propertyGrid.ExpandAll();
}

void CPropertiesWnd::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */)
{
}

void CPropertiesWnd::OnSortProperties()
{
	m_propertyGrid.SetAlphabeticMode(!m_propertyGrid.IsAlphabeticMode());
}

void CPropertiesWnd::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_propertyGrid.IsAlphabeticMode());
}

void CPropertiesWnd::OnProperties1()
{
	// TODO: Add your command handler code here
}

void CPropertiesWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CPropertiesWnd::OnProperties2()
{
	// TODO: Add your command handler code here
}

void CPropertiesWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CPropertiesWnd::InitPropList()
{
	SetPropListFont();

	m_propertyGrid.EnableHeaderCtrl(FALSE);
	m_propertyGrid.EnableDescriptionArea();
	m_propertyGrid.SetVSDotNetLook();
	m_propertyGrid.MarkModifiedProperties();

	m_propertyGrid.RemoveAll();

	auto* pDoc = theApp.GetActiveDocument();
	if (!pDoc) return;
	auto pTree = pDoc->m_demo.getTree();

	CMFCPropertyGridProperty* pProp;

	{
		CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Appearance"));

		pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("Name"), (_variant_t)pTree->name.c_str(), _T("Name")));

		pProp = new CMFCPropertyGridProperty(_T("Random Seed"), (_variant_t)pTree->randomSeed, _T("Random seed used by generator to create initial settings"), 42000);
		pProp->EnableSpinControl(TRUE, 0, 0x7FFFFFFF);
		pGroup1->AddSubItem(pProp);

		m_propertyGrid.AddProperty(pGroup1);
	}

	{
		CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Domain"), 0, TRUE);

		auto domain = pTree->domain;

		pProp = new CMFCPropertyGridProperty(_T("X"), (_variant_t)domain.x, _T("Specifies the domain X min"));
		pGroup->AddSubItem(pProp);

		pProp = new CMFCPropertyGridProperty(_T("Y"), (_variant_t)domain.y, _T("Specifies the domain Y min"));
		pGroup->AddSubItem(pProp);

		pProp = new CMFCPropertyGridProperty(_T("Width"), (_variant_t)domain.width, _T("Specifies the domain width"));
		pGroup->AddSubItem(pProp);

		pProp = new CMFCPropertyGridProperty(_T("Height"), (_variant_t)domain.height, _T("Specifies the domain height"));
		pGroup->AddSubItem(pProp);

		pProp = new CMFCPropertyGridProperty(_T("Shape"), (_variant_t)(int)pTree->domainShape, _T("One of: None, Thin, Resizable, or Dialog Frame"));
		pProp->AddOption(_T("RECT"));
		pProp->AddOption(_T("ELLIPSE"));
		pProp->AllowEdit(FALSE);
		pGroup->AddSubItem(pProp);

		m_propertyGrid.AddProperty(pGroup);
	}

	{
		CMFCPropertyGridProperty* pDrawSettingsGroup = new CMFCPropertyGridProperty(_T("Drawing"));

		RenderSettings const& rs = pDoc->m_demo.getRenderSettings();
		pProp = new CMFCPropertyGridProperty(_T("Render Size X"), (_variant_t)rs.renderSize.width, _T("Render size X"), 42001);
		pDrawSettingsGroup->AddSubItem(pProp);
		pProp = new CMFCPropertyGridProperty(_T("Render Size Y"), (_variant_t)rs.renderSize.height, _T("Render size Y"), 42002);
		pDrawSettingsGroup->AddSubItem(pProp);

		pProp = new CMFCPropertyGridProperty(_T("Line Thickness"), (_variant_t)pTree->lineThickness, _T("Polygon outline line thickness"), 42005);
		pProp->EnableSpinControl(TRUE, 0, 10);
		pDrawSettingsGroup->AddSubItem(pProp);

		CMFCPropertyGridColorProperty* pColorProp = new CMFCPropertyGridColorProperty(
			_T("Line Color"),
			RGB((int)(0.5 + 255.0 * pTree->lineColor[2]), 
				(int)(0.5 + 255.0 * pTree->lineColor[1]), 
				(int)(0.5 + 255.0 * pTree->lineColor[0])),
			0,
			_T("Polygon outline line color"),
			42010);
		pColorProp->EnableOtherButton(_T("Other..."));
		pColorProp->EnableAutomaticButton(_T("Default"), RGB(255, 255, 255));
		pDrawSettingsGroup->AddSubItem(pColorProp);

		m_propertyGrid.AddProperty(pDrawSettingsGroup);
		pDrawSettingsGroup->Expand(1);
	}

	{
		CMFCPropertyGridProperty* pGeometryGroup = new CMFCPropertyGridProperty(_T("Geometry"));

		pProp = new CMFCPropertyGridProperty(_T("Polygon"), (_variant_t)(int)pTree->polygon.size(), _T("Primary polygon"));
		pProp->AllowEdit(0);
		pGeometryGroup->AddSubItem(pProp);

		CString groupName;
		groupName.Format(_T("Transforms [%zu]"), pTree->transforms.size());

		CMFCPropertyGridProperty* pTransformsGroup = new CMFCPropertyGridProperty(groupName, 0, TRUE);

		for (auto const& t : pTree->transforms)
		{
			CString name;
			auto det = cv::determinant(t.transformMatrix);
			name.Format(_T("%hs (det=%lf)"), t.transformMatrixKey.c_str(), det);

			cv::Scalar_<float> hlsa;
			if (t.colorTransform.asHlsSink(hlsa[0], hlsa[1], hlsa[2], hlsa[3]))
			{
				cv::Scalar bgra = util::cvtColor(hlsa, cv::ColorConversionCodes::COLOR_HLS2BGR);
				CMFCPropertyGridColorProperty* pColorProp = new CMFCPropertyGridColorProperty(
					name,
					RGB((int)(0.5 + 255.0 * bgra[2]),
						(int)(0.5 + 255.0 * bgra[1]),
						(int)(0.5 + 255.0 * bgra[0])),
					0,
					_T("Color Sink"));
				pTransformsGroup->AddSubItem(pColorProp);
			}
			else
			{
				pProp = new CMFCPropertyGridProperty(name, (_variant_t)det, _T("2D Transform"));
				pProp->AllowEdit(0);
				pTransformsGroup->AddSubItem(pProp);
			}
		}

		pGeometryGroup->AddSubItem(pTransformsGroup);

		pGeometryGroup->Expand(1);
		m_propertyGrid.AddProperty(pGeometryGroup);
	}

	{
		CMFCPropertyGridProperty* pGroup2 = new CMFCPropertyGridProperty(_T("Font"));

		LOGFONT lf;
		CFont* font = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
		font->GetLogFont(&lf);

		_tcscpy_s(lf.lfFaceName, _T("Arial"));

		pGroup2->AddSubItem(new CMFCPropertyGridFontProperty(_T("Font"), lf, CF_EFFECTS | CF_SCREENFONTS, _T("Specifies the default font for the window")));
		pGroup2->AddSubItem(new CMFCPropertyGridProperty(_T("Use System Font"), (_variant_t)true, _T("Specifies that the window uses MS Shell Dlg font")));

		m_propertyGrid.AddProperty(pGroup2);
	}

	{
		CMFCPropertyGridProperty* pGroup3 = new CMFCPropertyGridProperty(_T("Misc"));
		pProp = new CMFCPropertyGridProperty(_T("(Name)"), _T("Application"));
		pProp->Enable(FALSE);
		pGroup3->AddSubItem(pProp);

		static const TCHAR szFilter[] = _T("Icon Files(*.ico)|*.ico|All Files(*.*)|*.*||");
		pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Icon"), TRUE, _T(""), _T("ico"), 0, szFilter, _T("Specifies the window icon")));

		pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Folder"), _T("c:\\")));

		m_propertyGrid.AddProperty(pGroup3);
	}

	{
		CMFCPropertyGridProperty* pGroup4 = new CMFCPropertyGridProperty(_T("Hierarchy"));

		CMFCPropertyGridProperty* pGroup41 = new CMFCPropertyGridProperty(_T("First sub-level"));
		pGroup4->AddSubItem(pGroup41);

		CMFCPropertyGridProperty* pGroup411 = new CMFCPropertyGridProperty(_T("Second sub-level"));
		pGroup41->AddSubItem(pGroup411);

		pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 1"), (_variant_t)_T("Value 1"), _T("This is a description")));
		pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 2"), (_variant_t)_T("Value 2"), _T("This is a description")));
		pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 3"), (_variant_t)_T("Value 3"), _T("This is a description")));

		pGroup4->Expand(FALSE);
		m_propertyGrid.AddProperty(pGroup4);
	}
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_propertyGrid.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_propertyGrid.SetFont(&m_fntPropList);
	m_wndObjectCombo.SetFont(&m_fntPropList);
}


void CPropertiesWnd::OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/)
{
	InitPropList();
}

LRESULT CPropertiesWnd::OnPropertyChanged(
	__in WPARAM wparam,
	__in LPARAM lparam)
{
	// Parameters:
	// [in] wparam: the control ID of the CMFCPropertyGridCtrl that changed.
	// [in] lparam: pointer to the CMFCPropertyGridProperty that changed.

	// Return value:
	// Not used.

	// Cast the lparam to a property.
	CMFCPropertyGridProperty* pProperty = (CMFCPropertyGridProperty*)lparam;

	//VARIANT varChildren;
	//m_propertyGrid.get_accSelection(&varChildren);

	auto* pDoc = theApp.GetActiveDocument();
	if (!pDoc) 
		return 0;
	auto pTree = pDoc->m_demo.getTree();

	// At this point you could simply pass pProperty to the appropriate object
	// and let it handle its own data validation. However, the wparam can be used
	// to give additional context to the property. For example, if you have two 
	// different class of object in your application, it makes sense to have two
	// seaprate property grid controls (one for each object). The following switch
	// deals with this scenario. 

	// Determine which property grid was changed (wparam is the control ID).
	switch (wparam)
	{
	case ID_THICKET_PROPERTY_GRID:
	{
		auto rs = pDoc->m_demo.getRenderSettings();

		switch (pProperty->GetData())
		{
		case 42000:	// random seed
			pTree->setRandomSeed(pProperty->GetValue().intVal);
			pDoc->m_demo.setModified(true);
			pDoc->m_demo.restart();
			break;
		case 42001:	// render size
			rs.renderSize.width = pProperty->GetValue().intVal;
			pDoc->m_demo.setRenderSettings(rs);
			pDoc->m_demo.restart();
			break;
		case 42002:	// render size
			rs.renderSize.height = pProperty->GetValue().intVal;
			pDoc->m_demo.setRenderSettings(rs);
			pDoc->m_demo.restart();
			break;
		case 42005:	// line thickness
			pTree->lineThickness = pProperty->GetValue().intVal;
			pDoc->m_demo.setRenderSettings(rs);
			pDoc->m_demo.restart();
			break;
		case 42010:	// line color
		{
			COLORREF rgb = pProperty->GetValue().uintVal;
			pTree->lineColor = cv::Scalar(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
			pDoc->m_demo.setRenderSettings(rs);
			pDoc->m_demo.restart();
			break;
		}
		case 42011:	// background color
		{
			COLORREF rgb = pProperty->GetValue().uintVal;
			pTree->backgroundColor = cv::Scalar(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
			pDoc->m_demo.setRenderSettings(rs);
			pDoc->m_demo.restart();
			break;
		}
		}

	}	// case ID_THICKET_PROPERTY_GRID
	break;

	default:
		MessageBox(_T("Something changed"));
	}

	if(pDoc->m_demo.isModified())
		pDoc->SetModifiedFlag(1);

	return 0;
}


//void CPropertiesWnd::OnPropertyChanged(NMHDR* pNMHDR, LRESULT* pResult)
//{
//	CMFCPropertyGridProperty* pProp = reinterpret_cast<CMFCPropertyGridProperty*>(pNMHDR);
//
//	VARIANT varChildren;
//	m_propertyGrid.get_accSelection(&varChildren);
//
//	auto* pDoc = theApp.GetActiveDocument();
//	if (!pDoc) return;
//	auto pTree = pDoc->m_demo.getTree();
//
//	*pResult = 0;
//}



void CPropertiesWnd::OnUpdateUIState(UINT /*nAction*/, UINT /*nUIElement*/)
{
	InitPropList();
}
