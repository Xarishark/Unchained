; The Windows installer: one exe holding the release folder release.bat lays out, which is the program and what it needs
; to build the game code on the player's PC. No game files or code. release.bat builds it when NSIS is installed:
;   makensis /DVERSION=<v> /DDIST=<release folder> /DART=<art folder> /DOUTFILE=<exe> tools\installer\unchained.nsi
; C:\ by default, not Program Files: saves, settings and caches go next to the program, and Program Files doesn't allow
; that. Its look follows the Configuration app's: the same colours, art drawn by tools/installer/art.py. On a Windows too
; old for the dark controls they stay plain, and it all still works.
Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user
ManifestDPIAware true

!ifndef VERSION
  !error "VERSION: release.bat gives it"
!endif
!define NAME "Unchained"
!define UNINST "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unchained"
!define BG "051618"
!define TXT "D9EFE5"

Name "${NAME} ${VERSION}"
OutFile "${OUTFILE}"
InstallDir "C:\Unchained"
InstallDirRegKey HKCU "${UNINST}" "InstallLocation"
BrandingText " "
XPStyle on

!include "MUI2.nsh"
!define MUI_ICON "${ART}\unchained.ico"
!define MUI_UNICON "${ART}\unchained.ico"
!define MUI_ABORTWARNING
!define MUI_BGCOLOR "${BG}"
!define MUI_TEXTCOLOR "${TXT}"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${ART}\installer-side.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${ART}\installer-side.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "${ART}\installer-header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${ART}\installer-header.bmp"
!define MUI_INSTFILESPAGE_COLORS "${TXT} ${BG}"
!define MUI_CUSTOMFUNCTION_GUIINIT DarkFrame
!define MUI_CUSTOMFUNCTION_UNGUIINIT un.DarkFrame
!define MUI_WELCOMEPAGE_TITLE "Unchained ${VERSION}"
!define MUI_WELCOMEPAGE_TEXT "An unofficial PC port of Whiplash for the original Xbox. This installs the program only: it contains no game files or game code.$\r$\n$\r$\nTo play, you need your own copy of Whiplash (USA) for the original Xbox: its disc image (.iso) or its extracted files. After installing, Unchained Configuration asks for it, copies the files and builds the game's code once on this PC.$\r$\n$\r$\nThis needs about 200 MB here, plus about 2 GB for the game files and the built code."
!define MUI_DIRECTORYPAGE_TEXT_TOP "Saves, settings and the game files are kept in this folder, so pick one you can write to. Program Files won't work."
!define MUI_FINISHPAGE_TITLE "Installed"
!define MUI_FINISHPAGE_TEXT "Next, Unchained Configuration asks where your copy of the game is, sets it up, and starts the game."
!define MUI_FINISHPAGE_RUN "$INSTDIR\Unchained Configuration.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start Unchained Configuration and set up your game files"
!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Add a shortcut to the desktop"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION DesktopShortcut

!define MUI_PAGE_CUSTOMFUNCTION_SHOW DarkInner
!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_CUSTOMFUNCTION_SHOW DarkInner
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_PAGE_CUSTOMFUNCTION_SHOW DarkInstFiles
!insertmacro MUI_PAGE_INSTFILES
!define MUI_PAGE_CUSTOMFUNCTION_SHOW DarkFinish
!insertmacro MUI_PAGE_FINISH
!define MUI_PAGE_CUSTOMFUNCTION_SHOW un.DarkInner
!insertmacro MUI_UNPAGE_CONFIRM
!define MUI_PAGE_CUSTOMFUNCTION_SHOW un.DarkInstFiles
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; The colours of the window around the pages (dark title bar: Windows 10 2004 on; its colour: Windows 11; dark buttons),
; and of everything inside a page, one control at a time.
!macro DarkFunctions un
Function ${un}DarkButton
  Pop $R1
  System::Call 'uxtheme::SetWindowTheme(p R1, w "DarkMode_Explorer", p 0)'
FunctionEnd

Function ${un}DarkFrame
  System::Call 'dwmapi::DwmSetWindowAttribute(p $HWNDPARENT, i 20, *i 1, i 4)'
  System::Call 'dwmapi::DwmSetWindowAttribute(p $HWNDPARENT, i 35, *i 0x00181605, i 4)'
  SetCtlColors $HWNDPARENT ${TXT} ${BG}
  StrCpy $R0 0
  loop:
    FindWindow $R0 "" "" $HWNDPARENT $R0
    StrCmp $R0 0 done
    System::Call 'user32::GetClassName(p R0, t .R2, i 64)'
    StrCmp $R2 "Button" 0 +3
      Push $R0
      Call ${un}DarkButton
    StrCmp $R2 "Static" 0 loop
      SetCtlColors $R0 ${TXT} ${BG}
    Goto loop
  done:
FunctionEnd

; (classic look on check boxes and the group box: a themed one ignores the colours)
Function ${un}DarkInner
  FindWindow $R3 "#32770" "" $HWNDPARENT
  SetCtlColors $R3 ${TXT} ${BG}
  StrCpy $R0 0
  loop:
    FindWindow $R0 "" "" $R3 $R0
    StrCmp $R0 0 done
    System::Call 'user32::GetClassName(p R0, t .R2, i 64)'
    StrCmp $R2 "Button" 0 +2
      System::Call 'uxtheme::SetWindowTheme(p R0, w " ", w " ")'
    StrCmp $R2 "Edit" 0 +2
      System::Call 'uxtheme::SetWindowTheme(p R0, w "DarkMode_CFD", p 0)'
    SetCtlColors $R0 ${TXT} ${BG}
    Goto loop
  done:
  GetDlgItem $R0 $R3 1001  ; (the directory page's Browse)
  StrCmp $R0 0 +3
    Push $R0
    Call ${un}DarkButton
FunctionEnd

; (the bar in the green, on the page's teal)
Function ${un}DarkInstFiles
  Call ${un}DarkInner
  GetDlgItem $R0 $R3 1027  ; (Show details, a dark button as the others)
  StrCmp $R0 0 +3
    Push $R0
    Call ${un}DarkButton
  GetDlgItem $R0 $R3 1004
  System::Call 'uxtheme::SetWindowTheme(p R0, w " ", w " ")'
  SendMessage $R0 0x0409 0 0x008AF239
  SendMessage $R0 0x2001 0 0x00282609
FunctionEnd
!macroend
!insertmacro DarkFunctions ""
!insertmacro DarkFunctions "un."

Function DarkFinish
  Call DarkInner
  SetCtlColors $mui.FinishPage.Run ${BG} 39F28A
  SetCtlColors $mui.FinishPage.ShowReadme ${BG} 39F28A
FunctionEnd

Function DesktopShortcut
  CreateShortcut "$DESKTOP\${NAME}.lnk" "$INSTDIR\Unchained.exe"
FunctionEnd

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${DIST}\*.*"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\${NAME}"
  CreateShortcut "$SMPROGRAMS\${NAME}\${NAME}.lnk" "$INSTDIR\Unchained.exe"
  CreateShortcut "$SMPROGRAMS\${NAME}\${NAME} Configuration.lnk" "$INSTDIR\Unchained Configuration.exe"
  CreateShortcut "$SMPROGRAMS\${NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "${UNINST}" "DisplayName" "${NAME}"
  WriteRegStr HKCU "${UNINST}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINST}" "DisplayIcon" "$INSTDIR\Unchained.exe"
  WriteRegStr HKCU "${UNINST}" "Publisher" "devall96"
  WriteRegStr HKCU "${UNINST}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINST}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKCU "${UNINST}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST}" "NoRepair" 1
SectionEnd

; Takes out what this installed and what it built, by name, never the whole folder: it may be one the player chose
; with other things in it. Saves, settings and the game files stay, and the message says so.
Section "Uninstall"
  RMDir /r "$INSTDIR\compiler"
  RMDir /r "$INSTDIR\cache"
  RMDir /r "$INSTDIR\fonts"
  Delete "$INSTDIR\Unchained.exe"
  Delete "$INSTDIR\Unchained Configuration.exe"
  Delete "$INSTDIR\Unchained.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\splash.png"
  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\THIRD_PARTY.txt"
  Delete "$INSTDIR\assets\README.txt"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR\assets"
  RMDir "$INSTDIR"
  RMDir /r "$SMPROGRAMS\${NAME}"
  Delete "$DESKTOP\${NAME}.lnk"
  DeleteRegKey HKCU "${UNINST}"
  IfFileExists "$INSTDIR\*.*" 0 +2
    MessageBox MB_OK "Your saves, settings and game files were kept in $INSTDIR. Delete that folder yourself if you don't want them." /SD IDOK
SectionEnd
