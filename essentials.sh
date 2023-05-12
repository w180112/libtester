set -ex

subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms
dnf install libconfig-devel -y
#apt update
#apt install git gcc make python3-dev libtool-bin libconfig-dev libc6-dev -y
