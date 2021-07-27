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
    ON_WM_RBUTTONDOWN()
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


//
//  cv::copyTo with cropping
//  Copies sourceImage onto targetImage, only overlapping region
//
void copyToSafe(const cv::Mat& sourceImage, const cv::Mat& sourceMask, cv::Mat& targetImage, cv::Point topLeft)
{
    // source image rect--start with entire source
    cv::Rect sourceRect(0, 0, sourceImage.cols, sourceImage.rows);

    // dest image rect, uncropped
    cv::Rect destRect(topLeft, sourceImage.size());

    // crop dest rect against target image bounds
    cv::Rect overlappingDestRect = (destRect & cv::Rect(0, 0, targetImage.cols, targetImage.rows));

    if (overlappingDestRect.area() <= 0)
        return;

    // if dest rect was cropped, crop source board image
    if (overlappingDestRect.x > destRect.x)
    {
        sourceRect.x += (overlappingDestRect.x - destRect.x);
    }
    if (overlappingDestRect.y > destRect.y)
    {
        sourceRect.y += (overlappingDestRect.y - destRect.y);
    }
    if (overlappingDestRect.width < destRect.width)
    {
        sourceRect.width -= (destRect.width - overlappingDestRect.width);
    }
    if (overlappingDestRect.height < destRect.height)
    {
        sourceRect.height -= (destRect.height - overlappingDestRect.height);
    }

    // copy masked board image to rgbImage
    sourceImage(sourceRect).copyTo(targetImage(overlappingDestRect), sourceMask(sourceRect));
}

void CMatView::OnPaint()
{
    CPaintDC dc(this);

    CRect dcBounds;
    dc.GetBoundsRect(&dcBounds, 0);
    auto size = dc.GetViewportExt();
    auto viewportOrigin = dc.GetViewportOrg();

    CRect clientRect;
    GetClientRect(&clientRect);

    if (m_mat.empty())
    {
        dc.FillSolidRect(&dcBounds, 0xCCCCCC);
    }
    else
    {
        double scale = 1.0;

        if (scale == 1.0)
        {
            BITMAPINFOHEADER bi = { sizeof(bi) };
            bi.biWidth = m_mat.cols;
            bi.biHeight = -m_mat.rows;
            bi.biBitCount = (WORD)(m_mat.channels() * 8);
            bi.biPlanes = 1;

            SetDIBitsToDevice(dc, 0, 0, m_mat.cols, m_mat.rows, 0, 0, 0, m_mat.rows,
                m_mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        }
        else
        {

            // ?? does something ??
            auto w = clientRect.Width();
            auto h = clientRect.Height();

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

void CMatView::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (m_onRButtonDown != nullptr)
        m_onRButtonDown(nFlags, point);
}
