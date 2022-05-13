@echo off

echo "---------- Downloading PopcornFX Runtime SDK for Unreal Engine (Desktop platforms) ----------"

setlocal

bitsadmin /reset
bitsadmin /create third_party_download_desktop
bitsadmin /addfile third_party_download_desktop https://downloads.popcornfx.com/Plugins/UE4/UnrealEngine_PopcornFXPlugin_2.12.3_Win64_Linux64_Mac64.7z "%~dp0\_PopcornFX_Runtime_SDK_Desktop.7z"
bitsadmin /setpriority third_party_download_desktop "FOREGROUND"
bitsadmin /resume third_party_download_desktop

:WAIT_FOR_DOWNLOAD_LOOP_START
    call bitsadmin /info third_party_download_desktop /verbose | find "STATE: TRANSFERRED"
    if %ERRORLEVEL% equ 0 goto WAIT_FOR_DOWNLOAD_LOOP_END
    call bitsadmin /RawReturn /GetBytesTransferred third_party_download_desktop
    printf ' / '
    call bitsadmin /RawReturn /GetBytesTotal third_party_download_desktop
    echo Bytes transferred
    timeout 2 > nul
    goto WAIT_FOR_DOWNLOAD_LOOP_START
:WAIT_FOR_DOWNLOAD_LOOP_END

bitsadmin /complete third_party_download_desktop

echo "---------- Download complete ----------"
echo "---------- Preparing PopcornFX_Runtime_SDK/ ----------"

rem rmdir /s /q "%~dp0\PopcornFX_Runtime_SDK"
mkdir "%~dp0\PopcornFX_Runtime_SDK"

echo "---------- Unzipping _PopcornFX_Runtime_SDK.7z ----------"

7z x _PopcornFX_Runtime_SDK_Desktop.7z -o_PopcornFX_Runtime_SDK_Desktop

echo "---------- Copying PopcornFX Runtime SDK ----------"

robocopy _PopcornFX_Runtime_SDK_Desktop\PopcornFX\PopcornFX_Runtime_SDK "%~dp0\PopcornFX_Runtime_SDK" /s

echo "---------- Removing temp files ----------"

rmdir /s /q "%~dp0\_PopcornFX_Runtime_SDK_Desktop"
del /f _PopcornFX_Runtime_SDK_Desktop.7z

echo "---------- Finished ----------"

endlocal
