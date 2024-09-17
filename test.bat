pushd skidtool\bin
cmake ..
ninja
popd

pushd MyACID500
..\skidtool\bin\acid500.exe
popd
