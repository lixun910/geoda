#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

# Default to regular Ubuntu build if BUILD_TYPE not specified
BUILD_TYPE=${BUILD_TYPE:-"regular"}

if [ "$BUILD_TYPE" = "appimage" ]; then
    echo "Creating dependencies for AppImage build"
else
    echo "Creating dependencies for Ubuntu build"
fi

echo $OS
echo $VER
echo $APT

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 
mkdir -p libraries
mkdir -p libraries/lib
mkdir -p libraries/include
mkdir -p temp

cd temp

# Install libgdal and development tools
if [ "$BUILD_TYPE" = "appimage" ]; then
    # Install development tools and headers (but not runtime packages for AppImage)
    export DEBIAN_FRONTEND=noninteractive
else
    # Install libgdal for regular builds
    export DEBIAN_FRONTEND=noninteractive
fi
$APT update -y
# fix curl 60 error
$APT install -y ca-certificates libgnutls30
echo '-k' > ~/.curlrc
$APT install -y libpq-dev
$APT install -y gdal-bin
$APT install -y libgdal-dev
$APT install -y unzip cmake dh-autoreconf libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev 

if  [ $OS = 'jammy' ] ; then
    $APT install -y libwebkit2gtk-4.0-dev
elif  [ $OS = 'focal' ] ; then
    $APT install -y libwebkit2gtk-4.0-dev
elif  [ $OS = 'noble' ] ; then
    $APT install -y libgtk-4-dev libwebkit2gtk-4.1-dev
else
    $APT install -y libwebkitgtk-3.0-dev 
fi

# Install boost 1.75
if ! [ -f "boost_1_75_0.tar.bz2" ] ; then
    curl -L -O https://pilotfiber.dl.sourceforge.net/project/boost/boost/1.75.0/boost_1_75_0.tar.bz2
fi
if ! [ -d "boost" ] ; then 
    tar -xf boost_1_75_0.tar.bz2 
    mv boost_1_75_0 boost
fi
cd boost
./bootstrap.sh
if [ "$BUILD_TYPE" = "appimage" ]; then
    # Build boost statically for AppImage
    ./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi stage
else
    # Build boost normally for regular builds
    ./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi stage
fi
cd ..

# Build JSON Spirit v4.08
if ! [ -f "json_spirit_v4.08.zip" ] ; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip
fi
if ! [ -d "json_spirit_v4.08" ] ; then 
    unzip json_spirit_v4.08.zip
fi
cd json_spirit_v4.08
cp ../../dep/json_spirit/CMakeLists.txt .
mkdir -p bld
cd bld
cmake -DBoost_NO_BOOST_CMAKE=TRUE -DBOOST_ROOT:PATHNAME=$GEODA_HOME/temp/boost  ..
make -j$(nproc)
cp -R ../json_spirit ../../../libraries/include/.
cp json_spirit/libjson_spirit.a ../../../libraries/lib/.
cd ..
cd ..

# Build CLAPACK
if ! [ -f "clapack.tgz" ] ; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/clapack.tgz
fi
if ! [ -d "CLAPACK-3.2.1" ] ; then 
    tar -xf clapack.tgz
    cp -rf ../dep/CLAPACK-3.2.1 .
fi
cd CLAPACK-3.2.1
make -j$(nproc) f2clib
make -j$(nproc) blaslib
cd INSTALL
make -j$(nproc)
cd ..
cd SRC
make -j$(nproc)
cd ..
cp F2CLIBS/libf2c.a .
cd ..

# Build Eigen3 and Spectra
if ! [ -f "eigen3.zip" ] ; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/eigen3.zip
    unzip eigen3.zip
fi
if ! [ -f "v0.8.0.zip" ] ; then
    curl -L -O https://github.com/yixuan/spectra/archive/refs/tags/v0.8.0.zip
    unzip v0.8.0.zip
    mv spectra-0.8.0 spectra
fi

# Build wxWidgets 3.2.4
if ! [ -f "wxWidgets-3.2.4.tar.bz2" ] ; then
    curl -L -O https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.4/wxWidgets-3.2.4.tar.bz2
fi
if ! [ -d "wxWidgets-3.2.4" ] ; then 
    tar -xf wxWidgets-3.2.4.tar.bz2
fi
cd wxWidgets-3.2.4
chmod +x configure
if [ "$BUILD_TYPE" = "appimage" ]; then
    # Configure for static build for AppImage
    ./configure --with-gtk=3 --disable-shared --enable-monolithic --with-opengl --enable-postscript --without-libtiff --disable-debug --enable-webview --prefix=$GEODA_HOME/libraries
else
    # Configure for regular build
    ./configure --with-gtk=3 --disable-shared --enable-monolithic --with-opengl --enable-postscript --without-libtiff --disable-debug --enable-webview --prefix=$GEODA_HOME/libraries
fi
make -j$(nproc)
make install
cd ..
cd ..

if [ "$BUILD_TYPE" = "appimage" ]; then
    echo "AppImage dependencies created successfully"
else
    echo "Ubuntu dependencies created successfully"
fi
