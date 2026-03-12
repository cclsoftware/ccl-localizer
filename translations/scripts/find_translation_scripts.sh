#!/bin/bash
#
# Reads the repository configuration and returns the list of the directories
#
# Usage       : ./find_translation_scripts.sh
#               The script will search the shared translation scripts
#               and return the path
#
#************************************************************************************************

scriptdir=$(dirname "$0")
config_name="repo.json"
script_hint="makepot.sh"
script_path=""

config_path=$(${scriptdir}/find_upwards.sh "${config_name}")
config_file="${config_path}/${config_name}"

if [ ! -f "${config_file}" ]; then
	>&2 echo "The file repo.json is not found!"
	exit 1
fi

translation_paths=$(grep "translations" "${config_file}" | grep -oE '\"([^\"]+)?\"' | sed 's/"//g' | tail -n +2)

for translation_path in ${translation_paths}; do
	if [ -x "${config_path}/${translation_path}/scripts/${script_hint}" ]; then
        script_path="${config_path}/${translation_path}/scripts"
        break
    fi
done

if [ -z "${script_path}" ]; then
	>&2 echo "The path for translation scripts cannot be found"
	exit 1
fi

echo ${script_path}

exit 0