Codejock Xtreme ToolkitPro (XTP) is required to build Client.

Option A - Install to default location:
  C:\Program Files (x86)\Codejock Software\MFC\Xtreme ToolkitPro v18.5.0

Option B - Copy/symlink XTP into this folder:
  worldgh0st\ThirdParty\Xtreme ToolkitPro v18.5.0\
    Source\XTToolkitPro.h
    Lib\vc120\*.lib

Option C - Set environment variable before opening VS2013:
  XTPROOT = <XTP root directory>

Option D - Local props file (recommended):
  1. Copy ..\XTP.local.props.example to ..\XTP.local.props
  2. Edit XtpRoot to your install path
  3. Rebuild Client

After installing XTP for VS2013 (v120), run XTP Command Line Wizard to build Lib\vc120 if missing.
