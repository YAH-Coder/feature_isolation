#!/bin/bash

set -e

usage() {
	echo "Usage: $0 TARGET"
	echo "TARGET can be a directory or a single file."
	exit 1
}

if [ "$#" -ne 1 ]; then
	usage
fi

LICENSE='/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */'

TARGET="$1"

# Function to prepend license to a single file
prepend_license() {
	local file="$1"

	if [ ! -f "$file" ]; then
		echo "File '$file' does not exist. Skipping."
		return
	fi

	if grep -q "This Source Code Form is subject to the terms of the Mozilla Public" "$file"; then
		echo "License already present in '$file', skipping."
	else
		{
			echo "$LICENSE"
			cat "$file"
		} >temp && mv temp "$file"
		echo "Prepended license to '$file'."
	fi
}

# Function to process a directory
process_directory() {
	local dir="$1"

	if [ ! -d "$dir" ]; then
		echo "Directory '$dir' does not exist."
		exit 1
	fi

	cd "$dir" || {
		echo "Failed to navigate to directory '$dir'."
		exit 1
	}

	EXTENSIONS=("cpp" "hpp")

	for ext in "${EXTENSIONS[@]}"; do
		for file in *."$ext"; do
			[ -e "$file" ] || continue

			prepend_license "$file"
		done
	done
}

if [ -d "$TARGET" ]; then
	echo "Processing directory '$TARGET'..."
	process_directory "$TARGET"
elif [ -f "$TARGET" ]; then
	echo "Processing file '$TARGET'..."
	prepend_license "$TARGET"
else
	echo "TARGET '$TARGET' is neither a file nor a directory."
	usage
fi
