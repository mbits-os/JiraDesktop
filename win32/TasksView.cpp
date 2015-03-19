// TasksView.cpp : implementation of the CTasksView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "TasksView.h"

BOOL CTasksView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

LRESULT CTasksView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);

	//TODO: Add your drawing code here

	return 0;
}
