#include "stdafx.h"
#include <memory>
#include <afxole.h>
#include "HexEditCtrl.h"
//#include "resource.h"
#include "stdafx.h"


/////////////////////////////////////////////////////////////////////////////
// defines
/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// control-layout customization (low-level)
#define ADR_DATA_SPACE				28
#define DATA_ASCII_SPACE			50
#define CONTROL_BORDER_SPACE		5

//ascii字符之间的间隔
#define ASCII_SPACE 0x1

//hex字符之间的间隔.
#define HEX_SPACE 0x4

// boundaries and special values
#define MAX_HIGHLIGHT_POLYPOINTS	8
#define UM_SETSCROLRANGE			(WM_USER + 0x5000)
#define MOUSEREP_TIMER_TID			0x400
#define MOUSEREP_TIMER_ELAPSE		0x5

// windows-class-name
#define HEXEDITBASECTRL_CLASSNAME	_T("CHexEditBase")
#define HEXEDITBASECTRL_CLSNAME_SC	_T("CHexEditBase_SC") //self creating

// macros
#define NORMALIZE_SELECTION(beg, end) if(beg>end){UINT tmp = end; end=beg; beg=tmp; }



/////////////////////////////////////////////////////////////////////////////
// global data
/////////////////////////////////////////////////////////////////////////////
const char tabHexCharacters[16] = {
	'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' }; 




	
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// class CHexEditBase
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
	
IMPLEMENT_DYNCREATE(CHexEditCtrl, CWnd)

BEGIN_MESSAGE_MAP(CHexEditCtrl, CWnd)
	//{{AFX_MSG_MAP(CHexEditBase)
	ON_MESSAGE(UM_SETSCROLRANGE, OnUmSetScrollRange)
	ON_MESSAGE(WM_CHAR, OnWMChar)
	ON_MESSAGE(WM_SETFONT, OnWMSetFont)
	ON_MESSAGE(WM_GETFONT, OnWMGetFont)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_KILLFOCUS()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_GETDLGCODE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CHexEditCtrl::CHexEditCtrl() :
	m_bSelfCleanup(false),
	m_pData(NULL), 
	m_BuffSize(0),
	m_nLength(0), 
	m_nCurCaretWidth(0),
	m_nCurCaretHeight(0), 
	m_bHasCaret(false), 
	m_bHighBits(true),
	m_nSelectingBeg(NOSECTION_VAL),
	m_nSelectingEnd(NOSECTION_VAL),
	m_nSelectionBegin(NOSECTION_VAL),
	m_nSelectionEnd(NOSECTION_VAL), 
	m_nCurrentAddress(0),
	m_bRecalc(true),
	m_nScrollPostionX(0), 
	m_nScrollRangeX(0), 
	m_nScrollPostionY(0), 
	m_nScrollRangeY(0),
	m_bShowCategory(false),
	m_nMouseRepSpeed(0),
	m_iMouseRepDelta(0),
	m_nMouseRepCounter(0),
	m_bIsMouseRepActive(false),
	m_cDragRect(0,0,0,0)

{
	memset(&m_tPaintDetails, 0, sizeof(PAINTINGDETAILS));

	m_tSelectedTxtCol = GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_tSelectedBkgCol = GetSysColor(COLOR_HIGHLIGHT);

	if (!m_cFont.CreateFont(
		20,
		0,
		0,
		0,
		FW_REGULAR,
		FALSE,
		FALSE,
		0,
		DEFAULT_CHARSET,
		DEFAULT_CHARSET,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT,
		TEXT("Consolas"))) {
		AfxThrowResourceException();
	}

	// register windows-class
	RegisterClass();
}

CHexEditCtrl::~CHexEditCtrl()
{
	if (m_pData) {
		delete[]m_pData;
		m_pData = NULL;
	}

    if(m_cFont.m_hObject != NULL) {
		m_cFont.DeleteObject();
	}

	if(::IsWindow(m_hWnd)) {
		DestroyWindow();
	}
}

BOOL CHexEditCtrl::Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	return CWnd::Create(HEXEDITBASECTRL_CLASSNAME, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL CHexEditCtrl::CreateEx(DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu, LPVOID lpParam)
{
	return CWnd::CreateEx(dwExStyle, HEXEDITBASECTRL_CLASSNAME, lpszWindowName, dwStyle, x, y, nWidth, nHeight, hwndParent, nIDorHMenu, lpParam);
}

BOOL CHexEditCtrl::CreateEx(DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam)
{
	return CWnd::CreateEx(dwExStyle, HEXEDITBASECTRL_CLASSNAME, lpszWindowName, dwStyle, rect, pParentWnd, nID, lpParam);
}

void CHexEditCtrl::RegisterClass()
{
	// register windowsclass
	WNDCLASS tWndClass;
	if(!::GetClassInfo(AfxGetInstanceHandle(), HEXEDITBASECTRL_CLASSNAME, &tWndClass))
	{
		memset(&tWndClass, 0, sizeof(WNDCLASS));
        tWndClass.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
		tWndClass.lpfnWndProc = ::DefWindowProc;
		tWndClass.hInstance = AfxGetInstanceHandle();
		tWndClass.hCursor = ::LoadCursor(NULL, IDC_IBEAM);
		tWndClass.lpszClassName = HEXEDITBASECTRL_CLASSNAME;

		if(!AfxRegisterClass(&tWndClass)) {
			AfxThrowResourceException();
		}
	}
	if(!::GetClassInfo(AfxGetInstanceHandle(), HEXEDITBASECTRL_CLSNAME_SC, &tWndClass))
	{
		memset(&tWndClass, 0, sizeof(WNDCLASS));
        tWndClass.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
		tWndClass.lpfnWndProc = CHexEditCtrl::WndProc;
		tWndClass.hInstance = AfxGetInstanceHandle();
		tWndClass.hCursor = ::LoadCursor(NULL, IDC_IBEAM);
		tWndClass.lpszClassName = HEXEDITBASECTRL_CLSNAME_SC;

		if(!AfxRegisterClass(&tWndClass)) {
			AfxThrowResourceException();
		}
	}	
}

LRESULT CALLBACK CHexEditCtrl::WndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if(nMsg == WM_NCCREATE) {		
		ASSERT(FromHandlePermanent(hWnd) == NULL );
		CHexEditCtrl* pControl = NULL;
		try {
			pControl = new CHexEditCtrl();
			pControl->m_bSelfCleanup = true;
		} catch(...) { 
			return FALSE;
		}
		if(pControl == NULL) {
			return FALSE;
		}
		if(!pControl->SubclassWindow(hWnd)) { 
			TRACE("CHexEditBase::WndProc: ERROR: couldn't subclass window (WM_NCCREATE)\n");
			delete pControl;
			return FALSE;
		}
		return TRUE;
	}
	return ::DefWindowProc(hWnd, nMsg, wParam, lParam);
}

void CHexEditCtrl::NotifyParent(WORD wNBotifictionCode)
{
	CWnd *pWnd = GetParent();
	if(pWnd != NULL) {
		pWnd->SendMessage(WM_COMMAND, MAKEWPARAM((WORD)GetDlgCtrlID(), wNBotifictionCode), (LPARAM)m_hWnd);
	}
}

void CHexEditCtrl::PostNcDestroy()
{
	if(m_bSelfCleanup) {
		m_bSelfCleanup = false;
		delete this;
	}
}

void CHexEditCtrl::OnDestroy()
{
	CWnd::OnDestroy();
}

//计算绘制区域信息。
void CHexEditCtrl::CalculatePaintingDetails(CDC& cDC)
{
	ASSERT(m_nScrollPostionY >= 0);

	CFont *pOldFont;
	m_bRecalc = false;

	// Get size information

	//获取一个字符的宽度
	int iWidth;
	pOldFont = cDC.SelectObject(&m_cFont);
	cDC.GetCharWidth('0', '0', &iWidth);
	ASSERT(iWidth > 0);
	m_tPaintDetails.nCharacterWidth = iWidth;
	
	//获取一行的高度.
	CSize cSize = cDC.GetTextExtent(TEXT("0"), 1);
	ASSERT(cSize.cy > 0);
	m_tPaintDetails.nLineHeight = cSize.cy;

	// count of visible lines
	GetClientRect(m_tPaintDetails.cPaintingRect);

	//
	if(GetStyle() & ES_MULTILINE) {
		m_tPaintDetails.cPaintingRect.InflateRect(
			-CONTROL_BORDER_SPACE, -CONTROL_BORDER_SPACE, 
			-CONTROL_BORDER_SPACE, -CONTROL_BORDER_SPACE);
		
		if(m_tPaintDetails.cPaintingRect.right < m_tPaintDetails.cPaintingRect.left) {
			m_tPaintDetails.cPaintingRect.right = m_tPaintDetails.cPaintingRect.left;
		}
		if(m_tPaintDetails.cPaintingRect.bottom < m_tPaintDetails.cPaintingRect.top) {
			m_tPaintDetails.cPaintingRect.bottom = m_tPaintDetails.cPaintingRect.top;
		}
	}

	//计算显示的行数
	m_tPaintDetails.nVisibleLines = m_tPaintDetails.cPaintingRect.Height() / m_tPaintDetails.nLineHeight;
	m_tPaintDetails.nLastLineHeight = m_tPaintDetails.cPaintingRect.Height() % m_tPaintDetails.nLineHeight;
	
	if(m_tPaintDetails.nLastLineHeight > 0) {
		m_tPaintDetails.nFullVisibleLines = m_tPaintDetails.nVisibleLines;
		m_tPaintDetails.nVisibleLines++;
	} else {
		m_tPaintDetails.nFullVisibleLines = m_tPaintDetails.nVisibleLines;
		m_tPaintDetails.nLastLineHeight = m_tPaintDetails.nLineHeight;
	}
	
	// position & size of the address
	m_tPaintDetails.nAddressPos = 0;
	m_tPaintDetails.nAddressLen = ADR_DATA_SPACE + m_tPaintDetails.nCharacterWidth* ADDRESS_SIZE;

	//initialize bytes per row.
	m_tPaintDetails.nBytesPerRow = BYTES_PER_ROW;

	//如果没有设置多行显示的话,那么只能在一行内显示所有的字节数了.
	if(!(GetStyle() & ES_MULTILINE)) {
		m_tPaintDetails.nBytesPerRow = m_nLength;
	}

	if(m_tPaintDetails.nBytesPerRow == 0) {
		m_tPaintDetails.nBytesPerRow = 1;
	}

	// position & size of the hex-data
	m_tPaintDetails.nHexPos = m_tPaintDetails.nAddressPos + m_tPaintDetails.nAddressLen;
	m_tPaintDetails.nHexLen = (m_tPaintDetails.nBytesPerRow * 2 + (m_tPaintDetails.nBytesPerRow - 1) * HEX_SPACE ) * m_tPaintDetails.nCharacterWidth;
														//2(HEXData) + 1(Space) (only n-1 spaces needed)
	iWidth = m_tPaintDetails.nHexPos + m_tPaintDetails.nHexLen;
	m_tPaintDetails.nHexLen += DATA_ASCII_SPACE;
	
	// position & size of the ascii-data
	m_tPaintDetails.nAsciiPos = m_tPaintDetails.nHexPos + m_tPaintDetails.nHexLen;
	m_tPaintDetails.nAsciiLen = (m_tPaintDetails.nBytesPerRow * 1 + (m_tPaintDetails.nBytesPerRow - 1) * ASCII_SPACE) * m_tPaintDetails.nCharacterWidth;

	iWidth = m_tPaintDetails.nAsciiPos + m_tPaintDetails.nAsciiLen;

	if(pOldFont != NULL) {
		pOldFont = cDC.SelectObject(pOldFont);
	}

	// calculate scrollranges	
	// Y-Bar
	UINT nTotalLines;
	//向上取整8 对齐,加上一个空字节
	nTotalLines = ((m_nLength + 1) + m_tPaintDetails.nBytesPerRow - 1) / m_tPaintDetails.nBytesPerRow;		

	if(nTotalLines > m_tPaintDetails.nFullVisibleLines) {
		m_nScrollRangeY = nTotalLines - m_tPaintDetails.nFullVisibleLines;
	} else {
		m_nScrollRangeY = 0;
	}
	if(m_nScrollPostionY > m_nScrollRangeY) {
		m_nScrollPostionY = m_nScrollRangeY;
	}

	// X-Bar， iWidth is total x
	if(iWidth > m_tPaintDetails.cPaintingRect.Width()) {
		m_nScrollRangeX = iWidth - m_tPaintDetails.cPaintingRect.Width();
	} else {
		m_nScrollRangeX = 0;
	}

	if(m_nScrollPostionX > m_nScrollRangeX) {
		m_nScrollPostionX = m_nScrollRangeX;
	}

	PostMessage(UM_SETSCROLRANGE, 0 ,0);
}

//绘制地址.
void CHexEditCtrl::PaintAddresses(CDC& cDC)
{
	ASSERT(m_tPaintDetails.nBytesPerRow > 0);

	UINT nAdr;
	UINT nEndAdr;
	CString cAdrFormatString;
	CRect cAdrRect(m_tPaintDetails.cPaintingRect);
	_TCHAR pBuf[32];
	CBrush cBkgBrush;
	
	// create the format string
	cAdrFormatString.Format(_T("%%0%uX"), ADDRESS_SIZE);

	// the Rect for painting & background
	cBkgBrush.CreateStockObject(WHITE_BRUSH);
	cAdrRect.left += m_tPaintDetails.nAddressPos - m_nScrollPostionX;
	cAdrRect.right = cAdrRect.left + m_tPaintDetails.nAddressLen - ADR_DATA_SPACE; // without border
	cDC.FillRect(cAdrRect, &cBkgBrush);

	cAdrRect.bottom = cAdrRect.top + m_tPaintDetails.nLineHeight;

	// start & end-address
	nAdr = m_nScrollPostionY * m_tPaintDetails.nBytesPerRow;
	nEndAdr = nAdr + m_tPaintDetails.nVisibleLines * m_tPaintDetails.nBytesPerRow;

	if(nEndAdr >= m_nLength) {
		nEndAdr = (m_nLength + 1);
	}

	//  paint
	for(; nAdr < nEndAdr; nAdr+=m_tPaintDetails.nBytesPerRow) {
		_sntprintf(pBuf, 32, (LPCTSTR)cAdrFormatString, nAdr); // slightly faster then CString::Format
		cDC.DrawText(pBuf, (LPRECT)cAdrRect, DT_LEFT|DT_TOP|DT_SINGLELINE|DT_NOPREFIX);
		cAdrRect.OffsetRect(0, m_tPaintDetails.nLineHeight);
	}
}	


//绘制16进制数据.
void CHexEditCtrl::PaintHexData(CDC& cDC)
{
	ASSERT(m_tPaintDetails.nBytesPerRow > 0);
	ASSERT(m_tPaintDetails.nBytesPerRow * 3 < 1021);

	if ((m_nLength < 1) || (m_pData == NULL))
		return;

	UINT nAdr;
	UINT nEndAdr;
	UINT nSelectionCount = 0;
	TCHAR print_buf[1024];
	TCHAR *print_end;
	TCHAR *print_selected_begin;
	TCHAR *print_selected_end;
	BYTE *pDataPtr;
	BYTE *pEndDataPtr;
	BYTE *pEndLineDataPtr;
	BYTE *pSelectionPtrBegin;
	BYTE *pSelectionPtrEnd;
	CRect cHexRect(m_tPaintDetails.cPaintingRect);
	CBrush cBkgBrush;
	CBrush *pOldBrush = NULL;

	// the Rect for painting & background
	cBkgBrush.CreateStockObject(WHITE_BRUSH);
	cHexRect.left += m_tPaintDetails.nHexPos - m_nScrollPostionX;
	cHexRect.right = cHexRect.left + m_tPaintDetails.nHexLen - DATA_ASCII_SPACE;
	cDC.FillRect(cHexRect, &cBkgBrush);
	cHexRect.bottom = cHexRect.top + m_tPaintDetails.nLineHeight;

	// selection (pointers) ,,看看当前有没有选中区域.
	if ((m_nSelectionBegin != NOSECTION_VAL) && (m_nSelectionEnd != NOSECTION_VAL)) {
		pSelectionPtrBegin = m_pData + m_nSelectionBegin;
		pSelectionPtrEnd = m_pData + m_nSelectionEnd;
	}
	else {
		pSelectionPtrBegin = NULL;
		pSelectionPtrEnd = NULL;
	}

	// start & end-address (& pointers)
	nAdr = m_nScrollPostionY * m_tPaintDetails.nBytesPerRow;
	nEndAdr = nAdr + m_tPaintDetails.nVisibleLines * m_tPaintDetails.nBytesPerRow;
	if (nEndAdr >= m_nLength) {
		nEndAdr = m_nLength;
	}

	//获取显示的字节范围.
	pDataPtr = m_pData + nAdr;
	pEndDataPtr = m_pData + nEndAdr;

	//  paint
	while (pDataPtr < pEndDataPtr) {

		//获取当前行要显示的字节.
		pEndLineDataPtr = pDataPtr + m_tPaintDetails.nBytesPerRow;
		if (pEndLineDataPtr > pEndDataPtr) {
			pEndLineDataPtr = pEndDataPtr;
		}

		//当前行中属于选中范围的字节.
		print_selected_begin = NULL;
		print_selected_end = NULL;

		for (print_end = print_buf; pDataPtr < pEndLineDataPtr; ++pDataPtr) {
			//判断该字节是否在选中范围.
			if (pDataPtr >= pSelectionPtrBegin &&
				pDataPtr < pSelectionPtrEnd){

				if (!print_selected_begin)
					print_selected_begin = print_end;

				print_selected_end = print_end + 2;
			}

			*print_end++ = tabHexCharacters[*pDataPtr >> 4];
			*print_end++ = tabHexCharacters[*pDataPtr & 0xf];
			
			for (int i = 0; i < HEX_SPACE; i++)
				*print_end++ = ' ';
		}

		print_end -= HEX_SPACE;
		*print_end = '\0';

		// first draw all normal
		cDC.SetBkColor(0xffffff);
		cDC.SetTextColor(0x0);
		cDC.DrawText(print_buf, (LPRECT)cHexRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

		// selection
		if (print_selected_begin != NULL) {
			bool bHasFocus = GetFocus() == this;

			// todo: flag für show selection always
			if (bHasFocus || (GetStyle() & ES_MULTILINE)) { 
				//只重新绘制选中的部分...
				CRect cRect(cHexRect);
				cRect.left += (print_selected_begin - print_buf) * m_tPaintDetails.nCharacterWidth;
				cRect.right -= (print_end - print_selected_end) * m_tPaintDetails.nCharacterWidth;
				
				CRect cSelectionRect(cRect);
				cSelectionRect.InflateRect(0, -1, +1, 0);
				//cDC.FillRect(cSelectionRect, &CBrush(m_tSelectedFousBkgCol));
				char old_ch = *print_selected_end;

				*print_selected_end = '\0'; // set "end-mark"

				cDC.SetTextColor(m_tSelectedTxtCol);
				cDC.SetBkColor(m_tSelectedBkgCol);
				cDC.DrawText(print_selected_begin, (LPRECT)cRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
				
				*print_selected_end = old_ch; // restore the buffer
			}
		}

		//绘制下一行.
		cHexRect.OffsetRect(0, m_tPaintDetails.nLineHeight);
	}
}

//绘制ascii数据
void CHexEditCtrl::PaintAsciiData(CDC& cDC)
{
	ASSERT(m_tPaintDetails.nBytesPerRow > 0);
	ASSERT(m_tPaintDetails.nBytesPerRow < 1021);

	if ((m_nLength < 1) || (m_pData == NULL))
		return;

	UINT nAdr;
	UINT nEndAdr;
	UINT nSelectionCount = 0;
	
	TCHAR print_buff[1024];
	TCHAR *print_end;
	TCHAR *line_selected_begin;
	TCHAR *print_selected_end;

	BYTE *pDataPtr;
	BYTE *pEndDataPtr;
	BYTE *pEndLineDataPtr;
	BYTE *pSelectionPtrBegin;
	BYTE *pSelectionPtrEnd;
	CRect cAsciiRect(m_tPaintDetails.cPaintingRect);
	CBrush cBkgBrush;
	CBrush *pOldBrush = NULL;

	// the Rect for painting & background
	cBkgBrush.CreateStockObject(WHITE_BRUSH);
	cAsciiRect.left += m_tPaintDetails.nAsciiPos - m_nScrollPostionX;
	cAsciiRect.right = cAsciiRect.left + m_tPaintDetails.nAsciiLen;
	cDC.FillRect(cAsciiRect, &cBkgBrush);
	cAsciiRect.bottom = cAsciiRect.top + m_tPaintDetails.nLineHeight;

	// selection (pointers) ,,看看当前有没有选中区域.
	if ((m_nSelectionBegin != NOSECTION_VAL) && (m_nSelectionEnd != NOSECTION_VAL)) {
		pSelectionPtrBegin = m_pData + m_nSelectionBegin;
		pSelectionPtrEnd = m_pData + m_nSelectionEnd;
	}
	else {
		pSelectionPtrBegin = NULL;
		pSelectionPtrEnd = NULL;
	}

	// start & end-address (& pointers)
	nAdr = m_nScrollPostionY * m_tPaintDetails.nBytesPerRow;
	nEndAdr = nAdr + m_tPaintDetails.nVisibleLines * m_tPaintDetails.nBytesPerRow;
	if (nEndAdr >= m_nLength) {
		nEndAdr = m_nLength;
	}

	//获取显示的字节范围.
	pDataPtr = m_pData + nAdr;
	pEndDataPtr = m_pData + nEndAdr;

	//  paint
	while (pDataPtr < pEndDataPtr) {

		//获取当前行要显示的字节.
		pEndLineDataPtr = pDataPtr + m_tPaintDetails.nBytesPerRow;
		if (pEndLineDataPtr > pEndDataPtr) {
			pEndLineDataPtr = pEndDataPtr;
		}

		//当前行中属于选中范围的字节.
		line_selected_begin = NULL;				//
		print_selected_end = NULL;				//

		for (print_end = print_buff; pDataPtr < pEndLineDataPtr; ++pDataPtr) {
			//判断该字节是否在选中范围.
			if (pDataPtr >= pSelectionPtrBegin &&
				pDataPtr < pSelectionPtrEnd){

				if (!line_selected_begin)
					line_selected_begin = print_end;

				print_selected_end = print_end + 1;
			}
			//输出到print buff.
			*print_end++ = isprint(*pDataPtr) ? *pDataPtr : '.';
			for (int i = 0; i < ASCII_SPACE; i++)
				*print_end++ = ' ';

		}
		
		print_end -= ASCII_SPACE;
		*print_end = '\0';

		// first draw all normal
		cDC.SetBkColor(0xffffff);
		cDC.SetTextColor(0x0);
		cDC.DrawText(print_buff, (LPRECT)cAsciiRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

		// selection
		if (line_selected_begin != NULL) {
			bool bHasFocus = GetFocus() == this;

			// todo: flag für show selection always
			if (bHasFocus || (GetStyle() & ES_MULTILINE)) 
			{
				//只重新绘制选中的部分...
				CRect cRect(cAsciiRect);
				cRect.left += (line_selected_begin - print_buff) * m_tPaintDetails.nCharacterWidth;
				cRect.right -= (print_end - print_selected_end)* m_tPaintDetails.nCharacterWidth;

				CRect cSelectionRect(cRect);
				cSelectionRect.InflateRect(0, -1, +1, 0);
				//cDC.FillRect(cSelectionRect, &CBrush(m_tSelectedFousBkgCol));

				char old_ch = *print_selected_end;
				*print_selected_end = '\0'; // set "end-mark"

				cDC.SetTextColor(m_tSelectedTxtCol);
				cDC.SetBkColor(m_tSelectedBkgCol);
				cDC.DrawText(line_selected_begin, (LPRECT)cRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

				*print_selected_end = old_ch; // restore the buffer
			}
		}

		//绘制下一行.
		cAsciiRect.OffsetRect(0, m_tPaintDetails.nLineHeight);
	}
}


//绘图消息.
void CHexEditCtrl::OnPaint()
{
	CPaintDC cPaintDC(this);
	CRect cClientRect;
	CDC	cMemDC;
	CBitmap cBmp;
	CBitmap *pOldBitmap;
	CFont *pOldFont;
	CBrush cBackBrush;

	// memorybuffered output (via a memorybitmap)
	cMemDC.CreateCompatibleDC(&cPaintDC);
	GetClientRect(cClientRect);	
	cBmp.CreateCompatibleBitmap(&cPaintDC, cClientRect.right, cClientRect.bottom);
	pOldBitmap = cMemDC.SelectObject(&cBmp);
	pOldFont = cMemDC.SelectObject(&m_cFont);
	
	//是否重新计算布局坐标.
	if(m_bRecalc)
		CalculatePaintingDetails(cMemDC);


	//填充背景.
	cBackBrush.CreateStockObject(WHITE_BRUSH);
	cMemDC.FillRect(cClientRect, &cBackBrush);
	
	CRgn cRegn;
	CRect cRect(m_tPaintDetails.cPaintingRect);
	cRect.left-=2;
	cRect.right+=2;
	cRegn.CreateRectRgnIndirect((LPCRECT)cRect);
	cMemDC.SelectClipRgn(&cRegn);

	//绘制地址区域
	PaintAddresses(cMemDC);

	//hex 区域
	PaintHexData(cMemDC);

	//ascii区域.
	PaintAsciiData(cMemDC);

	//拷贝到屏幕DC.
	cPaintDC.BitBlt(0, 0, cClientRect.right, cClientRect.bottom, &cMemDC, 0, 0, SRCCOPY);

	if(pOldFont != NULL) {
		cMemDC.SelectObject(pOldFont);
	}

	if(pOldBitmap != NULL) {
		cMemDC.SelectObject(pOldBitmap);
	}
}

//使目标位置的字节可见.
void CHexEditCtrl::MakeVisible(UINT nBegin, UINT nEnd, bool bUpdate)
{
	ASSERT(nBegin<=nEnd);

	UINT nAdrBeg = m_nScrollPostionY * m_tPaintDetails.nBytesPerRow;
	UINT nFullBytesPerScreen = m_tPaintDetails.nFullVisibleLines * m_tPaintDetails.nBytesPerRow;
	UINT nAdrEnd = nAdrBeg + nFullBytesPerScreen;
	UINT nLength = nEnd - nBegin;
	if( (nBegin > nAdrBeg) || (nEnd < nAdrEnd) ) {
		// don't do anything when it's simply not possible to see everything and
		// we already see one ful page.
		if(nLength > nFullBytesPerScreen) {
			if(nAdrBeg < nBegin) {
				SetScrollPositionY(nBegin/m_tPaintDetails.nBytesPerRow, false);
			} else if (nAdrEnd > nEnd) {
				SetScrollPositionY((nEnd-nFullBytesPerScreen+m_tPaintDetails.nBytesPerRow)/m_tPaintDetails.nBytesPerRow, false); 
			}
		} else {
			if(nAdrBeg > nBegin) {
				SetScrollPositionY(nBegin/m_tPaintDetails.nBytesPerRow, false);
			} else if (nAdrEnd < nEnd) {
				SetScrollPositionY((nEnd-nFullBytesPerScreen+m_tPaintDetails.nBytesPerRow)/m_tPaintDetails.nBytesPerRow, false); 
			}
		}
	}

	int iLineX = (int)((nBegin%m_tPaintDetails.nBytesPerRow)*3*m_tPaintDetails.nCharacterWidth + m_tPaintDetails.nHexPos + m_tPaintDetails.cPaintingRect.left) - (int)m_nScrollPostionX;
	int iLineX2 = (int)((2+(nEnd%m_tPaintDetails.nBytesPerRow)*3)*m_tPaintDetails.nCharacterWidth + m_tPaintDetails.nHexPos + m_tPaintDetails.cPaintingRect.left) - (int)m_nScrollPostionX;
	
	if(iLineX > iLineX2) {
		int iTemp = iLineX;
		iLineX = iLineX2;
		iLineX2 = iTemp;
	}

	if( (iLineX <= m_tPaintDetails.cPaintingRect.left) && (iLineX2 >= m_tPaintDetails.cPaintingRect.right) ) {
		// nothing to do here...
	} else if(iLineX < m_tPaintDetails.cPaintingRect.left) {
		SetScrollPositionX(m_nScrollPostionX + iLineX - m_tPaintDetails.cPaintingRect.left, false);
	} else if(iLineX2 >= m_tPaintDetails.cPaintingRect.right) {
		SetScrollPositionX(m_nScrollPostionX + iLineX2 - m_tPaintDetails.cPaintingRect.Width(), false);
	}

	if(bUpdate && ::IsWindow(m_hWnd)) {
		Invalidate();
	}
}

void CHexEditCtrl::SetScrollbarRanges()
{
	if(!(GetStyle() & ES_MULTILINE)) {
		return;
	}

	SCROLLINFO tScrollInfo;
	memset(&tScrollInfo, 0, sizeof(SCROLLINFO));
	tScrollInfo.cbSize = sizeof(SCROLLINFO);
	if(m_nScrollRangeY > 0) {
		ShowScrollBar(SB_VERT, TRUE);
		EnableScrollBar(SB_VERT);
		tScrollInfo.fMask = SIF_ALL ;
		tScrollInfo.nPage = m_tPaintDetails.nFullVisibleLines;
		tScrollInfo.nMax = m_nScrollRangeY + tScrollInfo.nPage - 1;
		if(m_nScrollPostionY > m_nScrollRangeY) {
			m_nScrollPostionY = m_nScrollRangeY;
		}
		tScrollInfo.nPos = m_nScrollPostionY;
		SetScrollInfo(SB_VERT, &tScrollInfo, TRUE);
	} else {
		ShowScrollBar(SB_VERT, FALSE);
	}
	if(m_nScrollRangeX > 0) {
		EnableScrollBar(SB_HORZ);
		ShowScrollBar(SB_HORZ, TRUE);
		tScrollInfo.fMask = SIF_ALL ;
		tScrollInfo.nPage = m_tPaintDetails.cPaintingRect.Width();
		tScrollInfo.nMax = m_nScrollRangeX + tScrollInfo.nPage - 1;
		if(m_nScrollPostionX > m_nScrollRangeX) {
			m_nScrollPostionX = m_nScrollRangeX;
		}
		tScrollInfo.nPos = m_nScrollPostionX;
		SetScrollInfo(SB_HORZ, &tScrollInfo, TRUE);
	} else {
		ShowScrollBar(SB_HORZ, FALSE);
	}
}

void CHexEditCtrl::OnSetFocus(CWnd*)
{	
	SetEditCaretPos(m_nCurrentAddress, m_bHighBits);
	Invalidate();
}

void CHexEditCtrl::OnKillFocus(CWnd* pNewWnd)
{
	DestoyEditCaret();
	CWnd::OnKillFocus(pNewWnd);
	Invalidate();
}

void CHexEditCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	CalculatePaintingDetails(CClientDC(this));
	SetEditCaretPos(m_nCurrentAddress, m_bHighBits);
	SetScrollbarRanges();
}

void CHexEditCtrl::MoveScrollPostionY(int iDelta, bool bUpdate)
{
	if(iDelta > 0) {
		SetScrollPositionY(m_nScrollPostionY+iDelta, bUpdate);
	} else {
		int iPositon = (int)m_nScrollPostionY;
		iPositon -= (-iDelta);
		if(iPositon < 0) {
			iPositon = 0;
		}
		SetScrollPositionY((UINT)iPositon, bUpdate);
	}
}

void CHexEditCtrl::MoveScrollPostionX(int iDelta, bool bUpdate)
{
	if(iDelta > 0) {
		SetScrollPositionX(m_nScrollPostionX+iDelta, bUpdate);
	} else {
		int iPositon = (int)m_nScrollPostionX;
		iPositon -= (-iDelta);
		if(iPositon < 0) {
			iPositon = 0;
		}
		SetScrollPositionX((UINT)iPositon, bUpdate);
	}
}

void CHexEditCtrl::GetAddressFromPoint(const CPoint& cPt, UINT& nAddress, bool& bHighBits)
{	
	CPoint cPoint(cPt);
	cPoint.x += m_nScrollPostionX;
	cPoint.y -= m_tPaintDetails.cPaintingRect.top;

	if((GetStyle() & ES_MULTILINE)) {
		cPoint.x += (m_tPaintDetails.nCharacterWidth>>1) - CONTROL_BORDER_SPACE ;
	} else {
		cPoint.x += (m_tPaintDetails.nCharacterWidth>>1);
	}

	if(cPoint.y < 0) {
		cPoint.y = 0;
	} else if(cPoint.y > (int)(m_tPaintDetails.nVisibleLines*m_tPaintDetails.nLineHeight)) {
		cPoint.y = m_tPaintDetails.nVisibleLines*m_tPaintDetails.nLineHeight;
	}

	if((int)cPoint.x < (int)m_tPaintDetails.nHexPos) {
		cPoint.x = m_tPaintDetails.nHexPos;
	} else if(cPoint.x > (int)(m_tPaintDetails.nHexPos + m_tPaintDetails.nHexLen - DATA_ASCII_SPACE)) {
		cPoint.x = m_tPaintDetails.nHexPos + m_tPaintDetails.nHexLen - DATA_ASCII_SPACE;
	}

	cPoint.x -= m_tPaintDetails.nHexPos;
	UINT nRow = cPoint.y / m_tPaintDetails.nLineHeight;
	UINT nCharColumn  = cPoint.x / m_tPaintDetails.nCharacterWidth;
	UINT nColumn = nCharColumn / (2 + HEX_SPACE);

	bHighBits = nCharColumn % (2 + HEX_SPACE) == 0;
	nAddress = nColumn + (nRow + m_nScrollPostionY) * m_tPaintDetails.nBytesPerRow;

	if(nAddress >= m_nLength) {
		nAddress = m_nLength;
		bHighBits = false;
	}
}

BOOL CHexEditCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint)
{
	MoveScrollPostionY(-(zDelta/WHEEL_DELTA), true);
	return TRUE;
}

void CHexEditCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*)
{
	if(m_pData == NULL) {
		return;
	}

	switch(nSBCode) {
	case SB_LINEDOWN:
		MoveScrollPostionY(1, true);
		break;
	
	case SB_LINEUP:
		MoveScrollPostionY(-1, true);
		break;
	
	case SB_PAGEDOWN:
		MoveScrollPostionY(m_tPaintDetails.nFullVisibleLines, true);
		break;

	case SB_PAGEUP:
		MoveScrollPostionY(-(int)m_tPaintDetails.nFullVisibleLines, true);
		break;

	case SB_THUMBTRACK:
		// Windows only allows 16Bit track-positions in the callback message.
		// MFC hides this by providing a 32-bit value (nobody expects to
		// be an invalid value) which is unfortunately casted from a 16Bit value.
		// -- MSDN gives a hint (in the API-documentation) about this problem
		// -- and a solution as well. We should use GetScrollInfo here to receive
		// -- the correct 32-Bit value when our scrollrange exceeds the 16bit range
		// -- to keep it simple, I decided to always do it like this
		SCROLLINFO tScrollInfo;
		memset(&tScrollInfo, 0, sizeof(SCROLLINFO));
		if(GetScrollInfo(SB_VERT, &tScrollInfo, SIF_TRACKPOS)) {
			SetScrollPositionY(tScrollInfo.nTrackPos, true);
		}
#ifdef _DEBUG
		else {
			TRACE("CHexEditBase::OnVScroll: Error receiving trackposition while thumbtracking\n");
		}
#endif
		break;
	}
}

void CHexEditCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*)
{
	if(m_pData == NULL) {
		return;
	}

	switch(nSBCode) {
	case SB_LINEDOWN:
		MoveScrollPostionX(m_tPaintDetails.nCharacterWidth, true);
		break;
	
	case SB_LINEUP:
		MoveScrollPostionX(-(int)m_tPaintDetails.nCharacterWidth, true);
		break;
	
	case SB_PAGEDOWN:
		MoveScrollPostionX(m_tPaintDetails.cPaintingRect.Width(), true);
		break;

	case SB_PAGEUP:
		MoveScrollPostionX(-(int)m_tPaintDetails.cPaintingRect.Width(), true);
		break;

	case SB_THUMBTRACK:
		SetScrollPositionX(nPos, true);
		break;
	}
}

//设置光标的位置.
void CHexEditCtrl::SetEditCaretPos(UINT nOffset, bool bHighBits)
{	
	ASSERT(::IsWindow(m_hWnd));
	
	m_nCurrentAddress = nOffset;
	m_bHighBits = bHighBits;
		
	if(m_bRecalc) {
		CalculatePaintingDetails(CClientDC(this));
	}

	if(m_nCurrentAddress < m_nScrollPostionY * m_tPaintDetails.nBytesPerRow 
		|| (m_nCurrentAddress >= (m_nScrollPostionY + m_tPaintDetails.nVisibleLines)*m_tPaintDetails.nBytesPerRow) ) {
		// not in the visible range
		DestoyEditCaret();
		return;
	}

	if(GetFocus() != this) {
		// in case we missed once something...
		DestoyEditCaret();
		return;
	}

	UINT nRelAdr = m_nCurrentAddress - m_nScrollPostionY * m_tPaintDetails.nBytesPerRow;
	UINT nRow = nRelAdr / m_tPaintDetails.nBytesPerRow;
	UINT nColumn = nRelAdr % m_tPaintDetails.nBytesPerRow;
	UINT nCarretHeight;
	UINT nCarretWidth = m_tPaintDetails.nCharacterWidth;
	
	// last row can be only half visible
	if(nRow == m_tPaintDetails.nVisibleLines-1) {
		nCarretHeight = m_tPaintDetails.nLastLineHeight;
	} else {
		nCarretHeight = m_tPaintDetails.nLineHeight;
	}

	CPoint cCarretPoint(m_tPaintDetails.cPaintingRect.left 
		- m_nScrollPostionX + m_tPaintDetails.nHexPos 
		+ (nColumn * (HEX_SPACE + 2) + (bHighBits ? 0 : 1)) * m_tPaintDetails.nCharacterWidth,
		m_tPaintDetails.cPaintingRect.top + 1 + nRow * m_tPaintDetails.nLineHeight);
	
	if( (cCarretPoint.x + (short)m_tPaintDetails.nCharacterWidth <= m_tPaintDetails.cPaintingRect.left-2 ) 
		|| (cCarretPoint.x > m_tPaintDetails.cPaintingRect.right) ) {
		// we can't see it
		DestoyEditCaret();
		return;
	}

	if(cCarretPoint.x < m_tPaintDetails.cPaintingRect.left-2) {
		nCarretWidth -= m_tPaintDetails.cPaintingRect.left-2 - cCarretPoint.x;
		cCarretPoint.x = m_tPaintDetails.cPaintingRect.left-2;
	}

	if(cCarretPoint.x + (int)nCarretWidth > (int)m_tPaintDetails.cPaintingRect.right+2) {
		nCarretWidth = m_tPaintDetails.cPaintingRect.right + 2 - cCarretPoint.x;
	}

	CreateEditCaret(nCarretHeight-1, nCarretWidth);
	SetCaretPos(cCarretPoint);
	ShowCaret();
}

void CHexEditCtrl::CreateEditCaret(UINT nCaretHeight, UINT nCaretWidth)
{
	if(!m_bHasCaret || (nCaretHeight != m_nCurCaretHeight) 
		|| (nCaretWidth != m_nCurCaretWidth) ) {
		m_bHasCaret = true;

		m_nCurCaretHeight = nCaretHeight;
		m_nCurCaretWidth = nCaretWidth;

		CreateSolidCaret(1, m_nCurCaretHeight);
	}
}

void CHexEditCtrl::DestoyEditCaret() {
	m_bHasCaret = false;
	DestroyCaret();
}

UINT CHexEditCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

BOOL CHexEditCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	return CWnd::PreCreateWindow(cs);
}

BOOL CHexEditCtrl::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	if(!CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext)) {
		return FALSE;
	}
	return TRUE;
}

BOOL CHexEditCtrl::OnEraseBkgnd(CDC*)
{
	return TRUE;
}

void CHexEditCtrl::OnLButtonDown(UINT, CPoint point)
{
	SetFocus();
	if(m_pData == NULL)
		return;
	
	//获取点击的地址偏移是多少...
	GetAddressFromPoint(point, m_nCurrentAddress, m_bHighBits);
	//设置光标的位置.
	SetEditCaretPos(m_nCurrentAddress, m_bHighBits);

	int iDragCX = GetSystemMetrics(SM_CXDRAG);
	int iDragCY = GetSystemMetrics(SM_CYDRAG);

	m_cDragRect = CRect(point.x - (iDragCX>>1), point.y - (iDragCY>>1),
		point.x + (iDragCX>>1) + (iDragCX&1), //(we don't want to loose a pixel, when it's so small)
		point.y + (iDragCY>>1) + (iDragCY&1));

	m_nSelectingEnd = NOSECTION_VAL;
	m_nSelectionBegin = NOSECTION_VAL;
	m_nSelectingEnd = NOSECTION_VAL;			//这几个值是还没确定的意思.

	m_nSelectingBeg = m_nCurrentAddress;
	
	SetCapture();
}

void CHexEditCtrl::OnLButtonUp(UINT, CPoint)
{
	if(GetCapture() == this)
		ReleaseCapture();
	
	StopMouseRepeat(); // in case it's started, otherwise it doesn't matter either
	Invalidate();
}

void CHexEditCtrl::OnLButtonDblClk(UINT, CPoint point)
{
	SetFocus();
	if(m_pData == NULL)
		return;

	GetAddressFromPoint(point, m_nCurrentAddress, m_bHighBits);
	SetEditCaretPos(m_nCurrentAddress, m_bHighBits);
	
	m_nSelectionEnd = m_nCurrentAddress;
	m_nSelectionBegin = m_nCurrentAddress;

	Invalidate();
}

void CHexEditCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_pData == NULL)
		return;
	
	if( (nFlags & MK_LBUTTON) && (m_nSelectingBeg != NOSECTION_VAL)) {

		// first make a self built drag-detect (one that doesn't block)
		if(!m_cDragRect.PtInRect(point)) {
			m_cDragRect = CRect(-1, -1, -1, -1); // when once, out, kill it...
		} else {
			return; // okay, still not draging
		}

		if( !m_tPaintDetails.cPaintingRect.PtInRect(point) && (GetStyle()&ES_MULTILINE) ) {
			int iRepSpeed = 0;
			int iDelta = 0;
			if(point.y < m_tPaintDetails.cPaintingRect.top) {
				iDelta = -1;
				iRepSpeed = (int)m_tPaintDetails.cPaintingRect.top + 1 - (int)point.y;
			} else if(point.y > m_tPaintDetails.cPaintingRect.bottom ) {
				iDelta = 1;
				iRepSpeed = (int)point.y - (int)m_tPaintDetails.cPaintingRect.bottom + 1;
			}
			if(iDelta != 0) {
				iRepSpeed /= 5;
				if(iRepSpeed > 5) {
					iRepSpeed = 6;
				}
				StartMouseRepeat(point, iDelta, (short)(7 - iRepSpeed));
			}
			m_cMouseRepPoint = point; // make sure we always have the latest point
		} else {
			StopMouseRepeat();
		}

		GetAddressFromPoint(point, m_nCurrentAddress, m_bHighBits);
		SetEditCaretPos(m_nCurrentAddress, m_bHighBits);

		m_nSelectingEnd = m_nCurrentAddress;

		m_nSelectionBegin = m_nSelectingBeg;
		m_nSelectionEnd = m_nSelectingEnd;

		//确保begin <= end.
		NORMALIZE_SELECTION(m_nSelectionBegin, m_nSelectionEnd);
		Invalidate();
	}
}

void CHexEditCtrl::OnTimer(UINT nTimerID)
{
	if( (m_pData == NULL) || (m_nLength < 1) )
		return;

	//鼠标点住拖到外面之后自动滚动
	if(m_bIsMouseRepActive && (nTimerID == MOUSEREP_TIMER_TID) ) {
		if(m_nMouseRepCounter > 0) {
			m_nMouseRepCounter--;
		} else {
			m_nMouseRepCounter = m_nMouseRepSpeed;
			MoveScrollPostionY(m_iMouseRepDelta, false);
			GetAddressFromPoint(m_cMouseRepPoint, m_nCurrentAddress, m_bHighBits);
			SetEditCaretPos(m_nCurrentAddress, m_bHighBits);
			m_nSelectingEnd = m_nCurrentAddress;
			m_nSelectionBegin = m_nSelectingBeg;
			m_nSelectionEnd = m_nSelectingEnd;
			NORMALIZE_SELECTION(m_nSelectionBegin, m_nSelectionEnd);
			Invalidate();
		}
	}
}

void CHexEditCtrl::StartMouseRepeat(const CPoint& cPoint, int iDelta, WORD nSpeed)
{
	if( (m_pData == NULL) || (m_nLength < 1) ) {
		return;
	}

	m_cMouseRepPoint = cPoint;
	m_nMouseRepSpeed = nSpeed;
	m_iMouseRepDelta = iDelta;
	if(!m_bIsMouseRepActive && (GetStyle() & ES_MULTILINE)) {
		m_bIsMouseRepActive = true;
		m_nMouseRepCounter = nSpeed;
		SetTimer(MOUSEREP_TIMER_TID, MOUSEREP_TIMER_ELAPSE, NULL);			
	}
}

void CHexEditCtrl::StopMouseRepeat()
{
	if(m_bIsMouseRepActive) {
		m_bIsMouseRepActive = false;
		KillTimer(MOUSEREP_TIMER_TID);
	}
}

bool CHexEditCtrl::OnEditInput(WORD nInput)
{
	ASSERT(m_nCurrentAddress <= m_nLength);
	if ((nInput > 255) ) {
		return false;
	}

	BYTE nValue = 255;
	char nKey = (char)tolower(nInput);
	if ((nKey >= 'a') && (nKey <= 'f')) {
		nValue = nKey - (BYTE)'a' + (BYTE)0xa;
	}
	else if ((nKey >= '0') && (nKey <= '9')) {
		nValue = nKey - (BYTE)'0';
	}

	if (nValue == 255)
		return false;

	if (m_nCurrentAddress < m_nLength &&
		!m_bHighBits){

		m_pData[m_nCurrentAddress] &= 0xf0;
		m_pData[m_nCurrentAddress] |= nValue;
		MoveCurrentAddress(1, true);

		Invalidate();
		NotifyParent(HEN_CHANGE);
		return true;
	}

	//m_nCurrentAddress == m_nLength or insert a byte(m_bHighBits)
	//grow buffer.
	if (m_nLength + 1 > m_BuffSize){
		if (!m_BuffSize)
			m_BuffSize = 0x1000;
		else
			m_BuffSize <<= 1;
		m_pData = (BYTE*)realloc(m_pData, m_BuffSize);
	}

	//append null byte.
	if (m_nCurrentAddress < m_nLength){	
		memmove(m_pData + m_nCurrentAddress + 1,
			m_pData + m_nCurrentAddress, m_nLength - m_nCurrentAddress);
	}

	m_pData[m_nCurrentAddress] = 0x00;
	m_bRecalc = true;
	m_nLength += 1;

	if (m_bHighBits) {
		nValue <<= 4;
		m_pData[m_nCurrentAddress] &= 0x0f;
		m_pData[m_nCurrentAddress] |= nValue;
		MoveCurrentAddress(0, false);
	}

	Invalidate();
	NotifyParent(HEN_CHANGE);
	return true;
}

LRESULT CHexEditCtrl::OnUmSetScrollRange(WPARAM, LPARAM)
{
	SetScrollbarRanges();
	return 0;
}

LRESULT CHexEditCtrl::OnWMSetFont(WPARAM wParam, LPARAM lParam)
{
	if(wParam != NULL) {
		CFont *pFont = CFont::FromHandle((HFONT)wParam);
		if(pFont != NULL) {
			LOGFONT tLogFont;
			memset(&tLogFont, 0, sizeof(LOGFONT));
			if( pFont->GetLogFont(&tLogFont) && ((tLogFont.lfPitchAndFamily & 3) == FIXED_PITCH) ) {
				if((HFONT)m_cFont != NULL) {
					m_cFont.DeleteObject();
					ASSERT((HFONT)m_cFont == NULL);
				}	
				m_cFont.CreateFontIndirect(&tLogFont);
			}
		}
	}
	if((HFONT)m_cFont == NULL) {
		//if we failed so far, we just create a new system font 
		m_cFont.CreateStockObject(SYSTEM_FIXED_FONT);
	}

	m_bRecalc = true;
	if(lParam && ::IsWindow(m_hWnd)) {
		Invalidate();
	}
	return 0; // no return value needed
}

LRESULT CHexEditCtrl::OnWMGetFont(WPARAM wParam, LPARAM lParam)
{
	wParam = 0;
	lParam = 0;
	return (LRESULT)((HFONT)m_cFont);
}

LRESULT CHexEditCtrl::OnWMChar(WPARAM wParam, LPARAM)
{

	if(wParam == 0x08) {			
		OnEditBackspace(); 
		return 0;
	}else {
		if(OnEditInput((WORD)wParam)) {
			return 0;
		}
	}
	return 1;
}


void CHexEditCtrl::SetScrollPositionY(UINT nPosition, bool bUpdate)
{
	if(!(GetStyle() & ES_MULTILINE)) {
		return;
	}
	if(nPosition > m_nScrollRangeY) {
		nPosition = m_nScrollRangeY;
	}
	SetScrollPos(SB_VERT, (int)nPosition, TRUE);
	if( (nPosition != m_nScrollPostionY) && bUpdate && ::IsWindow(m_hWnd) ) {
		m_nScrollPostionY = nPosition;
		Invalidate();
	}
	SetEditCaretPos(m_nCurrentAddress, m_bHighBits);
	m_nScrollPostionY = nPosition;
}

void CHexEditCtrl::SetScrollPositionX(UINT nPosition, bool bUpdate)
{
	if(nPosition > m_nScrollRangeX) {
		nPosition = m_nScrollRangeX;
	}
	SetScrollPos(SB_HORZ, (int)nPosition, TRUE);
	if((nPosition != m_nScrollPostionX) && bUpdate && ::IsWindow(m_hWnd) ) {
		m_nScrollPostionX = nPosition;
		Invalidate();
		SetEditCaretPos(m_nCurrentAddress, m_bHighBits);
	}
	m_nScrollPostionX = nPosition;
}

void CHexEditCtrl::MoveCurrentAddress(int iDeltaAdr, bool bNextIsHighBits)
{
	bool bIsShift = (GetKeyState(VK_SHIFT) & 0x80000000) == 0x80000000;

	if(m_pData == NULL)
		return;
		
	UINT nAddress = m_nCurrentAddress;

	if(!bIsShift) {
		m_nSelectingBeg = NOSECTION_VAL;
		m_nSelectionBegin = NOSECTION_VAL;
		m_nSelectingEnd = NOSECTION_VAL;
		m_nSelectionEnd = NOSECTION_VAL;
	}

	if(iDeltaAdr > 0) {
		// go down
		if(nAddress + iDeltaAdr >= m_nLength) {
			// we reached the end
			nAddress = m_nLength; // -1;
			bNextIsHighBits = true;
		} else {
			nAddress += iDeltaAdr;
		}
	} else if (iDeltaAdr < 0) {
		if((UINT)(-iDeltaAdr) <= nAddress) {
			nAddress -= (UINT)(-iDeltaAdr);
		} else {
			nAddress = 0;
			bNextIsHighBits = true;
		}
	} 

	if(bIsShift && (m_nSelectingBeg != NOSECTION_VAL)) {
		m_nSelectingEnd = nAddress;
		m_nSelectionBegin = m_nSelectingBeg;
		m_nSelectionEnd = m_nSelectingEnd;
		NORMALIZE_SELECTION(m_nSelectionBegin, m_nSelectionEnd);
	}

	MakeVisible(nAddress, nAddress + 1, true);
	SetEditCaretPos(nAddress, bNextIsHighBits);
}

void CHexEditCtrl::OnKeyDown(UINT nChar, UINT, UINT)
{
	bool bIsShift = (GetKeyState(VK_SHIFT) & 0x80000000) == 0x80000000;

	//shift is pressed.
	if( bIsShift && (m_nSelectingBeg == NOSECTION_VAL) ) {
		m_nSelectingBeg = m_nCurrentAddress;
	}

	switch(nChar) {
	case VK_DOWN:
		MoveCurrentAddress(m_tPaintDetails.nBytesPerRow, m_bHighBits);
		break;
	case VK_UP:
		MoveCurrentAddress(-(int)m_tPaintDetails.nBytesPerRow, m_bHighBits);
		break;
	case VK_RIGHT:
		if(m_bHighBits) {
			MoveCurrentAddress(0, false);		//goto low bits of current byte.
		} else if(m_nCurrentAddress < m_nLength) {
			MoveCurrentAddress(1, true);		//goto high bits of next byte
		}
		break;
	case VK_LEFT:
		if(!m_bHighBits) {	// offset stays the same, caret moves to high-byte
			MoveCurrentAddress(0, true);
		} else {
			MoveCurrentAddress(-1, false);
		}
		break;
	case VK_PRIOR:
		MoveCurrentAddress(-(int)(m_tPaintDetails.nBytesPerRow*(m_tPaintDetails.nVisibleLines-1)), m_bHighBits);
		break;
	case VK_NEXT:
		MoveCurrentAddress(m_tPaintDetails.nBytesPerRow*(m_tPaintDetails.nVisibleLines-1), m_bHighBits);
		break;
	case VK_HOME:
		MoveCurrentAddress(-0x8000000, true);
		break;
	case VK_END:
		MoveCurrentAddress(0x7ffffff, false);
		break;
	case VK_INSERT:
		// not suported yet
		break;
	case VK_RETURN:
		// not suported yet
		break;
	case VK_TAB:
		GetParent()->GetNextDlgTabItem(this, bIsShift)->SetFocus();
		break;
	}
}

void CHexEditCtrl::OnContextMenu(CWnd*, CPoint cPoint)
{	
	CString cString;

	if( (cPoint.x == -1) && (cPoint.y == -1) ) {
		//keystroke invocation
		cPoint = CPoint(5, 5);
		ClientToScreen(&cPoint);
	} else {
		CPoint cRelPoint(cPoint);
		ScreenToClient(&cRelPoint);
		bool bHigh;
		UINT nAdr;
		GetAddressFromPoint(cRelPoint, nAdr, bHigh);
		if( !IsSelection() || (nAdr < m_nSelectionBegin) || (nAdr > m_nSelectionEnd) ) {
			// no selection or outside of selection
			if(IsSelection()) {
				// kill selection
				SetSelection(NOSECTION_VAL, NOSECTION_VAL, false, true);
			}
			SetEditCaretPos(nAdr, true); //always high, because of paste...
		}
	}
}


void CHexEditCtrl::SetData(const BYTE *pData, UINT nLen, bool bUpdate)
{
	m_nSelectingBeg = NOSECTION_VAL;
	m_nSelectingEnd = NOSECTION_VAL;
	m_nSelectionBegin = NOSECTION_VAL;
	m_nSelectionEnd = NOSECTION_VAL;

	if (m_pData) {
		delete []m_pData;
		m_pData = NULL;
	}
	
	if(pData != NULL) {
		m_nLength = nLen;
		m_BuffSize = m_nLength;
		m_pData = new BYTE[nLen];
		memcpy(m_pData, pData, nLen);
	} else {
		m_nLength = 0;
		m_pData = NULL;
	}

	m_bRecalc = true;
	
	if(bUpdate && ::IsWindow(m_hWnd)) {
		Invalidate();
	}

	SetEditCaretPos(0, true);
}


bool CHexEditCtrl::IsSelection() const
{
	return (m_nSelectionEnd != NOSECTION_VAL) && (m_nSelectionBegin != NOSECTION_VAL);
}

bool CHexEditCtrl::GetSelection(UINT& nBegin, UINT& nEnd) const
{
	if(IsSelection()) {
		nBegin = m_nSelectionBegin;
		nEnd = m_nSelectionEnd;
		return true;
	}
	nBegin = NOSECTION_VAL;
	nEnd = NOSECTION_VAL;
	return false;
}


void CHexEditCtrl::SetSelection(UINT nBegin, UINT nEnd, bool bMakeVisible, bool bUpdate)
{
	ASSERT(m_nLength > 0);
	ASSERT( (nEnd < m_nLength) || (nEnd == NOSECTION_VAL) );
	ASSERT( (nBegin < m_nLength) || (nBegin == NOSECTION_VAL) );
	
	if( (m_nSelectionEnd != nEnd) || (m_nSelectionBegin != nBegin) ) {
		if( (nEnd >= m_nLength) && (nEnd != NOSECTION_VAL) ) {
			nEnd = m_nLength;
		}
		if( (nBegin >= m_nLength) && (nBegin != NOSECTION_VAL) ) {
			nBegin = m_nLength;
		}
		m_nSelectionEnd = nEnd;
		m_nSelectionBegin = nBegin;
		if(bMakeVisible && nEnd != NOSECTION_VAL && nBegin != NOSECTION_VAL) {
			MakeVisible(m_nSelectionBegin, m_nSelectionEnd, false);
		}
		if(bUpdate && ::IsWindow(m_hWnd)) {
			Invalidate();
		}
	}
}


void CHexEditCtrl::OnEditBackspace()
{
	if (!m_nLength)
		return;
	
	UINT DeleteBegin = NOSECTION_VAL;
	UINT DeleteLength = NOSECTION_VAL;

	if (m_nSelectionBegin != NOSECTION_VAL&&
		m_nSelectionEnd != NOSECTION_VAL){
		//delete range.
		if (m_nSelectionEnd > m_nSelectionBegin){
			DeleteBegin = m_nSelectionBegin;
			DeleteLength = m_nSelectionEnd - m_nSelectionBegin;
		}
	}
	else if (m_nCurrentAddress){
		DeleteBegin = m_nCurrentAddress - 1;
		DeleteLength = 0x1;
	}

	if (DeleteBegin == NOSECTION_VAL ||
		DeleteLength == NOSECTION_VAL)
		return;

	memmove(m_pData + DeleteBegin,
		m_pData + DeleteBegin + DeleteLength,
		m_nLength - (DeleteBegin + DeleteLength));

	m_nCurrentAddress = DeleteBegin;
	m_nLength -= DeleteLength;

	SetEditCaretPos(m_nCurrentAddress, 1);

	m_bRecalc = TRUE;


	m_nSelectingBeg = NOSECTION_VAL;
	m_nSelectingEnd = NOSECTION_VAL;
	m_nSelectionBegin = NOSECTION_VAL;
	m_nSelectionEnd = NOSECTION_VAL;

	Invalidate();
}