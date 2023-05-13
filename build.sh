set -ex

#./essentials.sh
git submodule update --init --recursive
pushd lib/libutil
autoreconf --install
./configure
make && sudo make install
popd

pushd lib/picohttpparser
gcc -fPIC -c -o picohttpparser.o picohttpparser.c
ar rcs libpicohttpparser.a picohttpparser.o
gcc -shared -o libpicohttpparser.so picohttpparser.o
sudo cp libpicohttpparser.so /usr/local/lib64/libpicohttpparser.so
sudo cp libpicohttpparser.a /usr/local/lib64/libpicohttpparser.a
sudo cp picohttpparser.h /usr/local/include/picohttpparser.h
popd

mkdir bin
make && sudo make install