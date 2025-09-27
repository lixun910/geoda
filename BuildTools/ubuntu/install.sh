#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

# Default to regular Ubuntu build if BUILD_TYPE not specified
BUILD_TYPE=${BUILD_TYPE:-"regular"}

if [ "$BUILD_TYPE" = "appimage" ]; then
    echo "Building GeoDa for AppImage"
else
    echo "Building GeoDa for Ubuntu"
fi

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
mkdir -p o
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 
echo $GEODA_HOME

# Build GeoDa
cp ../../GeoDamake.ubuntu.opt ../../GeoDamake.opt

# Add static linking flags for AppImage builds
if [ "$BUILD_TYPE" = "appimage" ]; then
    export EXTRA_GEODA_LD_FLAGS="$EXTRA_GEODA_LD_FLAGS -static-libgcc -static-libstdc++ -Wl,--disable-new-dtags"
    echo "Building with EXTRA_GEODA_LD_FLAGS: $EXTRA_GEODA_LD_FLAGS"
fi

make -j$(nproc)
make app

if [ "$BUILD_TYPE" = "appimage" ]; then
    echo "GeoDa build for AppImage completed successfully"
    
    # Verify the executable was created
    if [ -f "build/GeoDa" ] ; then
        echo "GeoDa executable created successfully at build/GeoDa"
        # Show what libraries the executable depends on
        echo "Library dependencies:"
        ldd build/GeoDa || echo "Static executable (no dynamic dependencies)"
    else
        echo "ERROR: GeoDa executable not found!"
        exit 1
    fi
else
    echo "GeoDa build for Ubuntu completed successfully"
    
    # Create deb package for regular builds
    ./create_deb.sh $OS $VER
fi