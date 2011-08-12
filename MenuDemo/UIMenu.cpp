#include "StdAfx.h"
#include "UICrack.h"
#include "UIMenu.h"

namespace DuiLib {

/////////////////////////////////////////////////////////////////////////////////////
//
ContextMenuObserver s_context_menu_observer;

// MenuUI
const TCHAR* const kMenuUIClassName = _T("MenuUI");
const TCHAR* const kMenuUIInterfaceName = _T("Menu");

CMenuUI::CMenuUI()
{
	if (GetHeader() != NULL)
		GetHeader()->SetVisible(false);
}

CMenuUI::~CMenuUI()
{}

LPCTSTR CMenuUI::GetClass() const
{
    return kMenuUIClassName;
}

LPVOID CMenuUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kMenuUIInterfaceName) == 0 ) return static_cast<CMenuUI*>(this);
    return CListUI::GetInterface(pstrName);
}

void CMenuUI::DoEvent(TEventUI& event)
{
	return __super::DoEvent(event);
}

bool CMenuUI::Add(CControlUI* pControl)
{
	CMenuElementUI* pMenuItem = static_cast<CMenuElementUI*>(pControl->GetInterface(kMenuElementUIInterfaceName));
	if (pMenuItem == NULL)
		return false;

	for (int i = 0; i < pMenuItem->GetCount(); ++i)
	{
		if (pMenuItem->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) != NULL)
		{
			(static_cast<CMenuElementUI*>(pMenuItem->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetInternVisible(false);
		}
	}
	return CListUI::Add(pControl);
}

bool CMenuUI::AddAt(CControlUI* pControl, int iIndex)
{
	CMenuElementUI* pMenuItem = static_cast<CMenuElementUI*>(pControl->GetInterface(kMenuElementUIInterfaceName));
	if (pMenuItem == NULL)
		return false;

	for (int i = 0; i < pMenuItem->GetCount(); ++i)
	{
		if (pMenuItem->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) != NULL)
		{
			(static_cast<CMenuElementUI*>(pMenuItem->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetInternVisible(false);
		}
	}
	return CListUI::AddAt(pControl, iIndex);
}

int CMenuUI::GetItemIndex(CControlUI* pControl) const
{
	CMenuElementUI* pMenuItem = static_cast<CMenuElementUI*>(pControl->GetInterface(kMenuElementUIInterfaceName));
	if (pMenuItem == NULL)
		return -1;

	return __super::GetItemIndex(pControl);
}

bool CMenuUI::SetItemIndex(CControlUI* pControl, int iIndex)
{
	CMenuElementUI* pMenuItem = static_cast<CMenuElementUI*>(pControl->GetInterface(kMenuElementUIInterfaceName));
	if (pMenuItem == NULL)
		return false;

	return __super::SetItemIndex(pControl, iIndex);
}

bool CMenuUI::Remove(CControlUI* pControl)
{
	CMenuElementUI* pMenuItem = static_cast<CMenuElementUI*>(pControl->GetInterface(kMenuElementUIInterfaceName));
	if (pMenuItem == NULL)
		return false;

	return __super::Remove(pControl);
}

SIZE CMenuUI::EstimateSize(SIZE szAvailable)
{
	int cxFixed = 0;
    int cyFixed = 0;
    for( int it = 0; it < GetCount(); it++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(GetItemAt(it));
        if( !pControl->IsVisible() ) continue;
        SIZE sz = pControl->EstimateSize(szAvailable);
        cyFixed += sz.cy;
		if( cxFixed < sz.cx )
			cxFixed = sz.cx;
    }
    return CSize(cxFixed, cyFixed);
}

void CMenuUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
	CListUI::SetAttribute(pstrName, pstrValue);
}

/////////////////////////////////////////////////////////////////////////////////////
//
class CMenuBuilderCallback: public IDialogBuilderCallback
{
	CControlUI* CreateControl(LPCTSTR pstrClass)
	{
		if (_tcsicmp(pstrClass, kMenuUIInterfaceName) == 0)
		{
			return new CMenuUI();
		}
		else if (_tcsicmp(pstrClass, kMenuElementUIInterfaceName) == 0)
		{
			return new CMenuElementUI();
		}
		return NULL;
	}
};

static bool bReachBottom = false;
static bool bReachRight = false;
static LONG chRightAlgin = 0;
static LONG chBottomAlgin = 0;

void ResetMenuCache()
{
	bReachBottom = false;
	bReachRight = false;
	chRightAlgin = 0;
	chBottomAlgin = 0;
}

CMenuWnd::CMenuWnd(HWND hParent):
m_hParent(hParent),
m_pOwner(NULL),
m_pLayout(),
m_xml(_T(""))
{}

BOOL CMenuWnd::Receive(ContextMenuParam param)
{
	switch (param.wParam)
	{
	case 1:
		Close();
		break;
	case 2:
		{
			HWND hParent = GetParent(m_hWnd);
			while (hParent != NULL)
			{
				if (hParent == param.hWnd)
				{
					Close();
					break;
				}
				hParent = GetParent(hParent);
			}
		}
		break;
	default:
		break;
	}

	return TRUE;
}

void CMenuWnd::Init(CMenuElementUI* pOwner, STRINGorID xml, LPCTSTR pSkinType, POINT point)
{
	m_BasedPoint = point;
    m_pOwner = pOwner;
    m_pLayout = NULL;

	if (pSkinType != NULL)
		m_sType = pSkinType;

	m_xml = xml;

	Create((m_pOwner == NULL) ? m_hParent : m_pOwner->GetManager()->GetPaintWindow(), NULL, WS_POPUP, WS_EX_TOOLWINDOW | WS_EX_TOPMOST, CRect());
    // HACK: Don't deselect the parent's caption
    HWND hWndParent = m_hWnd;
    while( ::GetParent(hWndParent) != NULL ) hWndParent = ::GetParent(hWndParent);
    ::ShowWindow(m_hWnd, SW_SHOW);
#if defined(WIN32) && !defined(UNDER_CE)
    ::SendMessage(hWndParent, WM_NCACTIVATE, TRUE, 0L);
#endif

	s_context_menu_observer.AddReceiver(this);

	m_pm.ReleaseCapture();
	SetCapture(m_hWnd);
}

LPCTSTR CMenuWnd::GetWindowClassName() const
{
    return _T("MenuWnd");
}

void CMenuWnd::OnFinalMessage(HWND hWnd)
{
	RemoveObserver();
	if( m_pOwner != NULL ) {
		for( int i = 0; i < m_pOwner->GetCount(); i++ ) {
			if( static_cast<CMenuElementUI*>(m_pOwner->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)) != NULL ) {
				(static_cast<CMenuElementUI*>(m_pOwner->GetItemAt(i)))->SetOwner(m_pOwner->GetParent());
				(static_cast<CMenuElementUI*>(m_pOwner->GetItemAt(i)))->SetVisible(false);
				(static_cast<CMenuElementUI*>(m_pOwner->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetInternVisible(false);
			}
		}
		m_pOwner->m_pWindow = NULL;
		m_pOwner->m_uButtonState &= ~ UISTATE_PUSHED;
		m_pOwner->Invalidate();
	}
    delete this;
}

BOOL CMenuWnd::IsMenuItemExpandable(POINT point, HWND hWnd, BOOL& bSameMenuBranch)
{
	bSameMenuBranch = FALSE;
	CControlUI* pControl = m_pm.GetRoot();
	if( pControl == NULL ) return FALSE;

	if( pControl->GetInterface(kMenuUIInterfaceName) == NULL ) return FALSE;

	CMenuUI* pMenuList = static_cast<CMenuUI*>(pControl->GetInterface(kMenuUIInterfaceName));

	CMenuElementUI* pMenuItem = NULL;
	if( pMenuList != NULL )
	{
		for( int i = 0; i < pMenuList->GetCount(); ++i )
		{
			if( PtInRect(&pMenuList->GetItemAt(i)->GetPos(), point) )
			{
				pMenuItem = static_cast<CMenuElementUI*>(pMenuList->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName));
				break;
			}
		}
	}

	if( pMenuItem == NULL ) return FALSE;
	for( int i = 0; i < pMenuItem->GetCount(); ++i ) {
		if(pMenuItem->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) != NULL) 
		{
			if (pMenuItem->GetMenuWnd() != NULL)
				bSameMenuBranch = (hWnd == pMenuItem->GetMenuWnd()->GetHWND());
			return (pMenuItem->GetMenuWnd() != NULL) ? FALSE : TRUE;
		}
	}

	return FALSE;
}

LRESULT CMenuWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if( uMsg == WM_CREATE ) {
		if( m_pOwner != NULL) {
			LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
			styleValue &= ~WS_CAPTION;
			::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
			RECT rcClient;
			::GetClientRect(*this, &rcClient);
			::SetWindowPos(*this, NULL, rcClient.left, rcClient.top, rcClient.right - rcClient.left, \
				rcClient.bottom - rcClient.top, SWP_FRAMECHANGED);

			m_pm.Init(m_hWnd);
			// The trick is to add the items to the new container. Their owner gets
			// reassigned by this operation - which is why it is important to reassign
			// the items back to the righfull owner/manager when the window closes.
			m_pLayout = new CMenuUI();
			m_pm.UseParentResource(m_pOwner->GetManager());
			m_pLayout->SetManager(&m_pm, NULL, true);
			LPCTSTR pDefaultAttributes = m_pOwner->GetManager()->GetDefaultAttributeList(kMenuUIInterfaceName);
			if( pDefaultAttributes ) {
				m_pLayout->ApplyAttributeList(pDefaultAttributes);
			}
			m_pLayout->SetBkColor(0xFFFFFFFF);
			m_pLayout->SetBorderColor(0xFF85E4FF);
			m_pLayout->SetBorderSize(0);
			m_pLayout->SetAutoDestroy(false);
			m_pLayout->EnableScrollBar();
			for( int i = 0; i < m_pOwner->GetCount(); i++ ) {
				if(m_pOwner->GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) != NULL ){
					(static_cast<CMenuElementUI*>(m_pOwner->GetItemAt(i)))->SetOwner(m_pLayout);
					m_pLayout->Add(static_cast<CControlUI*>(m_pOwner->GetItemAt(i)));
				}
			}
			m_pm.AttachDialog(m_pLayout);

			// Position the popup window in absolute space
			RECT rcOwner = m_pOwner->GetPos();
			RECT rc = rcOwner;

			int cxFixed = 0;
			int cyFixed = 0;

#if defined(WIN32) && !defined(UNDER_CE)
			MONITORINFO oMonitor = {}; 
			oMonitor.cbSize = sizeof(oMonitor);
			::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
			CRect rcWork = oMonitor.rcWork;
#else
			CRect rcWork;
			GetWindowRect(m_pOwner->GetManager()->GetPaintWindow(), &rcWork);
#endif
			SIZE szAvailable = { rcWork.right - rcWork.left, rcWork.bottom - rcWork.top };

			for( int it = 0; it < m_pOwner->GetCount(); it++ ) {
				if(m_pOwner->GetItemAt(it)->GetInterface(kMenuElementUIInterfaceName) != NULL ){
					CControlUI* pControl = static_cast<CControlUI*>(m_pOwner->GetItemAt(it));
					SIZE sz = pControl->EstimateSize(szAvailable);
					cyFixed += sz.cy;

					if( cxFixed < sz.cx )
						cxFixed = sz.cx;
				}
			}
			cyFixed += 4;
			cxFixed += 4;

			CRect rcWindow;
			GetWindowRect(m_pOwner->GetManager()->GetPaintWindow(), &rcWindow);

			rc.top = rcOwner.top;
			rc.bottom = rc.top + cyFixed;
			::MapWindowRect(m_pOwner->GetManager()->GetPaintWindow(), HWND_DESKTOP, &rc);
			rc.left = rcWindow.right;
			rc.right = rc.left + cxFixed;
			rc.right += 2;

			if (bReachBottom)
			{
				rc.bottom = chBottomAlgin;
				rc.top = rc.bottom - cyFixed;

				chBottomAlgin = rc.top;
			}
			else if( rc.bottom > rcWork.bottom )
			{
				bReachBottom = true;
				rc.bottom = rc.top;
				rc.top = rc.bottom - cyFixed;

				chBottomAlgin = rc.top;
			}

			if (bReachRight)
			{
				rc.right = chRightAlgin;
				rc.left = rc.right - cxFixed;

				chRightAlgin = rc.left;
			}
			else if (rc.right > rcWork.right)
			{
				bReachRight = true;

				rc.right = rcWindow.left;
				rc.left = rc.right - cxFixed;

				rc.top = rcWindow.bottom;
				rc.bottom = rc.top + cyFixed;

				chRightAlgin = rc.left;
			}


			MoveWindow(m_hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
		}
		else {
			ResetMenuCache();

			m_pm.Init(m_hWnd);

			CDialogBuilder builder;
			CMenuBuilderCallback menuCallback;

			CControlUI* pRoot = builder.Create(m_xml, m_sType.GetData(), &menuCallback, &m_pm);
			m_pm.AttachDialog(pRoot);

#if defined(WIN32) && !defined(UNDER_CE)
			MONITORINFO oMonitor = {}; 
			oMonitor.cbSize = sizeof(oMonitor);
			::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
			CRect rcWork = oMonitor.rcWork;
#else
			CRect rcWork;
			GetWindowRect(m_pOwner->GetManager()->GetPaintWindow(), &rcWork);
#endif
			SIZE szAvailable = { rcWork.right - rcWork.left, rcWork.bottom - rcWork.top };
			szAvailable = pRoot->EstimateSize(szAvailable);
			m_pm.SetInitSize(szAvailable.cx, szAvailable.cy);

			DWORD dwAlignment = eMenuAlignment_Left | eMenuAlignment_Top;

			SIZE szInit = m_pm.GetInitSize();
			CRect rc;
			CPoint point = m_BasedPoint;
			rc.left = point.x;
			rc.top = point.y;
			rc.right = rc.left + szInit.cx;
			rc.bottom = rc.top + szInit.cy;

			int nWidth = rc.GetWidth();
			int nHeight = rc.GetHeight();

			if (dwAlignment & eMenuAlignment_Right)
			{
				rc.right = point.x;
				rc.left = rc.right - nWidth;
			}

			if (dwAlignment & eMenuAlignment_Bottom)
			{
				rc.bottom = point.y;
				rc.top = rc.bottom - nHeight;
			}

			SetForegroundWindow(m_hWnd);
			MoveWindow(m_hWnd, rc.left, rc.top, rc.GetWidth(), rc.GetHeight(), FALSE);
			SetWindowPos(m_hWnd, HWND_TOPMOST, rc.left, rc.top, rc.GetWidth(), rc.GetHeight(), SWP_SHOWWINDOW);
		}

		return 0;
    }
    else if( uMsg == WM_CLOSE ) {
		if( m_pOwner != NULL )
		{
			m_pOwner->SetManager(m_pOwner->GetManager(), m_pOwner->GetParent(), false);
			m_pOwner->SetPos(m_pOwner->GetPos());
			m_pOwner->SetFocus();
		}
    }
	else if( uMsg == WM_MOUSEHOVER )
	{
		CPoint point(lParam);
		ClientToScreen(m_hWnd, &point);

		CRect rcPos;
		GetWindowRect(m_hWnd, &rcPos);
		if( !PtInRect(&rcPos, point) ) {
			ContextMenuParam param;
			param.hWnd = GetHWND();

			ContextMenuObserver::Iterator<BOOL, ContextMenuParam> iterator(s_context_menu_observer);
			ReceiverImplBase<BOOL, ContextMenuParam>* pReceiver = iterator.next();

			HWND hNextWnd = m_hWnd;
			while (pReceiver != NULL) {
				CMenuWnd* pContextMenu = dynamic_cast<CMenuWnd*>(pReceiver);
				if (pContextMenu != NULL) {
					CRect rcWindow;
					GetWindowRect(pContextMenu->GetHWND(), &rcWindow);

					CPoint point2(lParam);
					ClientToScreen(m_hWnd, &point2);
					ScreenToClient(pContextMenu->GetHWND(), &point2);
					if( PtInRect(&rcWindow, point) ) {
						BOOL bSameMenuBranch = FALSE;
						pReceiver = iterator.next();
						if( (pReceiver != NULL) && dynamic_cast<CMenuWnd*>(pReceiver) )
							hNextWnd = dynamic_cast<CMenuWnd*>(pReceiver)->GetHWND();
						
						BOOL bExpandable = pContextMenu->IsMenuItemExpandable(point2, hNextWnd, bSameMenuBranch);

						if( bSameMenuBranch )
							break;
						ReleaseCapture();
						ResetMenuCache();
						param.hWnd = pContextMenu->GetHWND();
						param.wParam = 2;
						s_context_menu_observer.RBroadcast(param);
						SetCapture(param.hWnd);

						if( bExpandable )
							::PostMessage(param.hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point2.x, point2.y));

						return 0;
					}					
				}

				pReceiver = iterator.next();
			}			
		}
	}
	else if( uMsg == WM_LBUTTONUP )
	{
		CPoint point(lParam);
		ClientToScreen(m_hWnd, &point);

		CRect rcPos;
		GetWindowRect(m_hWnd, &rcPos);
		if( !PtInRect(&rcPos, point) ) {
			BOOL bExpandable = FALSE;
			BOOL bSameMenuBranch = FALSE;
			bool bPointInContextMenu = false;
			ContextMenuParam param;
			param.hWnd = GetHWND();

			ContextMenuObserver::Iterator<BOOL, ContextMenuParam> iterator(s_context_menu_observer);
			ReceiverImplBase<BOOL, ContextMenuParam>* pReceiver = iterator.next();

			HWND hNextWnd = m_hWnd;
			while( pReceiver != NULL ) {
				CMenuWnd* pContextMenu = dynamic_cast<CMenuWnd*>(pReceiver);
				if( pContextMenu != NULL ) {
					CRect rcWindow;
					GetWindowRect(pContextMenu->GetHWND(), &rcWindow);

					if( PtInRect(&rcWindow, point) )
					{
						pReceiver = iterator.next();
						if( (pReceiver != NULL) && dynamic_cast<CMenuWnd*>(pReceiver) )
							hNextWnd = dynamic_cast<CMenuWnd*>(pReceiver)->GetHWND();

						CPoint point2(lParam);
						ClientToScreen(m_hWnd, &point2);
						ScreenToClient(pContextMenu->GetHWND(), &point2);
						bExpandable = pContextMenu->IsMenuItemExpandable(point2, hNextWnd, bSameMenuBranch);

						param.hWnd = pContextMenu->GetHWND();
						bPointInContextMenu = true;
						break;
					}
				}

				pReceiver = iterator.next();
			}

			if( bSameMenuBranch ) return 0L;
		}
	}
	else if( uMsg == WM_RBUTTONDOWN || uMsg == WM_CONTEXTMENU || uMsg == WM_RBUTTONUP || uMsg == WM_RBUTTONDBLCLK )
	{
		return 0L;
	}
	else if( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK )
	{
		CPoint point(lParam);
		ClientToScreen(m_hWnd, &point);

		CRect rcPos;
		GetWindowRect(m_hWnd, &rcPos);
		if( !PtInRect(&rcPos, point) ) {
			BOOL bExpandable = FALSE;
			BOOL bSameMenuBranch = FALSE;
			bool bPointInContextMenu = false;
			ContextMenuParam param;
			param.hWnd = GetHWND();

			ContextMenuObserver::Iterator<BOOL, ContextMenuParam> iterator(s_context_menu_observer);
			ReceiverImplBase<BOOL, ContextMenuParam>* pReceiver = iterator.next();

			HWND hNextWnd = m_hWnd;
			while( pReceiver != NULL ) {
				CMenuWnd* pContextMenu = dynamic_cast<CMenuWnd*>(pReceiver);
				if( pContextMenu != NULL ) {
					CRect rcWindow;
					GetWindowRect(pContextMenu->GetHWND(), &rcWindow);

					if( PtInRect(&rcWindow, point) )
					{
						pReceiver = iterator.next();
						if( (pReceiver != NULL) && dynamic_cast<CMenuWnd*>(pReceiver) )
							hNextWnd = dynamic_cast<CMenuWnd*>(pReceiver)->GetHWND();

						CPoint point2(lParam);
						ClientToScreen(m_hWnd, &point2);
						ScreenToClient(pContextMenu->GetHWND(), &point2);
						bExpandable = pContextMenu->IsMenuItemExpandable(point2, hNextWnd, bSameMenuBranch);

						param.hWnd = pContextMenu->GetHWND();
						bPointInContextMenu = true;
						break;
					}
				}

				pReceiver = iterator.next();
			}

			if( !bSameMenuBranch ) {
				ReleaseCapture();

				param.wParam = bPointInContextMenu ? 2 : 1;
				s_context_menu_observer.RBroadcast(param);

				if( bPointInContextMenu ) {
					ResetMenuCache();

					CPoint point(lParam);
					ClientToScreen(m_hWnd, &point);
					ScreenToClient(param.hWnd, &point);

					::SendMessage(param.hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
					::SendMessage(param.hWnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(point.x, point.y));
					if( bExpandable ) SetCapture(param.hWnd);
				}

				return 0;
			}

			if( uMsg == WM_LBUTTONDBLCLK )
				return 0L;
		}
	}
	else if( uMsg == WM_KEYDOWN)
	{
		if( wParam == VK_ESCAPE)
		{
			Close();
			SetCapture(GetParent(m_hWnd));
		}
	}

    LRESULT lRes = 0;
    if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) return lRes;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////
//

// MenuElementUI
const TCHAR* const kMenuElementUIClassName = _T("MenuElementUI");
const TCHAR* const kMenuElementUIInterfaceName = _T("MenuElement");

CMenuElementUI::CMenuElementUI():
m_pWindow(NULL)
{
	m_cxyFixed.cy = 25;
	m_bMouseChildEnabled = true;
}

CMenuElementUI::~CMenuElementUI()
{}

LPCTSTR CMenuElementUI::GetClass() const
{
	return kMenuElementUIClassName;
}

LPVOID CMenuElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kMenuElementUIInterfaceName) == 0 ) return static_cast<CMenuElementUI*>(this);    
    return CListContainerElementUI::GetInterface(pstrName);
}

void CMenuElementUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if( !::IntersectRect(&m_rcPaint, &rcPaint, &m_rcItem) ) return;
	CMenuElementUI::DrawItemBk(hDC, m_rcItem);
	DrawItemText(hDC, m_rcItem);
	for (int i = 0; i < GetCount(); ++i)
	{
		if (GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) == NULL)
		{
			GetItemAt(i)->DoPaint(hDC, rcPaint);
		}
	}
}

void CMenuElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    if( m_sText.IsEmpty() ) return;

    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iTextColor = pInfo->dwTextColor;
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if( IsSelected() ) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if( !IsEnabled() ) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    int nLinks = 0;
    RECT rcText = rcItem;
    rcText.left += pInfo->rcTextPadding.left;
    rcText.right -= pInfo->rcTextPadding.right;
    rcText.top += pInfo->rcTextPadding.top;
    rcText.bottom -= pInfo->rcTextPadding.bottom;

    if( pInfo->bShowHtml )
        CRenderEngine::DrawHtmlText(hDC, m_pManager, rcText, m_sText, iTextColor, \
        NULL, NULL, nLinks, DT_SINGLELINE | pInfo->uTextStyle);
    else
        CRenderEngine::DrawText(hDC, m_pManager, rcText, m_sText, iTextColor, \
        pInfo->nFont, DT_SINGLELINE | pInfo->uTextStyle);
}


SIZE CMenuElementUI::EstimateSize(SIZE szAvailable)
{
	SIZE cXY = {0};
	for( int it = 0; it < GetCount(); it++ ) {
		CControlUI* pControl = static_cast<CControlUI*>(GetItemAt(it));
		if( !pControl->IsVisible() ) continue;
		SIZE sz = pControl->EstimateSize(szAvailable);
		cXY.cy += sz.cy;
		if( cXY.cx < sz.cx )
			cXY.cx = sz.cx;
	}
	if(cXY.cy == 0) {
		TListInfoUI* pInfo = m_pOwner->GetListInfo();

		DWORD iTextColor = pInfo->dwTextColor;
		if( (m_uButtonState & UISTATE_HOT) != 0 ) {
			iTextColor = pInfo->dwHotTextColor;
		}
		if( IsSelected() ) {
			iTextColor = pInfo->dwSelectedTextColor;
		}
		if( !IsEnabled() ) {
			iTextColor = pInfo->dwDisabledTextColor;
		}

		RECT rcText = { 0, 0, MAX(szAvailable.cx, m_cxyFixed.cx), 9999 };
		rcText.left += pInfo->rcTextPadding.left;
		rcText.right -= pInfo->rcTextPadding.right;
		if( pInfo->bShowHtml ) {   
			int nLinks = 0;
			CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, iTextColor, NULL, NULL, nLinks, DT_CALCRECT | pInfo->uTextStyle);
		}
		else {
			CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, iTextColor, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle);
		}
		cXY.cx = rcText.right - rcText.left + pInfo->rcTextPadding.left + pInfo->rcTextPadding.right + 20;
		cXY.cy = rcText.bottom - rcText.top + pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
	}

	if( m_cxyFixed.cy != 0 ) cXY.cy = m_cxyFixed.cy;
	return cXY;
}

void CMenuElementUI::DoEvent(TEventUI& event)
{
	if( event.Type == UIEVENT_MOUSEHOVER )
	{
		if( m_pWindow ) return;
		bool hasSubMenu = false;
		for( int i = 0; i < GetCount(); ++i )
		{
			if( GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) != NULL )
			{
				(static_cast<CMenuElementUI*>(GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetVisible(true);
				(static_cast<CMenuElementUI*>(GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetInternVisible(true);

				hasSubMenu = true;
			}
		}
		if( hasSubMenu )
		{
			m_pWindow = new CMenuWnd(m_pManager->GetPaintWindow());
			ASSERT(m_pWindow);

			ContextMenuParam param;
			param.hWnd = m_pManager->GetPaintWindow();
			param.wParam = 2;
			s_context_menu_observer.RBroadcast(param);

			m_pWindow->Init(static_cast<CMenuElementUI*>(this), _T(""), _T(""), CPoint());
		}
		return;
	}

	if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN )
	{
		if( IsEnabled() ){
			CListContainerElementUI::DoEvent(event);

			if( m_pWindow ) return;

			bool hasSubMenu = false;
			for( int i = 0; i < GetCount(); ++i ) {
				if( GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) != NULL ) {
					(static_cast<CMenuElementUI*>(GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetVisible(true);
					(static_cast<CMenuElementUI*>(GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetInternVisible(true);

					hasSubMenu = true;
				}
			}
			if( hasSubMenu )
			{
				m_pWindow = new CMenuWnd(m_pManager->GetPaintWindow());
				ASSERT(m_pWindow);

				ContextMenuParam param;
				param.hWnd = m_pManager->GetPaintWindow();
				param.wParam = 2;
				s_context_menu_observer.RBroadcast(param);

				m_pWindow->Init(static_cast<CMenuElementUI*>(this), _T(""), _T(""), CPoint());
			}
			else
			{
				ContextMenuParam param;
				param.hWnd = m_pManager->GetPaintWindow();
				param.wParam = 1;
				s_context_menu_observer.RBroadcast(param);
			}
        }
        return;
    }

    CListContainerElementUI::DoEvent(event);
}

bool CMenuElementUI::Activate()
{
	if (CListContainerElementUI::Activate() && m_bSelected)
	{
		if( m_pWindow ) return true;
		bool hasSubMenu = false;
		for (int i = 0; i < GetCount(); ++i)
		{
			if (GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName) != NULL)
			{
				(static_cast<CMenuElementUI*>(GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetVisible(true);
				(static_cast<CMenuElementUI*>(GetItemAt(i)->GetInterface(kMenuElementUIInterfaceName)))->SetInternVisible(true);

				hasSubMenu = true;
			}
		}
		if (hasSubMenu)
		{
			m_pWindow = new CMenuWnd(m_pManager->GetPaintWindow());
			ASSERT(m_pWindow);

			ContextMenuParam param;
			param.hWnd = m_pManager->GetPaintWindow();
			param.wParam = 2;
			s_context_menu_observer.RBroadcast(param);

			m_pWindow->Init(static_cast<CMenuElementUI*>(this), _T(""), _T(""), CPoint());
		}
		else
		{
			ContextMenuParam param;
			param.hWnd = m_pManager->GetPaintWindow();
			param.wParam = 1;
			s_context_menu_observer.RBroadcast(param);
		}

		return true;
	}
	return false;
}

CMenuWnd* CMenuElementUI::GetMenuWnd()
{
	return m_pWindow;
}

} // namespace DuiLib