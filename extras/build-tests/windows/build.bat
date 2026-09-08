echo on

rem Temporarily hardcoded:
set TARGET=Visual Studio 2019
set SHORTNAME=vs2019

rem Initialize Visual Studio variables
if "%TARGET%" == "Visual Studio 2017" call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
if "%TARGET%" == "Visual Studio 2019" call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

rem Installing tools
rem only for appveyor:
rem cinst unrar -y
rem cinst unzip -y
rem cinst innosetup -y

rem Installing ZenIRCd dependencies
cd \projects
mkdir zenircd-6-libs
cd zenircd-6-libs
curl -fsS -o zenircd-libraries-6-devel.zip https://www.unrealircd.org/files/dev/win/libs/zenircd-libraries-6-devel.zip
unzip zenircd-libraries-6-devel.zip
copy dlltool.exe \users\user\worker\unreal6-w10\build /y

rem for appveyor, use: cd \projects\zenircd
cd \users\user\worker\unreal6-w10\build

rem Install 'zenircd-tests'
cd ..
rd /q/s zenircd-tests
git clone -q --branch unreal60 https://github.com/zenircd/zenircd-tests.git zenircd-tests
if %ERRORLEVEL% NEQ 0 EXIT /B 1
cd build

rem Now the actual build
rem - First this, otherwise JOM will fail
IF NOT EXIST src\version.c nmake -f Makefile.windows CONF
rem - Then build most of ZenIRCd.exe etc
call extras\build-tests\windows\compilecmd\%SHORTNAME%.bat ZENSVC.EXE ZenIRCd.exe zenircdctl.exe
rem - It will fail due to missing symbolfile, which we create here..
rem   it needs to run with SLOW=1 because JOM doesn't understand things otherwise..
SET SLOW=1
call extras\build-tests\windows\compilecmd\%SHORTNAME%.bat SYMBOLFILE
SET SLOW=0
rem - Then we finalize building ZenIRCd.exe: should be no error
call extras\build-tests\windows\compilecmd\%SHORTNAME%.bat ZENSVC.EXE ZenIRCd.exe zenircdctl.exe
if %ERRORLEVEL% NEQ 0 EXIT /B 1
rem - Build all the modules (DLL files): should be no error
call extras\build-tests\windows\compilecmd\%SHORTNAME%.bat MODULES
if %ERRORLEVEL% NEQ 0 EXIT /B 1

rem Compile dependencies for zenircd-tests -- this doesn't belong here though..
copy ..\zenircd-tests\serverconfig\zenircd\modules\fakereputation.c src\modules\third /Y
call extras\build-tests\windows\compilecmd\%SHORTNAME%.bat CUSTOMMODULE MODULEFILE=fakereputation
if %ERRORLEVEL% NEQ 0 EXIT /B 1

rem Convert c:\dev to c:\projects\zenircd-6-libs
rem TODO: should use environment variable in innosetup script?
sed -i "s/c:\\dev\\zenircd-6-libs/c:\\projects\\zenircd-6-libs/gi" src\windows\zeninst.iss

rem Build installer file
"c:\Program Files (x86)\Inno Setup 5\iscc.exe" /Q- src\windows\zeninst.iss
if %ERRORLEVEL% NEQ 0 EXIT /B 1

rem Show some proof
ren mysetup.exe zenircd-dev-build.exe
dir zenircd-dev-build.exe
sha256sum zenircd-dev-build.exe

rem Kill any old instances, just to be sure
taskkill -im zenircd.exe -f
sleep 2
rem Just a safety measure so we don't end up testing
rem some old version...
del "C:\Program Files\ZenIRCd 6\bin\zenircd.exe"

echo Running installer...
start /WAIT zenircd-dev-build.exe /VERYSILENT /LOG=setup.log
if %ERRORLEVEL% NEQ 0 goto installerfailed

rem Upload artifact
rem appveyor PushArtifact zenircd-dev-build.exe
rem if %ERRORLEVEL% NEQ 0 EXIT /B 1

cd ..\zenircd-tests
dir

rem All tests except db:
"C:\Program Files\Git\bin\bash.exe" ./runwin
if %ERRORLEVEL% NEQ 0 EXIT /B 1

rem Test unencrypted db's:
"C:\Program Files\Git\bin\bash.exe" ./runwin -boot tests/db/writing/*
if %ERRORLEVEL% NEQ 0 EXIT /B 1
"C:\Program Files\Git\bin\bash.exe" ./runwin -keepdbs -boot tests/db/reading/*
if %ERRORLEVEL% NEQ 0 EXIT /B 1

rem Test encrypted db's:
"C:\Program Files\Git\bin\bash.exe" ./runwin -include db_crypted.conf -boot tests/db/writing/*
if %ERRORLEVEL% NEQ 0 EXIT /B 1
"C:\Program Files\Git\bin\bash.exe" ./runwin -include db_crypted.conf -keepdbs -boot tests/db/reading/*
if %ERRORLEVEL% NEQ 0 EXIT /B 1

goto end



:installerfailed
type setup.log
echo INSTALLATION FAILED
EXIT /B 1

:end
