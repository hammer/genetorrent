; GeneTorrent.nsi
;
; This is a Nullsoft Scriptable Install System script that generates
; an installer for the Windows/cygwin version of GeneTorrent.

;--------------------------------

; The name of the installer
Name "GeneTorrent 3.8.1"

; The file to write
OutFile "Install-win-GeneTorrent-3.8.1.exe"

; The default installation directory
InstallDir $PROGRAMFILES\GeneTorrent

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\GeneTorrent" "Install_Dir"

; Request application privileges for Windows Vista or later
RequestExecutionLevel highest

InstType Default

;--------------------------------

; Pages

PageEx license
  LicenseData LICENSE
  LicenseForceSelection checkbox
PageExEnd
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The GeneTorrent binaries
Section "GeneTorrent (required)"

  SectionIn 1 RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; GeneTorrent docs
  File "src\gtdownload_manual.txt"

  ; GeneTorrent Files  
  File "src\.libs\gtdownload.exe"
  File "src\dhparam.pem"
  File "src\cacert.pem"
  File "libtorrent\src\.libs\cygtorrent-rasterbar-6.dll"
  File "src\.libs\cyggenetorrent-0.dll"

  ; Cygwin Files - GeneTorrent deps
  File "\cygwin\bin\cygboost_filesystem-mt-1_48.dll"
  File "\cygwin\bin\cygboost_program_options-mt-1_48.dll"
  File "\cygwin\bin\cygboost_regex-mt-1_48.dll"
  File "\cygwin\bin\cygboost_system-mt-1_48.dll"
  File "\cygwin\bin\cygssl-1.0.0.dll"
  File "\cygwin\bin\cygxerces-c-3-0.dll"
  File "\cygwin\bin\cygxqilla-6.dll"
  File "\cygwin\bin\cygcrypto-1.0.0.dll"
  File "\cygwin\bin\cygcurl-4.dll"

  ; Cygwin Files
  File "\cygwin\bin\cygasn1-8.dll"
  File "\cygwin\bin\cygcom_err-2.dll"
  File "\cygwin\bin\cygcrypt-0.dll"
  File "\cygwin\bin\cyggcc_s-1.dll"
  File "\cygwin\bin\cyggssapi-3.dll"
  File "\cygwin\bin\cygheimbase-1.dll"
  File "\cygwin\bin\cygheimntlm-0.dll"
  File "\cygwin\bin\cyghx509-5.dll"
  File "\cygwin\bin\cygiconv-2.dll"
  File "\cygwin\bin\cygicudata38.dll"
  File "\cygwin\bin\cygicudata48.dll"
  File "\cygwin\bin\cygicui18n48.dll"
  File "\cygwin\bin\cygicuuc38.dll"
  File "\cygwin\bin\cygicuuc48.dll"
  File "\cygwin\bin\cygidn-11.dll"
  File "\cygwin\bin\cygintl-8.dll"
  File "\cygwin\bin\cygkrb5-26.dll"
  File "\cygwin\bin\cyglber-2-4-2.dll"
  File "\cygwin\bin\cygldap-2-4-2.dll"
  File "\cygwin\bin\cygroken-18.dll"
  File "\cygwin\bin\cygsasl2-2.dll"
  File "\cygwin\bin\cygsqlite3-0.dll"
  File "\cygwin\bin\cygssh2-1.dll"
  File "\cygwin\bin\cygstdc++-6.dll"
  File "\cygwin\bin\cygwin1.dll"
  File "\cygwin\bin\cygwind-0.dll"
  File "\cygwin\bin\cygz.dll"


  ;For cgquery
  ;PyInstaller 2.0 needs to be run first outside of cygwin on cgquery FIRST
  File "dist\cgquery.exe"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\GeneTorrent "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GeneTorrent" "DisplayName" "GeneTorrent"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GeneTorrent" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GeneTorrent" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GeneTorrent" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  SectionIn 1

  CreateDirectory "$SMPROGRAMS\GeneTorrent"
  CreateShortCut "$SMPROGRAMS\GeneTorrent\GeneTorrent Shell.lnk" "cmd.exe" \
   "/k $\"@set CYGWIN=nodosfilewarning&&@set PATH=%PATH%;$INSTDIR&&@echo Type cgquery or gtdownload to begin.  &&@cd %HOMEPATH%$\"" \
   "cmd.exe" 0
  CreateShortCut "$SMPROGRAMS\GeneTorrent\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GeneTorrent"
  DeleteRegKey HKLM SOFTWARE\GeneTorrent

  ; Remove files and uninstaller
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\gt*.exe
  Delete $INSTDIR\cgquery.exe
  Delete $INSTDIR\cyg*.dll
  Delete $INSTDIR\gto*
  Delete $INSTDIR\*.pem
  Delete $INSTDIR\*.txt
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\GeneTorrent\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\GeneTorrent"
  RMDir "$INSTDIR"

SectionEnd
