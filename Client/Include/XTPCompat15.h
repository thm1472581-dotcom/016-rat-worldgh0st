#pragma once
// Compatibility shims: Client targets XTP 18.x APIs; build uses XTP 15.2.1.

#ifndef XTP_DPI_Y
#define XTP_DPI_Y(v) (v)
#define XTP_DPI_X(v) (v)
#define XTP_DPI(v) (v)
#define XTP_DPI_RECT(r) (r)
#endif

#ifndef xtpThemeOffice2013
#define xtpThemeOffice2013 xtpThemeRibbon
#endif

#ifndef xtpPaneThemeOffice2013
#define xtpPaneThemeOffice2013 xtpPaneThemeOffice2007Word
#endif

#ifndef xtpPaneThemeVisualStudio2015
#define xtpPaneThemeVisualStudio2015 xtpPaneThemeVisualStudio2010
#endif

#ifndef xtpControlThemeOffice2013
#define xtpControlThemeOffice2013 xtpControlThemeResource
#endif

#ifndef xtpControlThemeVisualStudio2015
#define xtpControlThemeVisualStudio2015 xtpControlThemeVisualStudio2010
#endif

#ifndef xtpIniOffice2010Blue
// XTP 15.2 Client.rc2 embeds Styles/Office2007/Office2007.rc only (not Office2010 DLLs).
#define xtpIniOffice2010Blue _T("OFFICE2007BLUE.INI")
#define xtpIniOffice2010Silver _T("OFFICE2007SILVER.INI")
#define xtpIniOffice2010Black _T("OFFICE2007BLACK.INI")
#define xtpIniOffice2013Word _T("OFFICE2007BLUE.INI")
#define xtpIniOffice2016WordColorful _T("OFFICE2007BLUE.INI")
#define xtpIniWindows7Blue _T("OFFICE2007BLUE.INI")
#endif

#ifndef XTPIniColor
inline COLORREF XTPIniColor(LPCTSTR, LPCTSTR, COLORREF clrDefault) { return clrDefault; }
#endif

#ifndef XTPThemeDLL
#define XTPThemeDLL() XTPResourceImages()
#endif
