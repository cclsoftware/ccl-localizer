;-----------------------------------------------------------------------
; CCL Localizer Installer Script
;-----------------------------------------------------------------------

; Properly display all languages
Unicode true

; DPI-awareness option requires NSIS 3.x!
ManifestDPIAware true

;-----------------------------------------------------------------------
; Build Locations
;-----------------------------------------------------------------------

!ifndef BASEDIR
!define BASEDIR "..\..\..\..\..\.."
!endif

!ifndef CCL_FRAMEWORK_DIR
!define CCL_FRAMEWORK_DIR "${BASEDIR}\framework"
!endif

!define EULADIR "${CCL_IDENTITIES_DIR}\ccl\eula"
!define LEGALDIR "${CCL_IDENTITIES_DIR}\ccl\legal"

;-----------------------------------------------------------------------
; Includes
;-----------------------------------------------------------------------

!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"

!include "shared.nsh"

!addplugindir "${CCL_BASEDIR}\submodules\nsis-plugins\ccltools\bin\"

;-----------------------------------------------------------------------
; Product Settings
;-----------------------------------------------------------------------

# Note: X64 has to be defined via command line: "makensis /dX64 localizerapp.nsi"

!define PRODUCT  "CCL Localizer"
!define EXENAME	"${PRODUCT}.exe"

Name "${PRODUCT}"
BrandingText " "

!ifdef X64
  OutFile		"${PRODUCT} Installer.exe"
  InstallDir	"$PROGRAMFILES64\${PRODUCT}"
!else
!ifdef ARM64
  OutFile		"${PRODUCT} Installer Arm64.exe"
  InstallDir	"$PROGRAMFILES64\${PRODUCT}"
!else
  OutFile		"${PRODUCT} Installer x86.exe"
  InstallDir	"$PROGRAMFILES\${PRODUCT}"
!endif
!endif

;-----------------------------------------------------------------------
; Version Information
;-----------------------------------------------------------------------

!searchparse /file ../../../source/appversion.h '#define VER_MAJOR			' VER_MAJOR
!searchparse /file ../../../source/appversion.h '#define VER_MINOR			' VER_MINOR
!searchparse /file ../../../source/appversion.h '#define VER_REVISION		' VER_REVISION
!define PRODUCT_VERSION	"${VER_MAJOR}.${VER_MINOR}.${VER_REVISION}"

# search buildnumber.h for repository revision
!searchparse /file "${BASEDIR}\buildnumber.h" '#define BUILD_REVISION_STRING		"' BUILD_REVISION '"'

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey  "CompanyName" "${COMPANY}"
VIAddVersionKey  "FileDescription" "${PRODUCT} Installer"
VIAddVersionKey  "FileVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"
VIAddVersionKey  "InternalName" "${VENDOR_PACKAGE_DOMAIN}.localizerapp"
VIAddVersionKey  "LegalCopyright" "${COPYRIGHT}"
VIAddVersionKey  "ProductName" "${PRODUCT}"
VIAddVersionKey  "ProductVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"

;-----------------------------------------------------------------------
; Definitions
;-----------------------------------------------------------------------

!define MUI_ICON   "..\resource\localizer.ico"
!define MUI_UNICON "..\resource\localizer.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP "resource\wizard@2x.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "resource\wizard@2x.bmp"

;!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_BITMAP "resource\header.bmp"
;!define MUI_HEADERIMAGE_UNBITMAP "resource\header.bmp"

!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
!define MUI_LANGDLL_REGISTRY_KEY "Software\${PRODUCT}" 
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp

;-----------------------------------------------------------------------
; Pages
;-----------------------------------------------------------------------

!insertmacro MUI_PAGE_WELCOME_CUSTOMIZED
!insertmacro MUI_PAGE_LICENSE_CUSTOMIZED $(licenseFileName)

!insertmacro MUI_PAGE_DIRECTORY_CUSTOMIZED
!insertmacro MUI_PAGE_INSTFILES_CUSTOMIZED
!insertmacro MUI_PAGE_FINISH_CUSTOMIZED

; Uninstaller
!insertmacro MUI_UNPAGE_CONFIRM_CUSTOMIZED
!insertmacro MUI_UNPAGE_INSTFILES_CUSTOMIZED

;-----------------------------------------------------------------------
; Languages
;-----------------------------------------------------------------------

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Spanish"

LicenseLangString licenseFileName ${LANG_ENGLISH} "${EULADIR}\EULA.txt"
LicenseLangString licenseFileName ${LANG_GERMAN} "${EULADIR}\EULA.txt" ; EULA-de.txt
LicenseLangString licenseFileName ${LANG_SIMPCHINESE} "${EULADIR}\EULA.txt" ; EULA-zh.txt
LicenseLangString licenseFileName ${LANG_JAPANESE} "${EULADIR}\EULA.txt" ; EULA-ja.txt
LicenseLangString licenseFileName ${LANG_FRENCH} "${EULADIR}\EULA.txt" ; EULA-fr.txt
LicenseLangString licenseFileName ${LANG_SPANISH} "${EULADIR}\EULA.txt" ; EULA-es.txt

; Shared Strings and font definitions
!include "${NSIS_INCLUDES_DIR}\localize.nsh"

;-----------------------------------------------------------------------
; Variables
;-----------------------------------------------------------------------

Var RunAppOnExit

;-----------------------------------------------------------------------
; Functions
;-----------------------------------------------------------------------

Function .onInit

	StrCpy $RunAppOnExit 0

	; Check for supported Windows version
	Call CheckWindowsVersion

	; Check for 64 bit version and switch registry view
	Call Check64BitVersion

	; Language selection dialog
	!insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

Function LaunchApp
	StrCpy $RunAppOnExit 1
FunctionEnd

Function .onGUIEnd
	${if} $RunAppOnExit != 0
		ccltools::runFromExplorer "$INSTDIR\${EXENAME}"
		pop $0
	${EndIf}
FunctionEnd

;-----------------------------------------------------------------------
; Installer Sections
;-----------------------------------------------------------------------

Section "-All"

  SetOverwrite ifnewer

  ;-----------------------------------------------------------------------
  ; Application Files
  ;-----------------------------------------------------------------------
  
  SetOutPath "$INSTDIR"
  Delete "$INSTDIR\*.dll" ; remove old EXE and DLL files
  Delete "$INSTDIR\*.exe" 

  File "${BUILDDIR}\${EXENAME}"

  RMDir /r "$INSTDIR\license" ; cleanup old license files

  SetOutPath "$INSTDIR\license"
  File "${LEGALDIR}\*.txt"
  File "${EULADIR}\*.txt"

  ;-----------------------------------------------------------------------
  ; Help files
  ;-----------------------------------------------------------------------
  
  RMDir /r "$INSTDIR\help" ; cleanup old help files

  SetOutPath "$INSTDIR\help"
  File /r "..\..\..\assets\help\*.*"
  
  ;-----------------------------------------------------------------------
  ; Framework Files
  ;-----------------------------------------------------------------------

  SetOutPath "$INSTDIR"
  File "${BUILDDIR}\ccltext.dll"
  File "${BUILDDIR}\cclsystem.dll"
  File "${BUILDDIR}\cclgui.dll"
 
  ;-----------------------------------------------------------------------
  ; CRT
  ;-----------------------------------------------------------------------
  
  !include /NONFATAL "${CCL_BASEDIR}\submodules\vcredist\crt.nsh"

  ;-----------------------------------------------------------------------
  ; gettext Files
  ;-----------------------------------------------------------------------

  RMDir /r "$INSTDIR\gettext" ; cleanup old gettext folder

  SetOutPath "$INSTDIR\gettext"
  File /NONFATAL /r "${CCL_BASEDIR}\tools\bin\win\gettext\*.*"

  ;-----------------------------------------------------------------------
  ; PlugIns
  ;-----------------------------------------------------------------------

  Delete "$INSTDIR\Plugins\*.dll" ; cleanup plug-in folder

  SetOutPath "$INSTDIR\Plugins" ; create empty plug-ins folder to avoid error message on startup

  ;-----------------------------------------------------------------------
  ; Language Packs
  ;-----------------------------------------------------------------------

  RMDir /r "$INSTDIR\languages" ; cleanup old language packs

  SetOutPath "$INSTDIR\languages"
  File /r "..\..\..\translations\projects\localizerapp\languages\*.langpack"

  ;-----------------------------------------------------------------------
  ; Define Language
  ;-----------------------------------------------------------------------
	
  StrCmp $LANGUAGE ${LANG_ENGLISH} 0 +2
		WriteRegStr HKCU "${CCL_LOCALE_KEY}" "$INSTDIR\${EXENAME}" ""
  StrCmp $LANGUAGE ${LANG_GERMAN} 0 +2
		WriteRegStr HKCU "${CCL_LOCALE_KEY}" "$INSTDIR\${EXENAME}" "$INSTDIR\languages\German.langpack"
  StrCmp $LANGUAGE ${LANG_JAPANESE} 0 +2
		WriteRegStr HKCU "${CCL_LOCALE_KEY}" "$INSTDIR\${EXENAME}" "$INSTDIR\languages\Japanese.langpack"
  StrCmp $LANGUAGE ${LANG_SIMPCHINESE} 0 +2
		WriteRegStr HKCU "${CCL_LOCALE_KEY}" "$INSTDIR\${EXENAME}" "$INSTDIR\languages\Chinese.langpack"
  StrCmp $LANGUAGE ${LANG_FRENCH} 0 +2
		WriteRegStr HKCU "${CCL_LOCALE_KEY}" "$INSTDIR\${EXENAME}" "$INSTDIR\languages\French.langpack"
  StrCmp $LANGUAGE ${LANG_SPANISH} 0 +2
		WriteRegStr HKCU "${CCL_LOCALE_KEY}" "$INSTDIR\${EXENAME}" "$INSTDIR\languages\Spanish.langpack"

  ;-----------------------------------------------------------------------
  ; Uninstaller
  ;-----------------------------------------------------------------------
 
  SetOutPath "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;-----------------------------------------------------------------------
  ; Shortcuts
  ;-----------------------------------------------------------------------

  !ifdef WIN64
    CreateShortCut "$SMPROGRAMS\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}"
    CreateShortCut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}"
  !else
    CreateShortCut "$SMPROGRAMS\${PRODUCT} x86.lnk" "$INSTDIR\${EXENAME}"
    CreateShortCut "$DESKTOP\${PRODUCT} x86.lnk" "$INSTDIR\${EXENAME}"
  !endif

  ;-----------------------------------------------------------------------
  ; File Association
  ;-----------------------------------------------------------------------
  
  !insertmacro AddFileAssociationAndCommand "lzproject" "LocalizationProject" "Localization Project" 2

  ;-----------------------------------------------------------------------
  ; Register Uninstaller
  ;-----------------------------------------------------------------------

  WriteRegStr HKLM "Software\${PRODUCT}" "" $INSTDIR
  
  !ifdef WIN64
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayName" "${PRODUCT}"
  !else
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayName" "${PRODUCT} x86"
  !endif

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayIcon" "$INSTDIR\${EXENAME},-1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoRepair" "1"
  
SectionEnd

;-----------------------------------------------------------------------
; Uninstaller
;-----------------------------------------------------------------------

Section "Uninstall"

  ; User Settings
  RMDir /r "$APPDATA\${PRODUCT}"

  RMDir /r "$INSTDIR\license"
  RMDir /r "$INSTDIR\languages"
  RMDir /r "$INSTDIR\gettext"
  RMDir /r "$INSTDIR\help"
  RMDir /r "$INSTDIR\Plugins"

  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.dll"

  RMDir $INSTDIR ; should be empty now
    
  !ifdef WIN64
    Delete "$DESKTOP\${PRODUCT}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT}.lnk"
  !else
    Delete "$DESKTOP\${PRODUCT} x86.lnk"
    Delete "$SMPROGRAMS\${PRODUCT} x86.lnk"
  !endif
  
  DeleteRegKey HKCR ".lzproject"
  DeleteRegKey HKCR "LocalizationProject"
  
  DeleteRegKey HKCU "Software\${PRODUCT}"
  DeleteRegKey HKLM "Software\${PRODUCT}"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"
 
SectionEnd

;-----------------------------------------------------------------------
; Uninstaller Functions
;-----------------------------------------------------------------------

Function un.onInit

  !ifdef WIN64
    ; switch to 64 bit registry
    SetRegView 64
  !endif

  ; Language selection dialog
  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd
