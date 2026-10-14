; ============================================================
; RO-MP Setup — installer ala SA-MP (NSIS).
; Menginstall romp.exe (launcher + server browser) dan romp.dll
; (client mod yang di-inject ke GTA:SA).
; Build: makensis romp-setup.nsi  (butuh NSIS 3.x)
; ============================================================

!include "MUI2.nsh"
!include "LogicLib.nsh"

Name "RO-MP"
OutFile "romp-setup.exe"
InstallDir "$PROGRAMFILES\RO-MP"
RequestExecutionLevel admin

; --- Halaman ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
Page custom GtaDirPage GtaDirLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; Variabel folder GTA
Var GtaDir

; --- Cari GTA:SA otomatis ---
Function FindGta
    ; registry Rockstar
    ReadRegStr $GtaDir HKLM "SOFTWARE\Rockstar Games\GTA San Andreas" "InstallFolder"
    ${If} $GtaDir != ""
        IfFileExists "$GtaDir\gta_sa.exe" done
    ${EndIf}
    ReadRegStr $GtaDir HKLM "SOFTWARE\WOW6432Node\Rockstar Games\GTA San Andreas" "InstallFolder"
    ${If} $GtaDir != ""
        IfFileExists "$GtaDir\gta_sa.exe" done
    ${EndIf}
    ; path umum (unrolled loop)
    StrCpy $GtaDir "C:\Program Files\Rockstar Games\GTA San Andreas"
    IfFileExists "$GtaDir\gta_sa.exe" done
    StrCpy $GtaDir "C:\Program Files (x86)\Rockstar Games\GTA San Andreas"
    IfFileExists "$GtaDir\gta_sa.exe" done
    StrCpy $GtaDir "D:\Program Files\Rockstar Games\GTA San Andreas"
    IfFileExists "$GtaDir\gta_sa.exe" done
    StrCpy $GtaDir "D:\Games\GTA San Andreas"
    IfFileExists "$GtaDir\gta_sa.exe" done
    StrCpy $GtaDir ""
    done:
FunctionEnd

; --- Halaman pilih folder GTA ---
Function GtaDirPage
    Call FindGta
    !insertmacro MUI_HEADER_TEXT "Lokasi GTA: San Andreas" "Pilih folder instalasi GTA:SA (v1.0 US)."
    nsDialogs::Create 1018
    Pop $0
    ${If} $0 == error
        Abort
    ${EndIf}
    ${NSD_CreateLabel} 0 0 100% 24u "Folder GTA: San Andreas (berisi gta_sa.exe):"
    Pop $0
    ${NSD_CreateDirRequest} 0 28u 100% 14u "$GtaDir"
    Pop $1
    ${NSD_CreateBrowseButton} 0 46u 30% 14u "Browse..."
    Pop $2
    ${NSD_OnClick} $2 OnBrowseGta
    nsDialogs::Show
FunctionEnd

Function OnBrowseGta
    Pop $0
    nsDialogs::SelectFolderDialog "Pilih folder GTA: San Andreas" "$GtaDir"
    Pop $3
    ${If} $3 != error
        ${NSD_SetText} $1 "$3"
    ${EndIf}
FunctionEnd

Function GtaDirLeave
    ${NSD_GetText} $1 $GtaDir
    IfFileExists "$GtaDir\gta_sa.exe" ok
        MessageBox MB_ICONEXCLAMATION "gta_sa.exe tidak ditemukan di folder itu.$\nPilih folder GTA: San Andreas yang benar."
        Abort
    ok:
    ; simpan untuk dipakai launcher
    WriteINIStr "$INSTDIR\romp.ini" "game" "folder" "$GtaDir"
FunctionEnd

; --- Instalasi ---
Section "RO-MP" SecMain
    SetOutPath "$INSTDIR"
    File "..\build\Release\romp.exe"
    File "..\mod\build\out\Release\romp.dll"

    ; tulis folder GTA ke config launcher juga (%APPDATA% ditulis saat run,
    ; tapi sediakan default di sini)
    ; (launcher membaca romp.ini dulu bila %APPDATA% kosong)

    ; shortcut
    CreateDirectory "$SMPROGRAMS\RO-MP"
    CreateShortcut "$SMPROGRAMS\RO-MP\RO-MP.lnk" "$INSTDIR\romp.exe"
    CreateShortcut "$SMPROGRAMS\RO-MP\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortcut "$DESKTOP\RO-MP.lnk" "$INSTDIR\romp.exe"

    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; --- Uninstaller ---
Section "Uninstall"
    Delete "$INSTDIR\romp.exe"
    Delete "$INSTDIR\romp.dll"
    Delete "$INSTDIR\romp.ini"
    Delete "$INSTDIR\uninstall.exe"
    Delete "$SMPROGRAMS\RO-MP\RO-MP.lnk"
    Delete "$SMPROGRAMS\RO-MP\Uninstall.lnk"
    Delete "$DESKTOP\RO-MP.lnk"
    RMDir "$SMPROGRAMS\RO-MP"
    RMDir "$INSTDIR"
SectionEnd
