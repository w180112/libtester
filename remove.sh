set -ex

sudo make uninstall
sudo rm -rf /usr/local/lib/libpicohttpparser.*
sudo rm -rf /usr/local/include/picohttpparser.h
pushd lib/libutil
sudo make uninstall
popd