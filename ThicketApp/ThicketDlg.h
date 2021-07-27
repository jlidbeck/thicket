
// ThicketDlg.h : header file
//

#pragma once


#include <mutex>
#include <opencv2/opencv.hpp>
#include "MatView.h"


// CThicketDlg dialog
class CThicketDlg : public CDialogEx
{
	TreeDemo m_demo;
	std::recursive_mutex m_mutex;

// Construction
public:
	CThicketDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THICKETAPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	CPoint m_minDialogSize;

	CListCtrl m_transformsList;
	std::vector<int> m_sortOrder;
	int m_sortColumn = 1;

	CMatView m_matView;

	afx_msg LRESULT OnRunProgress(WPARAM, LPARAM);

	void OnMatViewLButtonDown(UINT nFlags, CPoint point);
	void OnMatViewRButtonDown(UINT nFlags, CPoint point);

	// Generated message map functions
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnFileOpenPrevious();
	afx_msg void OnFileSave();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg BOOL OnTtnNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL PreTranslateMessage(MSG *pMsg);
	afx_msg void OnBnClickedRandomize();
	afx_msg void OnBnClickedStep();
	afx_msg void OnBnClickedStart();
    afx_msg void OnLvnGetdispinfoTransforms(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnItemchangedTransforms(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMCustomdrawTransforms(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMClickTransforms(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnclickTransforms(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnClickedImage();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
