; Script based on the facebook-chat nsi file
; Updated to support PortablePidgin

SetCompress auto

; todo: SetBrandingImage
; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "expand"
!define PRODUCT_PUBLISHER "Mike Miller <mikeage@gmail.com>"
!define PRODUCT_WEB_SITE "https://code.google.com/p/expand/"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!define BASEDIR "."

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "${BASEDIR}\COPYING"
!define MUI_PAGE_CUSTOMFUNCTION_PRE GetPidginInstPath
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
;!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PACKAGE_VERSION}"
OutFile "${PRODUCT_NAME}-${PACKAGE_VERSION}.exe"

ShowInstDetails show
ShowUnInstDetails show



Section "${PRODUCT_NAME} core" SEC01
SectionIn RO
    SetOverwrite try
    
	copy:
		ClearErrors
# Original names
		Delete "$INSTDIR\plugins\expand.dll"
		Delete "$INSTDIR\plugins\expand.dll.dbgsym"
		IfErrors dllbusy
		SetOutPath "$INSTDIR\plugins"
		File "${BASEDIR}\expand.dll"
		Goto after_copy
	dllbusy:
		MessageBox MB_RETRYCANCEL "expand.dll is busy. Please close Pidgin (including tray icon) and try again" IDCANCEL cancel
		Goto copy
	cancel:
		Abort "Installation of Expand aborted"
	after_copy:
SectionEnd

Section "${PRODUCT_NAME} dbgsym" SEC02
		SetOutPath "$INSTDIR\plugins"
		File "${BASEDIR}\expand.dll.dbgsym"
SectionEnd

Section "${PRODUCT_NAME} localizations" SEC03
	!include po_expand.nsi
SectionEnd

Function GetPidginInstPath
  Push $0
  ReadRegStr $0 HKLM "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
	ReadRegStr $0 HKCU "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
	StrCpy $0 "C:\PortableApps\PortableApps\PidginPortable\App\Pidgin"	
	IfFileExists "$0\pidgin-portable.exe" cont
	StrCpy $0 "C:\PortableApps\PidginPortable\App\Pidgin"	
	IfFileExists "$0\pidgin-portable.exe" cont
	StrCpy $0 "$PROGRAMFILES\PortableApps\PidginPortable\App\Pidgin"	
	IfFileExists "$0\pidgin-portable.exe" cont
	StrCpy $0 "$PROGRAMFILES\PortableApps\PortableApps\PidginPortable\App\Pidgin"	
	IfFileExists "$0\pidgin-portable.exe" cont
	MessageBox MB_OK|MB_ICONINFORMATION "Failed to automatically find a Pidgin installation. Please select your Pidgin directory directly [if you are using PortablePidgin, please select the App\Pidgin subdirectory]"
	Goto done
  cont:
	StrCpy $INSTDIR $0
  done:
FunctionEnd
