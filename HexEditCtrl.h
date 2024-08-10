///////////////////////////////////////////////////////////////////////////
#ifndef __hexeditbase_h
#define __hexeditbase_h
#if _MSC_VER > 1000
#pragma once
#endif


/////////////////////////////////////////////////////////////////////////////
// defines
/////////////////////////////////////////////////////////////////////////////
#define NOSECTION_VAL				0xffffffff

// notification codes
#define HEN_CHANGE					EN_CHANGE	//the same as the EDIT (CEdit)


#define ADDRESS_SIZE  0x8
#define BYTES_PER_ROW 0x8

#define TEXT_COLOR			  0x0
#define BACKGROUND_COLOR 0xffffff



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// class CHexEditBase
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CHexEditCtrl : public CWnd
{
public:
	CHexEditCtrl();
	virtual ~CHexEditCtrl();

	void SetData(const BYTE *pData, UINT nLen, bool bUpdate = true);

	BYTE* GetData() const { return m_pData; };
	UINT GetDataSize() const { return m_nLength; }

	void SetSelection(UINT nBegin, UINT nEnd, bool bMakeVisible = true, bool bUpdate = true);
	void MakeVisible(UINT nBegin, UINT nEnd, bool bUpdate=true);
	
	bool GetSelection(UINT& nBegin, UINT& nEnd) const;
	bool IsSelection() const;
	

	BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu, LPVOID lpParam = NULL);
	BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL);	
	static void RegisterClass();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

protected:
	struct PAINTINGDETAILS {
		UINT nFullVisibleLines;					//可以完整看到的有几行.
		UINT nLastLineHeight;					
		UINT nVisibleLines;						//可以看到有几行
		UINT nLineHeight;						//一行的高度
		UINT nCharacterWidth;					//一个字符的宽度
		UINT nBytesPerRow;						//一行显示的字节数
		UINT nHexPos;							//hex 列的横坐标
		UINT nHexLen;							//hex 列的宽度,像素
		UINT nAsciiPos;
		UINT nAsciiLen;
		UINT nAddressPos;						//地址列的x坐标
		UINT nAddressLen;						//地址列的宽度(像素)
		CRect cPaintingRect;					//绘制区域的大小.
	};

	bool m_bSelfCleanup;

	PAINTINGDETAILS m_tPaintDetails;

	BYTE *m_pData;
	UINT  m_nLength;									//字节数
	UINT  m_BuffSize; 

	
	
	//正在选择中,实际鼠标开始和结束的位置
	UINT m_nSelectingBeg;							
	UINT m_nSelectingEnd;			//

	//normali之后的,就是保证begin < end
	UINT m_nSelectionBegin;
	UINT m_nSelectionEnd;			//不包含end.

	UINT m_nCurrentAddress;
	UINT m_nCurCaretHeight; 
	
	UINT m_nScrollPostionY;	
	UINT m_nScrollRangeY;
	UINT m_nScrollPostionX;	
	UINT m_nScrollRangeX;
	UINT m_nCurCaretWidth;
	
	bool m_bRecalc;						//重新计算显示信息.
	bool m_bHasCaret;					//当前是否显示编辑光标.
	bool m_bHighBits;					//是当前字节的高位吗???

	bool m_bAddressIsWide;
	bool m_bShowCategory;

	//COLORREF m_tHighlightBkgCol;
	//COLORREF m_tHighlightTxtCol;
	///COLORREF m_tHighlightFrameCol;
	//COLORREF m_tNotUsedBkCol;
	
	COLORREF m_tSelectedTxtCol;			//在获取焦点情况下的文本颜色.
	COLORREF m_tSelectedBkgCol;			//在获取焦点的情况下的背景颜色

	CFont m_cFont;	
	CRect m_cDragRect;
	CPoint m_cMouseRepPoint;
	
	int m_iMouseRepDelta;
	WORD m_nMouseRepSpeed;
	WORD m_nMouseRepCounter;
	bool m_bIsMouseRepActive;

	// overrideables
	virtual void OnExtendContextMenu(CMenu&) {} // override this to add your own context-menue-items

	void NotifyParent(WORD wNBotifictionCode);			//通知父窗口...
	void CalculatePaintingDetails(CDC& cDC);
	void PaintAddresses(CDC& cDC);
	void PaintHexData(CDC& cDC);
	void PaintAsciiData(CDC& cDC);	

	void CreateEditCaret(UINT nCaretHeight, UINT nCaretWidth);
	void DestoyEditCaret();
	void SetEditCaretPos(UINT nOffset, bool bHighBits);
	bool OnEditInput(WORD nInput);

	void OnEditBackspace();
	void MoveCurrentAddress(int iDeltaAdr, bool bHighBits);
	void SetScrollPositionY(UINT nPosition, bool bUpdate=false);
	void SetScrollPositionX(UINT nPosition, bool bUpdate=false);
	void SetScrollbarRanges();
	void MoveScrollPostionY(int iDelta, bool bUpdate=false);
	void MoveScrollPostionX(int iDelta, bool bUpdate=false);
	void StartMouseRepeat(const CPoint& cPoint, int iDelta, WORD nSpeed);
	void StopMouseRepeat();

	void GetAddressFromPoint(const CPoint& cPt, UINT& nAddress, bool& bHighByte);

	//{{AFX_VIRTUAL(CHexEditBase)
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CHexEditBase)
	afx_msg void OnDestroy(); 
	afx_msg void OnTimer(UINT nTimerID);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint cPoint);
	afx_msg LRESULT OnWMChar(WPARAM wParam, LPARAM);
	afx_msg LRESULT OnWMSetFont(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWMGetFont(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUmSetScrollRange(WPARAM, LPARAM);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd*);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*);
	afx_msg UINT OnGetDlgCode();
	afx_msg BOOL OnEraseBkgnd(CDC*);
	afx_msg void OnLButtonDown(UINT, CPoint point);
	afx_msg void OnLButtonDblClk(UINT, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnKeyDown(UINT nChar, UINT, UINT);
	//}}AFX_MSG
	DECLARE_DYNCREATE(CHexEditCtrl)
	DECLARE_MESSAGE_MAP()
};




#endif
