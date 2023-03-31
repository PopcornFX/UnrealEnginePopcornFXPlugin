@echo off

echo "---------- Downloading PopcornFX Runtime SDK for Unreal Engine (Mobile platforms) ----------"

setlocal

bitsadmin /reset
bitsadmin /create third_party_download_mobile
bitsadmin /addfile third_party_download_mobile https://downloads.popcornfx.com/Plugins/UE4/UnrealEngine_PopcornFXPlugin_2.15.5_iOS_Android.7z "%~dp0\_PopcornFX_Runtime_SDK_Mobile.7z"
bitsadmin /setpriority third_party_download_mobile "FOREGROUND"
bitsadmin /resume third_party_download_mobile

:WAIT_FOR_DOWNLOAD_LOOP_START
    call bitsadmin /info third_party_download_mobile /verbose | find "STATE: TRANSFERRED"
    if %ERRORLEVEL% equ 0 goto WAIT_FOR_DOWNLOAD_LOOP_END
    call bitsadmin /RawReturn /GetBytesTransferred third_party_download_mobile
    printf ' / '
    call bitsadmin /RawReturn /GetBytesTotal third_party_download_mobile
    echo Bytes transferred
    timeout 2 > nul
    goto WAIT_FOR_DOWNLOAD_LOOP_START
:WAIT_FOR_DOWNLOAD_LOOP_END

bitsadmin /complete third_party_download_mobile

echo "---------- Download complete ----------"
echo "---------- Preparing PopcornFX_Runtime_SDK/ ----------"

rem rmdir /s /q "%~dp0\PopcornFX_Runtime_SDK"
mkdir "%~dp0\PopcornFX_Runtime_SDK"

echo "---------- Unzipping _PopcornFX_Runtime_SDK_Mobile.7z ----------"

7z x _PopcornFX_Runtime_SDK_Mobile.7z -o_PopcornFX_Runtime_SDK_Mobile

echo "---------- Copying PopcornFX Runtime SDK ----------"

robocopy _PopcornFX_Runtime_SDK_Mobile\PopcornFX\PopcornFX_Runtime_SDK "%~dp0\PopcornFX_Runtime_SDK" /s

echo "---------- Removing temp files ----------"

rmdir /s /q "%~dp0\_PopcornFX_Runtime_SDK_Mobile"
del /f _PopcornFX_Runtime_SDK_Mobile.7z

echo "---------- Finished ----------"

endlocal
