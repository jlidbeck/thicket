#pragma once
#include <afxwin.h>
#include <opencv2/opencv.hpp>


class CMatView : public CScrollView
{
	DECLARE_DYNCREATE(CMatView)

    cv::Mat m_mat;

public:
    CMatView();
	virtual ~CMatView();

    void SetImage(cv::Mat const& mat);

#ifdef _DEBUG
    virtual void AssertValid() const;
#ifndef _WIN32_WCE
    virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
    virtual void PostNcDestroy() override
    {
        // The default implementation of CView::PostNcDestroy calls "delete this".
        // Our MatView is allocated on the heap, so don't call the base implementation
        // as this would cause memory corruption.
        // CScrollView::PostNcDestroy();
    }
    virtual void OnDraw(CDC *pDC) override;
    virtual void OnInitialUpdate();     // first time after construct
    virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
    DECLARE_MESSAGE_MAP()
};

