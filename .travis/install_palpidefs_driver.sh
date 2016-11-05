#!/bin/bash

# This package is necessary for the palpidefs producer

echo "Entering install_palpidefs_driver"
echo "Installing palpidefs driver"

export temporary_path=`pwd`

cd --

if [ $TRAVIS_OS_NAME == linux ]; then 
	
	sudo apt-get update && sudo apt-get install -y libtinyxml-dev expect-dev libusb-1.0-0-dev; 
	
	wget -O alice-its-alpide-software-master-latest.zip https://cernbox.cern.ch/index.php/s/QIRPTV84XziyQ3q/download

	unzip alice-its-alpide-software-master-latest.zip

	cd alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b

	cd pALPIDEfs-software

	sed -i '2s/.*//' Makefile
	
	make lib

fi

# http://apple.stackexchange.com/questions/193138/to-install-unbuffer-in-osx

if [ $TRAVIS_OS_NAME == osx ]; then 

	brew update
	#brew --prefix
	#brew -ls verbose tinyxml
	#brew install tinyxml homebrew/dupes/expect libusb
	brew install homebrew/dupes/expect libusb

	wget -O tinyxml_2_6_2.zip http://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip

	unzip tinyxml_2_6_2.zip

	cd tinyxml

	make
	
	echo $PATH
	
	export PATH=/Users/travis/tinyxml:$PATH
	export CFLAGS="-I /Users/travis/tinyxml" $CFLAGS
	export LDFLAGS="-L /Users/travis/tinyxml" $LDFLAGS
	
	cd ..	
	
	wget -O alice-its-alpide-software-master-latest.zip https://cernbox.cern.ch/index.php/s/QIRPTV84XziyQ3q/download

	unzip alice-its-alpide-software-master-latest.zip

	cd alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b

	cd pALPIDEfs-software

	sed -i '' '2s/.*//' Makefile
	sed -i '' '3s/.*/CFLAGS= -I\/Users\/travis\/tinyxml -pipe -fPIC -DVERSION=\"$(GIT_VERSION)\" -g -std=c++0x/' Makefile
	sed -i '' '4s/.*/LINKFLAGS=-L\/Users\/travis\/tinyxml -lusb-1.0 -ltinyxml/' Makefile		
	
	make lib
	
fi

pwd

cd $temporary_path

echo "Installed palpidefs driver"