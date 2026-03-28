#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"

readonly DEFAULT_JUCE_SUBMODULE_URL="https://github.com/marsDSP/JUCE.git"
readonly DEFAULT_XSIMD_SUBMODULE_URL="https://github.com/marsDSP/xsimd.git"

log()
{
    printf '[bootstrap] %s\n' "$*"
}

fail()
{
    printf '[bootstrap] ERROR: %s\n' "$*" >&2
    exit 1
}

require_command()
{
    local command_name="$1"

    command -v "${command_name}" >/dev/null 2>&1 || fail "Required command '${command_name}' was not found."
}

enter_project_root()
{
    cd -- "${PROJECT_ROOT}"
}

project_identifier_from_name()
{
    local input="$1"
    local output=""
    local capitalize_next=1
    local index
    local character

    for ((index = 0; index < ${#input}; ++index)); do
        character="${input:index:1}"

        if [[ "${character}" =~ [[:alnum:]] ]]; then
            if (( capitalize_next )) && [[ "${character}" =~ [[:alpha:]] ]]; then
                output+="$(printf '%s' "${character}" | tr '[:lower:]' '[:upper:]')"
            else
                output+="${character}"
            fi

            capitalize_next=0
        else
            capitalize_next=1
        fi
    done

    if [ -z "${output}" ]; then
        output="Project"
    fi

    if [[ ! "${output:0:1}" =~ [[:alpha:]_] ]]; then
        output="Project${output}"
    fi

    printf '%s' "${output}"
}

plugin_code_from_identifier()
{
    local identifier="$1"
    local sanitized

    sanitized="$(printf '%s' "${identifier}" | tr -cd '[:alnum:]' | tr '[:lower:]' '[:upper:]')"
    sanitized="${sanitized}XXXX"

    printf '%.4s' "${sanitized}"
}

directory_is_non_empty()
{
    local directory_path="$1"

    [ -d "${directory_path}" ] && [ -n "$(ls -A "${directory_path}" 2>/dev/null)" ]
}

ensure_git_repository()
{
    if ! git -C "${PROJECT_ROOT}" rev-parse --show-toplevel >/dev/null 2>&1; then
        log "Initializing git repository in ${PROJECT_ROOT}"
        git -C "${PROJECT_ROOT}" init >/dev/null
    fi
}

ensure_submodule()
{
    local name="$1"
    local path="$2"
    local url="$3"
    local absolute_path="${PROJECT_ROOT}/${path}"
    local parent_directory

    parent_directory="$(dirname -- "${path}")"

    if directory_is_non_empty "${absolute_path}" && [ ! -f "${absolute_path}/.git" ] && [ ! -d "${absolute_path}/.git" ]; then
        fail "Cannot configure submodule at '${path}' because the path already exists and is not a git submodule."
    fi

    mkdir -p "${PROJECT_ROOT}/${parent_directory}"

    (
        cd -- "${PROJECT_ROOT}"

        if git config -f .gitmodules --get "submodule.${name}.url" >/dev/null 2>&1; then
            git config -f .gitmodules "submodule.${name}.path" "${path}"
            git config -f .gitmodules "submodule.${name}.url" "${url}"
            git submodule sync -- "${path}" >/dev/null
        elif [ ! -f "${absolute_path}/.git" ] && [ ! -d "${absolute_path}/.git" ]; then
            log "Adding submodule ${path} from ${parent_directory}"
            git -c protocol.file.allow=always submodule add "${url}" "${path}" >/dev/null
        fi

        log "Updating submodule ${path}"
        git -c protocol.file.allow=always submodule update --init --recursive "${path}" >/dev/null
    )
}

create_directories()
{
    local directories=(
        "assets"
        "docs"
        "libs"
        "scripts/bash"
        "scripts/py3"
        "source"
        "source/dsp"
        "source/math"
        "source/utils"
        "source/utils/helpers"
        "tests"
    )
    local directory_path

    for directory_path in "${directories[@]}"; do
        mkdir -p "${PROJECT_ROOT}/${directory_path}"
    done
}

create_virtualenv()
{
    local venv_path="${PROJECT_ROOT}/.venv"

    if [ ! -d "${venv_path}" ]; then
        log "Creating Python virtual environment"
        python3 -m venv "${venv_path}"
    else
        log "Python virtual environment already exists"
    fi
}

write_gitignore()
{
    cat > "${PROJECT_ROOT}/.gitignore" <<'EOF'
CMakeLists.txt.user
CMakeCache.txt
CMakeFiles
CMakeScripts
Testing
Makefile
cmake_install.cmake
install_manifest.txt
compile_commands.json
CTestTestfile.cmake
_deps
CMakeUserPresets.json

# Covers JetBrains IDEs: IntelliJ, GoLand, RubyMine, PhpStorm, AppCode, PyCharm, CLion, Android Studio, WebStorm and Rider
# Reference: https://intellij-support.jetbrains.com/hc/en-us/articles/206544839

# User-specific stuff
.idea/**/workspace.xml
.idea/**/tasks.xml
.idea/**/usage.statistics.xml
.idea/**/dictionaries
.idea/**/shelf

# AWS User-specific
.idea/**/aws.xml

# Generated files
.idea/**/contentModel.xml

# Sensitive or high-churn files
.idea/**/dataSources/
.idea/**/dataSources.ids
.idea/**/dataSources.local.xml
.idea/**/sqlDataSources.xml
.idea/**/dynamic.xml
.idea/**/uiDesigner.xml
.idea/**/dbnavigator.xml

# Gradle
.idea/**/gradle.xml
.idea/**/libraries

# Gradle and Maven with auto-import
# When using Gradle or Maven with auto-import, you should exclude module files,
# since they will be recreated, and may cause churn.  Uncomment if using
# auto-import.
# .idea/artifacts
# .idea/compiler.xml
# .idea/jarRepositories.xml
# .idea/modules.xml
# .idea/*.iml
# .idea/modules
# *.iml
# *.ipr

# CMake
cmake-build-*/

# Mongo Explorer plugin
.idea/**/mongoSettings.xml

# File-based project format
*.iws

# IntelliJ
out/

# mpeltonen/sbt-idea plugin
.idea_modules/

# JIRA plugin
atlassian-ide-plugin.xml

# Cursive Clojure plugin
.idea/replstate.xml

# SonarLint plugin
.idea/sonarlint/
.idea/sonarlint.xml # see https://community.sonarsource.com/t/is-the-file-idea-idea-idea-sonarlint-xml-intended-to-be-under-source-control/121119

# Crashlytics plugin (for Android Studio and IntelliJ)
com_crashlytics_export_strings.xml
crashlytics.properties
crashlytics-build.properties
fabric.properties

# Editor-based HTTP Client
.idea/httpRequests
http-client.private.env.json

# Android studio 3.1+ serialized cache file
.idea/caches/build_file_checksums.ser

# Apifox Helper cache
.idea/.cache/.Apifox_Helper
.idea/ApifoxUploaderProjectSetting.xml

# Github Copilot persisted session migrations, see: https://github.com/microsoft/copilot-intellij-feedback/issues/712#issuecomment-3322062215
.idea/**/copilot.data.migration.*.xml

# CLion
#  JetBrains specific template is maintained in a separate JetBrains.gitignore that can
#  be found at https://github.com/github/gitignore/blob/main/Global/JetBrains.gitignore
#  and can be added to the global gitignore or merged into this file.  For a more nuclear
#  option (not recommended) you can uncomment the following to ignore the entire idea folder.
#cmake-build-*

# Python virtual environment
.venv/

# Doxygen generated documentation
docs/
EOF
}

write_log_dumper()
{
    cat > "${PROJECT_ROOT}/scripts/py3/LogDumper.py" <<'EOF'
import os
import sys
import datetime
import subprocess
import platform

def get_clipboard_content():
    """
    Retrieves text content from the system clipboard.
    Supports MacOS (pbpaste), Windows (PowerShell), and Linux (xclip/xsel).
    """
    system = platform.system()

    try:
        if system == 'Darwin':  # MacOS
            # Preserve existing environment and set LANG to ensure correct decoding
            env = os.environ.copy()
            env['LANG'] = 'en_US.UTF-8'
            return subprocess.check_output(['pbpaste'], env=env).decode('utf-8').rstrip()

        elif system == 'Windows':
            # PowerShell is standard on modern Windows
            # Using shell=True for Windows command execution
            return subprocess.check_output(['powershell', '-command', 'Get-Clipboard'], shell=True).decode('utf-8').rstrip()

        elif system == 'Linux':
            # Try xclip, then xsel
            try:
                return subprocess.check_output(['xclip', '-selection', 'clipboard', '-o']).decode('utf-8').rstrip()
            except FileNotFoundError:
                try:
                    return subprocess.check_output(['xsel', '-ob']).decode('utf-8').rstrip()
                except FileNotFoundError:
                    print("ERROR: Linux clipboard requires 'xclip' or 'xsel' to be installed.")
                    return None
        else:
            print(f"ERROR: Unsupported operating system: {system}")
            return None

    except Exception as e:
        print(f"ERROR: Could not access clipboard on {system}. Details: {e}")
        return None

def get_unique_filepath(base_dir):
    """
    Generates a unique filename based on the current timestamp.
    Ensures no overwriting by appending a counter if necessary.
    """
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    base_name = f"ChatDump_{timestamp}"
    extension = ".md"
    filename = f"{base_name}{extension}"
    filepath = os.path.join(base_dir, filename)

    counter = 1
    while os.path.exists(filepath):
        filename = f"{base_name}_{counter}{extension}"
        filepath = os.path.join(base_dir, filename)
        counter += 1

    return filepath

def main():
    print("--- Auto Log Dumper ---")

    # 1. Resolve directories
    # Script is likely in project/scripts/python3/, we target project/docs/
    script_dir = os.path.dirname(os.path.abspath(__file__))
    docs_dir = os.path.abspath(os.path.join(script_dir, "../../docs"))

    # Ensure target directory exists
    if not os.path.exists(docs_dir):
        try:
            os.makedirs(docs_dir)
            print(f"Created missing directory: {docs_dir}")
        except OSError as e:
            print(f"FATAL: Could not create directory '{docs_dir}'. Reason: {e}")
            sys.exit(1)

    # 2. Get content
    content = get_clipboard_content()

    # Check for empty content (empty string or None or whitespace only)
    if not content or not content.strip():
        print("WARNING: Clipboard is empty or contains only whitespace. Nothing was saved.")
        sys.exit(1)

    # 3. Prepare file
    try:
        filepath = get_unique_filepath(docs_dir)
    except Exception as e:
        print(f"FATAL: Could not generate filepath. Reason: {e}")
        sys.exit(1)

    # 4. Write content
    try:
        header = f"<!-- Dumped on {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')} -->\n\n"
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(header)
            f.write(content)
        print(f"SUCCESS: Clipboard dumped to:\n{filepath}")
    except IOError as e:
        print(f"FATAL: Failed to write to file '{filepath}'. Reason: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
EOF
}

write_plugin_sources()
{
    local project_identifier="$1"
    local processor_class="${project_identifier}Processor"
    local editor_class="${project_identifier}Editor"

    cat > "${PROJECT_ROOT}/source/${processor_class}.h" <<EOF
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
class ${processor_class} final : public juce::AudioProcessor
{
public:
    //==============================================================================
    ${processor_class}();
    ~${processor_class}() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (${processor_class})
};
EOF

    cat > "${PROJECT_ROOT}/source/${processor_class}.cpp" <<EOF
#include "${processor_class}.h"
#include "${editor_class}.h"

//==============================================================================
${processor_class}::${processor_class}()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

${processor_class}::~${processor_class}()
{
}

//==============================================================================
const juce::String ${processor_class}::getName() const
{
    return JucePlugin_Name;
}

bool ${processor_class}::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ${processor_class}::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ${processor_class}::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ${processor_class}::getTailLengthSeconds() const
{
    return 0.0;
}

int ${processor_class}::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ${processor_class}::getCurrentProgram()
{
    return 0;
}

void ${processor_class}::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ${processor_class}::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void ${processor_class}::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void ${processor_class}::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void ${processor_class}::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool ${processor_class}::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void ${processor_class}::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer, midiMessages);
    juce::ScopedNoDenormals noDenormals;
}

//==============================================================================
bool ${processor_class}::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ${processor_class}::createEditor()
{
    return new ${editor_class} (*this);
}

//==============================================================================
void ${processor_class}::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void ${processor_class}::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ${processor_class}();
}
EOF

    cat > "${PROJECT_ROOT}/source/${editor_class}.h" <<EOF
#pragma once

#include "${processor_class}.h"

//==============================================================================
class ${editor_class} final : public juce::AudioProcessorEditor
{
public:
    explicit ${editor_class} (${processor_class}&);
    ~${editor_class}() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ${processor_class}& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (${editor_class})
};
EOF

    cat > "${PROJECT_ROOT}/source/${editor_class}.cpp" <<EOF
#include "${processor_class}.h"
#include "${editor_class}.h"

//==============================================================================
${editor_class}::${editor_class} (${processor_class}& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (400, 300);
}

${editor_class}::~${editor_class}()
{
}

//==============================================================================
void ${editor_class}::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void ${editor_class}::resized()
{
}
EOF
}

write_cmake_lists()
{
    local project_identifier="$1"
    local plugin_code="$2"
    local processor_header="source/${project_identifier}Processor.h"

    cat > "${PROJECT_ROOT}/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.23.1)

# Suppress CMP0177 warning about install() DESTINATION paths normalization
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

# General setup
set(PROJECT_NAME "${project_identifier}")
set(CURRENT_VERSION "0.1.0")
set(PRODUCT_NAME "${project_identifier}")
set(COMPANY_NAME "MarsDSP")
set(BUNDLE_ID "com.marsdsp.${project_identifier}")
set(FORMATS Standalone VST3)
if(APPLE)
    list(APPEND FORMATS AU)
endif()

# Keep CMake project version in sync with CURRENT_VERSION defined above
project(\${PROJECT_NAME} VERSION \${CURRENT_VERSION})

add_subdirectory(libs/JUCE)

# Project configuration
set(CMAKE_CXX_STANDARD 23)

# Find Python 3 for scripting support
set(Python3_FIND_VIRTUALENV STANDARD)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
message(STATUS "Found Python3: \${Python3_EXECUTABLE}")

# Add a target to verify Python setup
add_custom_target(CheckPython
    COMMAND \${Python3_EXECUTABLE} "\${CMAKE_CURRENT_SOURCE_DIR}/scripts/py3/CheckPy.py"
    COMMENT "Checking Python configuration..."
    VERBATIM
)

# Run LogDumper
add_custom_target(Run_LogDumper
    COMMAND \${Python3_EXECUTABLE} "\${CMAKE_CURRENT_SOURCE_DIR}/scripts/py3/LogDumper.py"
    COMMENT "Running LogDumper.py..."
    VERBATIM
)

# Run CheckCMake
add_custom_target(Run_CheckCMake
    COMMAND \${Python3_EXECUTABLE} "\${CMAKE_CURRENT_SOURCE_DIR}/scripts/py3/CheckCMake.py"
    WORKING_DIRECTORY "\${CMAKE_CURRENT_SOURCE_DIR}/scripts/py3"
    COMMENT "Running CheckCMake.py..."
    VERBATIM
)

# macOS-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Architectures" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=arm64] "11.0" CACHE STRING "arm 64 minimum deployment target" FORCE)
endif()

# Disable all warnings on non-MSVC compilers
if(NOT MSVC)
    add_definitions(-w)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

# Define the plugin
juce_add_plugin("\${PROJECT_NAME}"
        COMPANY_NAME "\${COMPANY_NAME}"
        BUNDLE_ID "\${BUNDLE_ID}"
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE MDSP
        PLUGIN_CODE ${plugin_code}
        FORMATS \${FORMATS}
        PRODUCT_NAME "\${PRODUCT_NAME}"
        NEEDS_WEB_BROWSER FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        IS_SYNTH FALSE
)

juce_generate_juce_header("\${PROJECT_NAME}")

set(GENERATED_DIR "\${CMAKE_CURRENT_BINARY_DIR}/\${PROJECT_NAME}_artefacts/JuceLibraryCode")

add_compile_definitions(JUCE_PROJECT_NAME="\${PROJECT_NAME}")

# Define SharedCode as an INTERFACE library (no sources required)
add_library(SharedCode INTERFACE
        ${processor_header})

# Set compile features for SharedCode
target_compile_features(SharedCode INTERFACE cxx_std_23)

# Include directories and compile definitions for SharedCode
cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED)

target_include_directories(SharedCode INTERFACE
        "\${CMAKE_CURRENT_SOURCE_DIR}/source"
        "\${CMAKE_CURRENT_SOURCE_DIR}/libs"
        "\${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/modules"
        "\${CMAKE_CURRENT_SOURCE_DIR}/libs/xsimd/include"
        "\${GENERATED_DIR}"
)

target_link_libraries(SharedCode INTERFACE Boost::headers)

# Set JUCE flags for SharedCode
target_compile_definitions(SharedCode INTERFACE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        CMAKE_BUILD_TYPE="\${CMAKE_BUILD_TYPE}"
        VERSION="\${CURRENT_VERSION}"
        JUCE_DISPLAY_SPLASH_SCREEN=0
        PRODUCT_NAME_WITHOUT_VERSION="\${PRODUCT_NAME}"
)

# Add sources to the main project
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
        "\${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
        "\${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
        "\${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp"
)

# Sources to main project
target_sources("\${PROJECT_NAME}" PRIVATE \${SourceFiles})

# Assets setup
file(GLOB_RECURSE Assets CONFIGURE_DEPENDS "\${CMAKE_CURRENT_SOURCE_DIR}/assets/*.*")

if(Assets)
    juce_add_binary_data(AudioPluginData SOURCES \${Assets})
    target_link_libraries("\${PROJECT_NAME}" PRIVATE AudioPluginData)
endif()

# Link libraries to main project
target_link_libraries("\${PROJECT_NAME}" PUBLIC
        SharedCode
        PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Ensure the main project knows where its sources are
target_include_directories("\${PROJECT_NAME}" PUBLIC
        "\${CMAKE_CURRENT_SOURCE_DIR}/source"
        "\${GENERATED_DIR}"
)

if(APPLE)
    target_compile_options("\${PROJECT_NAME}" PRIVATE "-fno-objc-arc")

    file(GLOB_RECURSE JuceObjCSourceFiles "\${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/modules/*.mm")
    set_source_files_properties(
        \${JuceObjCSourceFiles}
        PROPERTIES COMPILE_FLAGS "-fno-objc-arc"
    )
endif()

# Doxygen documentation generation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYFILE "\${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
    add_custom_target(docs
        COMMAND \${DOXYGEN_EXECUTABLE} \${DOXYFILE}
        WORKING_DIRECTORY \${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
EOF
}

main()
{
    local project_directory_name
    local project_identifier
    local plugin_code
    local juce_submodule_url
    local xsimd_submodule_url

    require_command git
    require_command python3

    enter_project_root

    project_directory_name="$(basename -- "${PWD}")"
    project_identifier="$(project_identifier_from_name "${project_directory_name}")"
    plugin_code="$(plugin_code_from_identifier "${project_identifier}")"
    juce_submodule_url="${JUCE_SUBMODULE_URL:-${DEFAULT_JUCE_SUBMODULE_URL}}"
    xsimd_submodule_url="${XSIMD_SUBMODULE_URL:-${DEFAULT_XSIMD_SUBMODULE_URL}}"

    log "Bootstrapping project scaffold for ${project_identifier} in ${PROJECT_ROOT}"

    create_directories
    ensure_git_repository
    ensure_submodule "libs/xsimd" "libs/xsimd" "${xsimd_submodule_url}"
    ensure_submodule "libs/JUCE" "libs/JUCE" "${juce_submodule_url}"
    create_virtualenv
    write_gitignore
    write_log_dumper
    write_plugin_sources "${project_identifier}"
    write_cmake_lists "${project_identifier}" "${plugin_code}"

    log "Bootstrap complete. Generated files for ${project_identifier}."
}

main "$@"