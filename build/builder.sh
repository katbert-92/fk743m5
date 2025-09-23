#!/usr/bin/env bash

BUILD_DIR=build
UTILS_DIR=utils
RESULT_DIR=debug

set -e # Exit on error

# Function to log errors and exit
log_error() {
    echo "Error: $1"
    exit 1
}

run_step() {
    local description="$1"
    local cmd="$2"

    echo
    echo "▶️  Run $cmd"
    if eval "$cmd"; then
        echo "✅ $description completed successfully"
    else
        echo "❌ $description failed"
    fi
}

# Detect OS and architecture
if [ "$OS" == "Windows_NT" ]; then
    OS_NAME="Windows_NT"
    HW=$(echo "$PROCESSOR_ARCHITECTURE")
    CPU_ARCH=$HW
else
    OS_NAME=$(uname -s)
    HW=$(uname -m)
    CPU_ARCH=$HW
fi

echo "-----------------------------------------------------------------"
echo "Task started"
echo "Use $OS_NAME as OS, $HW as hardware, $CPU_ARCH as CPU architecture"

# Get shell path
SHELL_PATH="$BASH"
echo "Use $SHELL_PATH as shell"

# Determine virtual environment details
case "$OS_NAME" in
    "Darwin")
        VENV_NAME="virtualenv"
        VENV_SRC="bin"
        PYTHON_NAME="python3"
        ;;
    "Linux")
        VENV_NAME="venv"
        VENV_SRC="bin"
        PYTHON_NAME="python3"
        ;;
    "Windows_NT")
        VENV_NAME="venv"
        VENV_SRC="Scripts"
        PYTHON_NAME="python"
        ;;
    *)
        log_error "Unsupported OS: $OS_NAME"
        ;;
esac

# Check for Python version and availability
if ! command -v "$PYTHON_NAME" &> /dev/null; then
    log_error "Python is not installed. Please install $PYTHON_NAME"
fi

# Set up virtual environment if not present
if [ ! -d ".venv" ]; then
    echo ".venv not found. Creating virtual environment..."
    "$PYTHON_NAME" -m "$VENV_NAME" .venv || log_error "Failed to create virtual environment"
    ".venv/$VENV_SRC/$PYTHON_NAME" -m pip install --upgrade pip || log_error "Failed to upgrade pip"
    ".venv/$VENV_SRC/pip" install -r "requirements.txt" || log_error "Failed to install dependencies"
else
    echo "Skipping virtual environment setup: .venv exists"
fi

PYTHON_VER=$($PYTHON_NAME --version 2>&1)
echo "Use $PYTHON_VER"

# Update git submodules
echo "Updating submodules..."
git submodule sync
git submodule update --init --depth=1 || log_error "Failed to update submodules"

venv_python=".venv/$VENV_SRC/$PYTHON_NAME"

# Run build script
python_build_script="$BUILD_DIR/fw_builder.py"
build_cmd="$venv_python $python_build_script $*"
run_step "Build" "$build_cmd"

# Run ELF and MAP analysis script
analysis_script="$UTILS_DIR/fw_analyse/fw_footprint_analyse.py"
analysis_elf_file="$RESULT_DIR/fw.elf"
analysis_map_file="$RESULT_DIR/fw.map"
analysis_out_file="$RESULT_DIR/fw_sections.json"
analyze_cmd="$venv_python $analysis_script -e \"$analysis_elf_file\" -m \"$analysis_map_file\" -o \"$analysis_out_file\""
run_step "Firmware analyse" "$analyze_cmd"
