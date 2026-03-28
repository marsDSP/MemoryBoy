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
