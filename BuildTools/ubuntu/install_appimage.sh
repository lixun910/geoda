#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

# Set BUILD_TYPE to indicate this is an AppImage build
export BUILD_TYPE="appimage"

# Reuse the main install.sh script with AppImage-specific settings
exec ./install.sh