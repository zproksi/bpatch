set -e

divisor() {
    echo -e "\033[35m----------------------------------\033[0m"
}

# Establish the mode, defaulting to Release if no argument is given
MODE=Release
if [ "$1" != "" ]; then
    MODE=$1
fi

# Check if MODE is not Debug, if true set it to Release
if [ "$MODE" != "Debug" ] && [ "$MODE" != "RelWithDebInfo" ]; then
    MODE=Release
fi

CWD=$(pwd)
SCRIPT_PATH=""

get_script_path() {
    local CWDLocal=$(pwd)
    # Handle symbolic links
    if [ -L "$0" ]; then
        SCRIPT_PATH="$( dirname "$( readlink -f "$0" )" )"
    else
        SCRIPT_PATH="$( cd "$( dirname "$0" )" >/dev/null 2>&1 && pwd )"
    fi
    cd ${CWDLocal}
}

#check cmake presence
command -v cmake >/dev/null 2>&1 || { echo >&2 "CMake is not installed. Aborting."; exit 1; }

get_script_path

divisor
echo " Rebuilding of bpatch $MODE mode"
divisor


# Establish the directory containing the script
PROJECTDIR=${SCRIPT_PATH}

# rebuilding initiation
cd $PROJECTDIR
rm -rf "buildLinux$MODE"
mkdir "buildLinux$MODE" && cd "buildLinux$MODE"
cmake -DCMAKE_BUILD_TYPE=$MODE ..
cmake --build . --config $MODE

divisor

FILENAME="$PROJECTDIR/buildLinux$MODE/bpatch"

if [ -e "$FILENAME" ]; then
    echo "bpatch built for $MODE mode is here:"
    echo "$FILENAME"
else
    echo -e "\033[31mERROR\033[0m bpatch built for $MODE mode could not be found at"
    echo "$FILENAME"
fi

FILENAME="$PROJECTDIR/buildLinux$MODE/testbpatch/testbpatch"

if [ -e "$FILENAME" ]; then
    echo "Unit tests for bpatch are here:"
    echo "$FILENAME"
else
    echo -e "\033[31mERROR\033[0m Failed to find unit tests for bpatch here:"
    echo "$FILENAME"
fi

divisor

# Restore the initial execution directory
cd $CWD
