#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

# Set BUILD_TYPE to indicate this is an AppImage build
export BUILD_TYPE="appimage"

# Reuse the main create_deps.sh script with AppImage-specific settings
exec ./create_deps.sh