#!/bin/bash
#
# Firmware Build Script for ESP32 MMDVM Hotspot
# Builds both normal and beta versions with automatic version management
#

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
CONFIG_FILE="config.h"
NORMAL_VERSION_SUFFIX="ESP32"
BETA_VERSION_SUFFIX="ESP32_BETA"
NO_PUSH=false  # Default: push to git
NO_COMMIT=false  # Default: commit to git

# Arduino CLI settings
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,CPUFreq=240,DebugLevel=none,DFUOnBoot=default,EraseFlash=none,EventsCore=1,FlashMode=qio,FlashSize=16M,JTAGAdapter=default,LoopCore=1,MSCOnBoot=default,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadMode=default,UploadSpeed=921600,USBMode=hwcdc,ZigbeeMode=default"
BUILD_DIR="build/esp32.esp32.esp32s3"
BINARY_NAME="esp32_mmdvm_hotspot.ino.bin"

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to get current date in YYYYMMDD format
get_version_date() {
    date +%Y%m%d
}

# Function to update version in config.h
update_version() {
    local version_string="$1"
    local backup_file="${CONFIG_FILE}.bak"

    print_info "Updating version to: $version_string"

    # Create backup
    cp "$CONFIG_FILE" "$backup_file"

    # Update the version line
    # Comment out the non-matching version and uncomment/update the matching one
    if [[ "$version_string" == *"BETA"* ]]; then
        # Building beta version
        sed -i.tmp \
            -e 's|^#define FIRMWARE_VERSION.*ESP32".*|//#define FIRMWARE_VERSION "'"$version_string"'"  // Update version as needed|' \
            -e 's|^//#define FIRMWARE_VERSION.*BETA".*|#define FIRMWARE_VERSION "'"$version_string"'"  // Update version as needed|' \
            "$CONFIG_FILE"
    else
        # Building normal version
        sed -i.tmp \
            -e 's|^//#define FIRMWARE_VERSION.*ESP32".*|#define FIRMWARE_VERSION "'"$version_string"'"  // Update version as needed|' \
            -e 's|^#define FIRMWARE_VERSION.*BETA".*|//#define FIRMWARE_VERSION "'"$version_string"'"  // Update version as needed|' \
            "$CONFIG_FILE"
    fi

    # Remove the temp file created by sed on macOS
    rm -f "${CONFIG_FILE}.tmp"

    print_success "Version updated in $CONFIG_FILE"
}

# Function to restore config.h from backup
restore_config() {
    if [ -f "${CONFIG_FILE}.bak" ]; then
        print_info "Restoring original config.h"
        mv "${CONFIG_FILE}.bak" "$CONFIG_FILE"
    fi
}

# Function to clean build directory
clean_build() {
    print_info "Cleaning build directory..."
    rm -rf build 2>/dev/null || true
    print_success "Build directory cleaned"
}

# Function to compile firmware
compile_firmware() {
    print_info "Compiling firmware..."
    arduino-cli compile --fqbn "$FQBN" . --export-binaries --verbose
    print_success "Compilation completed"
}

# Function to copy binary to firmware directory
copy_binary() {
    local output_file="$1"

    print_info "Copying binary to $output_file"

    # Create firmware directory if it doesn't exist
    mkdir -p firmware

    # Copy the binary
    cp "${BUILD_DIR}/${BINARY_NAME}" "$output_file"

    print_success "Binary copied to $output_file"
}

# Function to write version to version file
write_version_file() {
    local version_string="$1"
    local version_file="$2"

    print_info "Writing version to $version_file"

    echo "$version_string" > "$version_file"

    print_success "Version written to $version_file"
}

# Function to commit and push to git
git_commit_push() {
    local file_to_commit="$1"
    local commit_message="$2"

    print_info "Committing and pushing to git..."

    git add "$file_to_commit"

    # Check if there are changes to commit
    if git diff --cached --quiet; then
        print_warning "No changes to commit for $file_to_commit"
    else
        git commit -m "$commit_message"
        git push
        print_success "Changes committed and pushed"
    fi
}

# Function to build normal version
build_normal() {
    local version_date=$(get_version_date)
    local version_string="${version_date}_${NORMAL_VERSION_SUFFIX}"
    local output_file="firmware/update.bin"
    local version_file="version.txt"

    echo ""
    print_info "========================================="
    print_info "Building NORMAL firmware version"
    print_info "Version: $version_string"
    print_info "========================================="
    echo ""

    # Update version in config.h
    update_version "$version_string"

    # Clean and compile
    clean_build
    compile_firmware

    # Copy binary
    copy_binary "$output_file"

    # Write version file
    write_version_file "$version_string" "$version_file"

    # Clean build directory
    clean_build

    # Git operations - commit both binary and version file
    if [ "$NO_COMMIT" = false ]; then
        git add "$output_file" "$version_file"
        if git diff --cached --quiet; then
            print_warning "No changes to commit for normal version"
        else
            git commit -m "Updated update file (update.bin) - Version: $version_string"
            if [ "$NO_PUSH" = false ]; then
                git push
                print_success "Changes committed and pushed"
            else
                print_success "Changes committed (not pushed, --no-push was specified)"
            fi
        fi
    else
        print_info "Skipping git operations (--no-commit was specified)"
    fi

    # Clean up backup file
    rm -f "${CONFIG_FILE}.bak"

    print_success "Normal firmware build completed!"
}

# Function to build beta version
build_beta() {
    local version_date=$(get_version_date)
    local version_string="${version_date}_${BETA_VERSION_SUFFIX}"
    local output_file="firmware/update_beta.bin"
    local version_file="version_beta.txt"

    echo ""
    print_info "========================================="
    print_info "Building BETA firmware version"
    print_info "Version: $version_string"
    print_info "========================================="
    echo ""

    # Update version in config.h
    update_version "$version_string"

    # Clean and compile
    clean_build
    compile_firmware

    # Copy binary
    copy_binary "$output_file"

    # Write version file
    write_version_file "$version_string" "$version_file"

    # Clean build directory
    clean_build

    # Git operations - commit both binary and version file
    if [ "$NO_COMMIT" = false ]; then
        git add "$output_file" "$version_file"
        if git diff --cached --quiet; then
            print_warning "No changes to commit for beta version"
        else
            git commit -m "Updated update file (update_beta.bin) - Version: $version_string"
            if [ "$NO_PUSH" = false ]; then
                git push
                print_success "Changes committed and pushed"
            else
                print_success "Changes committed (not pushed, --no-push was specified)"
            fi
        fi
    else
        print_info "Skipping git operations (--no-commit was specified)"
    fi

    # Clean up backup file
    rm -f "${CONFIG_FILE}.bak"

    print_success "Beta firmware build completed!"
}

# Function to build both versions
build_both() {
    print_info "Building both normal and beta versions..."
    echo ""

    build_normal
    echo ""
    build_beta

    # Clean up backup file
    rm -f "${CONFIG_FILE}.bak"

    echo ""
    print_success "========================================="
    print_success "All builds completed successfully!"
    print_success "========================================="
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [normal|beta|both] [--no-push|--no-commit]"
    echo ""
    echo "Options:"
    echo "  normal      - Build normal/stable firmware version"
    echo "  beta        - Build beta firmware version"
    echo "  both        - Build both normal and beta versions (default)"
    echo "  --no-push   - Build and commit locally, but don't push to GitHub"
    echo "  --no-commit - Build and copy files only, skip all git operations"
    echo ""
    echo "Examples:"
    echo "  $0                    # Build both versions and push"
    echo "  $0 normal             # Build only normal version and push"
    echo "  $0 beta               # Build only beta version and push"
    echo "  $0 both --no-push     # Build both, commit locally, don't push"
    echo "  $0 normal --no-commit # Build normal, just copy files, no git"
    echo ""
}

# Trap to ensure cleanup on error
trap 'print_error "Build failed!"; exit 1' ERR

# Parse command line arguments
BUILD_TYPE="${1:-both}"

# Check for flags in any position
for arg in "$@"; do
    if [ "$arg" = "--no-push" ]; then
        NO_PUSH=true
        print_info "Running in no-push mode: builds will be committed locally but not pushed to GitHub"
        echo ""
    elif [ "$arg" = "--no-commit" ]; then
        NO_COMMIT=true
        print_info "Running in no-commit mode: builds will only compile and copy files, no git operations"
        echo ""
    fi
done

# Main script logic
case "$BUILD_TYPE" in
    normal)
        build_normal
        ;;
    beta)
        build_beta
        ;;
    both)
        build_both
        ;;
    -h|--help|help)
        show_usage
        ;;
    --no-push|--no-commit)
        # If flag is the first argument, default to both
        build_both
        ;;
    *)
        print_error "Invalid option: $BUILD_TYPE"
        echo ""
        show_usage
        exit 1
        ;;
esac

exit 0
