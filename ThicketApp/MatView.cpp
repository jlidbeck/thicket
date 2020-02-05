#include "MatView.h"
#include "ThicketApp.h"


// CMatView

IMPLEMENT_DYNCREATE(CMatView, CScrollView)

CMatView::CMatView()
{
	SetScrollSizes(MM_TEXT, CSize(100, 100));
}

CMatView::~CMatView()
{
}

void CMatView::SetImage(cv::Mat const& mat)
{
    m_mat = mat;
    if (!mat.empty())
    {
        SetScrollSizes(MM_TEXT, CSize(mat.cols, mat.rows));
    }
    Invalidate();
}


BEGIN_MESSAGE_MAP(CMatView, CScrollView)
END_MESSAGE_MAP()


// CMatView diagnostics

#ifdef _DEBUG
void CMatView::AssertValid() const
{
	CScrollView::AssertValid();
}

#ifndef _WIN32_WCE
void CMatView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}
#endif
#endif //_DEBUG


// CMatView drawing

void CMatView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CSize sizeTotal;
	// TODO: calculate the total size of this view
	sizeTotal.cx = sizeTotal.cy = 100;
	SetScrollSizes(MM_TEXT, sizeTotal);
}


void CMatView::OnDraw(CDC *pDC)
{
	CDocument* pDoc = GetDocument();

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
        CRect clientRect;
        GetClientRect(&clientRect);
        pDC->IntersectClipRect(clientRect);

        //SetDIBitsToDevice(*pDC, 0, 0, m_mat.cols, m_mat.rows, 0, 0, 0, m_mat.rows,
        //    m_mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        StretchDIBits(*pDC, 
            0, 0, m_mat.cols, m_mat.rows, 
            0, 0, m_mat.cols, m_mat.rows, 
            m_mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY);

        pDC->SelectClipRgn(nullptr);
    }
}


// CMatView message handlers


void CMatView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/)
{
	// TODO: Add your specialized code here and/or call the base class
}


