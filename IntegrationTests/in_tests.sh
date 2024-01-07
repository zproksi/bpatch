set -e

divisor() {
    echo -e "\033[35m========\033[0m"
}


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


howto()
{
    echo "To run script profide to script 'bpatch' executable name as parameter"
    echo "  $0  path_to/bpatch"
}

if [ -z "$1" ] 
then
    howto
    exit 1
fi

divisor
echo "This script is located at: $SCRIPT_PATH"
echo "current folder is ${CWD}"
echo "Files will be written here"
divisor


get_script_path
# folder with actions
FOLDER_FA=${SCRIPT_PATH}/scripts
# folder with binary files if they exists in test
FOLDER_FB=${SCRIPT_PATH}/binaries

# folder for source files
FOLDER_SOURCE=${SCRIPT_PATH}/sources
# folder with expected output results
FOLDER_EXPECTED=${SCRIPT_PATH}/expected


RETURN_VALUE=0

if [[ -d "${FOLDER_SOURCE}" ]]; then
  # Iterate over files in the directory
  for file in "${FOLDER_SOURCE}"/*; do  
    if [[ -f "$file" ]]; then
FILENAME=$(basename "$file")
BASEFILENAME="${FILENAME%.*}"

SRC=${FOLDER_SOURCE}/${BASEFILENAME}.test
ACT=${BASEFILENAME}.json
DEST=${SCRIPT_PATH}/${BASEFILENAME}.res
COMMAND="$1 -s ${SRC} -a ${ACT} -w ${DEST} -fa ${FOLDER_FA} -fb ${FOLDER_FB}"
        echo "to execute: '${COMMAND}'"
${COMMAND}

EXPECTEDDATA=${FOLDER_EXPECTED}/${BASEFILENAME}.expected

        divisor
        if cmp -s "${EXPECTEDDATA}" "${DEST}"; then
           echo -e "\033[32mOK\033[0m for ${BASEFILENAME}.test" 
           rm ${DEST}
        else
           echo -e "\033[31mERROR\033[0m for ${BASEFILENAME}.test"
           RETURN_VALUE=1
        fi
        divisor
    fi
  done
else
  echo "${FOLDER_SOURCE} with sources for testing does not exists."
fi

if [ "${RETURN_VALUE}" -ne 0 ]; then
    echo -e "\033[31mERROR\033[0m detected"
else 
    echo -e "\033[32mOK\033[0m for every test"
fi

exit ${RETURN_VALUE}
