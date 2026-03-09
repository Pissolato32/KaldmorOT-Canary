#!/bin/bash

set -euo pipefail

# Variáveis
VCPKG_PATH=${1:-"$HOME"}
VCPKG_PATH=$VCPKG_PATH/vcpkg/scripts/buildsystems/vcpkg.cmake
BUILD_TYPE=${2:-"linux-release"}
ARCHITECTURE=$(uname -m)
ARCHITECTUREVALUE=0

# Function to print information messages
info() {
	echo -e "\033[1;34m[INFO]\033[0m $1"
}

# Function to check if a command is available
check_command() {
	if ! command -v "$1" >/dev/null; then
		echo "The command '$1' is not available. Please install it and try again."
		exit 1
	fi
}

check_architecture() {
	if [[ $ARCHITECTURE == "aarch64"* ]]; then
		info "its architecture is $ARCHITECTURE (ARM)"
		ARCHITECTUREVALUE=1
	else
		info "its architecture is $ARCHITECTURE"
	fi
}

# Function to check if sccache is available
check_sccache() {
	if command -v sccache >/dev/null; then
		SCCACHE_AVAILABLE=1
		info "sccache detected and will be used to speed up the build."
		# Ensure sccache is started
		sccache --start-server >/dev/null 2>&1 || true
	else
		SCCACHE_AVAILABLE=0
	fi
}

# Function to configure Canary
setup_canary() {
	# Check if cmake has already been configured for the real binary directory
	if [ -f "build/${BUILD_TYPE}/CMakeCache.txt" ]; then
		info "Canary has already been configured, skipping configuration step..."
		return 0 # skip cmake configure
	fi
	info "Configuring Canary..."
	return 1 # needs configuration
}

# Function to build Canary
build_canary() {
	if ! setup_canary; then
		if [[ $ARCHITECTUREVALUE == 1 ]]; then
			export VCPKG_FORCE_SYSTEM_BINARIES=1
		fi

		local cmake_args=("-DCMAKE_TOOLCHAIN_FILE=$VCPKG_PATH" "." "--preset" "$BUILD_TYPE")
		if [[ $SCCACHE_AVAILABLE == 1 ]]; then
			cmake_args+=("-DOPTIONS_ENABLE_SCCACHE=ON")
		fi

		# Configure from root using . as source dir
		cmake "${cmake_args[@]}" >cmake_log.txt 2>&1 || {
			cat cmake_log.txt
			return 1
		}
	fi

	info "Starting the build process..."

	local total_steps=0
	local progress=0
	local build_status=0

	global_beats=0
	local temp_file="temp_global_beats.txt"
	echo "0" >$temp_file

	# Build using preset from root
	cmake --build --preset "$BUILD_TYPE" --verbose 2>&1 | while IFS= read -r line; do
		echo "$line" >>build_log.txt
		if [[ $line =~ ^\[([0-9]+)/([0-9]+)\].* ]]; then
			current_step=${BASH_REMATCH[1]}
			total_steps=${BASH_REMATCH[2]}
			progress=$((current_step * 100 / total_steps))
			printf "\r\033[1;32m[INFO]\033[0m Progress build: [%3d%%] %s" $progress "${line#*] }"
			echo "1" >$temp_file
		else
			echo "$line"
		fi
	done || build_status=1

	global_beats=$(cat $temp_file)
	rm $temp_file

	if [[ $SCCACHE_AVAILABLE == 1 ]]; then
		echo
		info "sccache build statistics:"
		sccache --show-stats
	fi

	if [[ $build_status -eq 0 ]]; then
		if [[ $global_beats == 1 ]]; then
			echo
		fi
		return 0
	else
		echo
		cat build_log.txt
		return 1
	fi
}

# Function to move the generated executable
move_executable() {
	local executable_name="canary"
	# No need to cd .. as we stay in root
	if [ -e "$executable_name" ]; then
		info "Saving old build"
		mv ./"$executable_name" ./"$executable_name".old
	fi
	info "Moving the generated executable to the canary folder directory..."
	# Path is now relative to root: build/{preset}/bin/canary
	cp ./build/"$BUILD_TYPE"/bin/"$executable_name" ./"$executable_name"
	info "Build completed successfully!"
}

# Main function
main() {
	check_command "cmake"
	check_architecture
	check_sccache

	if build_canary; then
		move_executable
	else
		echo -e "\033[31m[ERROR]\033[0m Build failed..."
		exit 1
	fi
}

main
