; NSIS Modern User Interface installer for GNOME Web (Epiphany for Windows)
Unicode true
!include "MUI2.nsh"

!define APP_NAME "GNOME Web"
!define APP_EXE "gnome-web.exe"
!ifndef VERSION
  !define VERSION "49.1.0"
!endif

Name "${APP_NAME}"
OutFile "..\dist\GNOME-Web-Setup-${VERSION}.exe"
InstallDir "$LOCALAPPDATA\Programs\GNOME Web"
InstallDirRegKey HKCU "Software\GNOME Web" "InstallDir"
RequestExecutionLevel user

; Interface Settings
!define MUI_ABORTWARNING

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Start GNOME Web now"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"

Section "MainSection" SecMain
  SetOutPath "$INSTDIR"
  File /r "..\dist\app\*.*"

  ; Shortcuts
  CreateDirectory "$SMPROGRAMS\GNOME Web"
  CreateShortcut "$SMPROGRAMS\GNOME Web\GNOME Web.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortcut "$SMPROGRAMS\GNOME Web\Uninstall GNOME Web.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\GNOME Web.lnk" "$INSTDIR\${APP_EXE}"

  ; Write Uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Registry for Add/Remove Programs
  WriteRegStr HKCU "Software\GNOME Web" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web" "DisplayName" "GNOME Web"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web" "Publisher" "The GNOME Project"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web" "DisplayIcon" "$INSTDIR\${APP_EXE},0"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web" "NoRepair" 1
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\GNOME Web.lnk"
  Delete "$SMPROGRAMS\GNOME Web\GNOME Web.lnk"
  Delete "$SMPROGRAMS\GNOME Web\Uninstall GNOME Web.lnk"
  RMDir "$SMPROGRAMS\GNOME Web"

  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\GNOME Web"
  DeleteRegKey HKCU "Software\GNOME Web"
SectionEnd
