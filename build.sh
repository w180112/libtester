set -ex

./essentials.sh
git submodule update --init --recursive
pushd lib/libutil
autoreconf --install
./configure
make && make install
popd

pushd lib/picohttpparser
gcc -fPIC -c -o picohttpparser.o picohttpparser.c
ar rcs libpicohttpparser.a picohttpparser.o
gcc -shared -o libpicohttpparser.so picohttpparser.o
cp libpicohttpparser.so /usr/local/lib64/libpicohttpparser.so
cp libpicohttpparser.a /usr/local/lib64/libpicohttpparser.a
cp picohttpparser.h /usr/local/include/picohttpparser.h
popd

mkdir bin
make && make install