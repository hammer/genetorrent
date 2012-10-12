REM Batch script for running GeneTorrent tests on Windows
@echo off
set PYTHONBIN=C:\Python27
set OPENSSLBIN=C:\OpenSSL-Win32\bin

echo Setting up environment
set PATH=%PYTHONBIN%;%PATH%
set PATH=%OPENSSLBIN%;%PATH%
set srcdir=.
set PYTHONPATH=.

echo Setting up web.
del /F /S /Q web
if ERRORLEVEL 1 goto commandfail
mkdir web
xcopy /S web.py-0.37\web web\
if ERRORLEVEL 1 goto commandfail

echo Running tests

python gt_argument_tests.py
if ERRORLEVEL 1 goto fail

python gt_upload_tests.py
if ERRORLEVEL 1 goto fail

python gt_download_tests.py
if ERRORLEVEL 1 goto fail

python gt_inactivity_tests.py
if ERRORLEVEL 1 goto fail

python gt_curl_ssl_verify_tests.py
if ERRORLEVEL 1 goto fail

python gt_cred_as_uri_tests.py
if ERRORLEVEL 1 goto fail

goto success

:fail
del /F /S /Q web
rmdir /S /Q web
echo A test failed
exit /B

:commandfail
del /F /S /Q web
rmdir /S /Q web
echo A command failed.  Try removing "echo off" from the batch script.
exit /B

:success
REM clean up
del /F /S /Q web
rmdir /S /Q web
echo All tests completed successfully
