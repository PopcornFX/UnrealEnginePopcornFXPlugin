@echo off

rem Converts all dss to u8888 dds textures

rem Download and install:
rem https://developer.nvidia.com/gameworksdownload#?dn=dds-utilities-8-31

setlocal

set "NVDXT_1=%programfiles(x86)%\NVIDIA Corporation\DDS Utilities\nvdxt.exe"
set "NVDXT_2=%programfiles%\NVIDIA Corporation\DDS Utilities\nvdxt.exe"

if exist "%NVDXT_1%" (
	set "NVDXT=%NVDXT_1%"
) else if exist "%NVDXT_2%" (
	set "NVDXT=%NVDXT_2%"
) else (
	echo --
	echo NVIDIA DDS Utilities not found !!!
	echo see https://developer.nvidia.com/gameworksdownload#?dn=dds-utilities-8-31
	echo --
	pause
	exit /B 1
)

set dstFormat=u8888

echo --
echo About to convert all dds files to %dstFormat%
echo continue ????
echo --

pause

for %%x in (*.dds) do (
	rem echo %%x
	"%NVDXT%" -file "%%x" -%dstFormat% -outfile "%%x"
)

echo --
echo OK
echo --

pause

