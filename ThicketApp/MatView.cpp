#include "MatView.h"
using std::min;
using std::max;
#include <atlimage.h>


void CMatView::SetImage(cv::Mat const& mat)
{
    m_mat = mat;
    Invalidate();
}


BEGIN_MESSAGE_MAP(CMatView, CStatic)
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_CTLCOLOR_REFLECT()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


HBRUSH CMatView::CtlColor(CDC* pDC, UINT nCtlColor)
{
    pDC->SetBkMode(TRANSPARENT);
    return (HBRUSH)GetStockObject(NULL_BRUSH);
}


BOOL CMatView::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}


void CMatView::OnPaint()
{
    if (m_mat.empty())
    {
        //CDC *pdc = CDC::FromHandle(lpDrawItemStruct->hDC);
        //pdc->FillSolidRect(&lpDrawItemStruct->rcItem, 0x0000FF);
    }
    else
    {
        CPaintDC dc(this);

        BITMAPINFOHEADER bi = { sizeof(bi) };
        bi.biWidth = m_mat.cols;
        bi.biHeight = -m_mat.rows;
        bi.biBitCount = (WORD)(m_mat.channels() * 8);
        bi.biPlanes = 1;

        SetDIBitsToDevice(dc, 0, 0, m_mat.cols, m_mat.rows, 0, 0, 0, m_mat.rows,
            m_mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        return;

        // ?? does something ??
        CRect viewRect;
        GetClientRect(&viewRect);
        auto w = viewRect.Width();
        auto h = viewRect.Height();

        CImage img;
        img.Create(w, h, 24);

        BITMAPINFO bmpinfo;
        bmpinfo.bmiHeader.biBitCount = 24;
        bmpinfo.bmiHeader.biWidth = w;
        bmpinfo.bmiHeader.biHeight = h;
        bmpinfo.bmiHeader.biPlanes = 1;
        bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpinfo.bmiHeader.biCompression = BI_RGB;
        bmpinfo.bmiHeader.biClrImportant = 0;
        bmpinfo.bmiHeader.biClrUsed = 0;
        bmpinfo.bmiHeader.biSizeImage = 0;
        bmpinfo.bmiHeader.biXPelsPerMeter = 0;
        bmpinfo.bmiHeader.biYPelsPerMeter = 0;

        StretchDIBits(
            img.GetDC(),
            0, 0, w, h,
            0, 0, m_mat.cols, m_mat.rows,
            m_mat.data,
            &bmpinfo, DIB_RGB_COLORS, SRCCOPY);

        img.BitBlt(::GetDC(m_hWnd), 0, 0);
        img.ReleaseDC();
    }
}


void CMatView::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    //if (m_mat.empty())
    //{
    //    CDC *pdc = CDC::FromHandle(lpDrawItemStruct->hDC);
    //    pdc->FillSolidRect(&lpDrawItemStruct->rcItem, 0x0000FF);
    //}
    //else
    //{
    //    CImage img;
    //    img.Create(m_mat.cols, m_mat.rows, 24);

    //    CRect viewRect(lpDrawItemStruct->rcItem);
    //    auto w = viewRect.Width();
    //    auto h = viewRect.Height();

    //    BITMAPINFO bmpinfo;
    //    bmpinfo.bmiHeader.biBitCount = 24;
    //    bmpinfo.bmiHeader.biWidth = w;
    //    bmpinfo.bmiHeader.biHeight = h;
    //    bmpinfo.bmiHeader.biPlanes = 1;
    //    bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    //    bmpinfo.bmiHeader.biCompression = BI_RGB;
    //    bmpinfo.bmiHeader.biClrImportant = 0;
    //    bmpinfo.bmiHeader.biClrUsed = 0;
    //    bmpinfo.bmiHeader.biSizeImage = 0;
    //    bmpinfo.bmiHeader.biXPelsPerMeter = 0;
    //    bmpinfo.bmiHeader.biYPelsPerMeter = 0;

    //    StretchDIBits(
    //        img.GetDC(),
    //        0, 0, w, h,
    //        0, 0, m_mat.cols, m_mat.rows, 
    //        m_mat.data, 
    //        &bmpinfo, DIB_RGB_COLORS, SRCCOPY);

    //    img.BitBlt(::GetDC(m_hWnd), 0, 0);
    //    img.ReleaseDC();
    //}
}


void CMatView::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_onLButtonDown != nullptr)
        m_onLButtonDown(nFlags, point);
}
