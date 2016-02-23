#!/bin/bash

set -e

# Determine PSMoveAPI root dir
if [[ -v $TRAVIS_BUILD_DIR ]]
then
	PSMOVE_API_ROOT_DIR=$TRAVIS_BUILD_DIR
else
	PSMOVE_API_ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/../../"
fi

cd $PSMOVE_API_ROOT_DIR

# Only install packages here if not running through TravisCI; 
# it has its own system for installing packages (and not all of them are available)
if [ -z $TRAVIS ]
then
	echo "Installing prerequisites..."
	sudo apt-get update -qq
	sudo apt-get install -q  build-essential cmake 			\
			     	 libudev-dev libbluetooth-dev 		\
			     	 libv4l-dev libopencv-dev 		\
			     	 openjdk-7-jdk ant liblwjgl-java 	\
			     	 python-dev mono-mcs 			\
			     	 swig3.0 libsdl2-dev freeglut3-dev
fi

# Configure
echo "Configuring build..."

mkdir -p build
cd build
JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64/ 
cmake ..

# Build
echo "Building..."
make -j4

echo "Build completed successfully"
