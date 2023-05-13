set -ex

if [ -f /etc/os-release ]; then
    . /etc/os-release
    ID=$ID
elif type lsb_release >/dev/null 2>&1; then
    ID=$ID
fi

case $ID in
    ubuntu)
	echo "Detect ubuntu"
	sudo apt update
	sudo apt install git gcc make python3-dev libtool-bin libconfig-dev uuid-dev -y
    ;;
    rhel)
	    echo "Detect RedHat Linux"
	    if [ -f /etc/redhat-release ]; then
            version=$( cat /etc/redhat-release | grep -oP "[0-9]+" | head -1 )
        fi
        case $version in
            9)
                sudo subscription-manager repos --enable codeready-builder-for-rhel-9-x86_64-rpms
	        ;;
	        8)
                sudo subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms
            ;;
    	    *)
		        echo "Unsupported RedHat Linux version"
		        exit 127
	        ;;
	    esac
        sudo echo /usr/local/lib > /etc/ld.so.conf.d/local.conf
	    sudo dnf install libconfig-devel libuuid-devel autoconf libtool -y
    ;;
    rocky) 
    ;&
    centos)
        echo "Detect CentOS/Rocky Linux"
        echo /usr/local/lib > /etc/ld.so.conf.d/local.conf
	    sudo dnf --enablerepo=devel install libconfig-devel -y
	    sudo dnf install libuuid-devel autoconf libtool -y
    ;;
    *) 
	    echo "Detect unknown distribution."
        exit 127
    ;;
esac