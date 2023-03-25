#pragma once
#include <afxwin.h>
#include <opencv2/opencv.hpp>


class CMatView : public CStatic
{
    cv::Mat m_mat;
public:

    void SetImage(cv::Mat const& mat);

protected:
    virtual void PostNcDestroy() override
    {
        // The default implementation of CView::PostNcDestroy calls "delete this".
        // Our MatView is allocated on the heap, so don't call the base implementation
        // as this would cause memory corruption.
        // CScrollView::PostNcDestroy();
    }
    DECLARE_MESSAGE_MAP()
    HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;
};

