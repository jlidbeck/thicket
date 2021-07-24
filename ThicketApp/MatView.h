#pragma once
#include <afxwin.h>
#include <opencv2/opencv.hpp>
#include <functional>


class CMatView : public CStatic
{
    cv::Mat m_mat;
public:
    std::function<void(UINT, CPoint)> m_onLButtonDown = nullptr;

    void SetImage(cv::Mat const& mat);

private:
    DECLARE_MESSAGE_MAP()
    HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

