set -ex

version=$(cat VERSION)
git tag -a $version -m "version $version"
git push origin $version
