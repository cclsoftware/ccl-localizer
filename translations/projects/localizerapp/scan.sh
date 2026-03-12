#!/bin/bash

scriptdir=$(dirname "$0")
translation_scripts=$(${scriptdir}/../../scripts/find_translation_scripts.sh)

${translation_scripts}/scan.sh
