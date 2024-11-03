pushd skidtool\bin
cmake ..
ninja
popd

if %errorlevel% neq 0 exit /b %errorlevel%

pushd MyACID500
..\skidtool\bin\acid500.exe
popd
