@echo off

rd /S /Q build.dir
mkdir build.dir
cd build.dir
cmake ..
python ../scripts/msbuild.py /nologo /flp:LogFile=full-build.log /clp:Verbosity=minimal /m /p:Configuration=Release JiraDesktop.sln
python ../scripts/jenkins_win32.py %1%.zip
