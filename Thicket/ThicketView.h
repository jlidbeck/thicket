
// ThicketView.h : interface of the CThicketView class
//

#pragma once
#include <opencv2/opencv.hpp>


class CThicketView : public CScrollView
{
protected: // create from serialization only
	CThicketView() noexcept;
	DECLARE_DYNCREATE(CThicketView)

	cv::Mat m_mat;

// Attributes
public:
	CThicketDoc* GetDocument() const;
	//void SetImage(cv::Mat const& mat);

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CThicketView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

#ifndef _DEBUG  // debug version in ThicketView.cpp
inline CThicketDoc* CThicketView::GetDocument() const
   { return reinterpret_cast<CThicketDoc*>(m_pDocument); }
#endif
