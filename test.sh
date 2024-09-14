mkdir -p skidtool/bin
pushd skidtool/bin
cmake ..
make
popd

pushd MyACID500
../skidtool/bin/acid500
popd
