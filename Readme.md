# Introduction 

This is a simple MFC HexEdit Ctrl,support editing operations on smaller data.


# Usage

1. define HexEditCtrl object

```c
CHexEditCtrl m_HexEdit;
```

2. bind it with the edit control
```
void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_HexEdit);
}
```

