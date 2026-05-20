#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR/../.."

Tools/premake5 xcode4

# Fix SKIP_INSTALL for proper app archiving
# Premake generates array values from multiple xcodebuildsettings calls;
# Xcode needs scalar values. Static libs = YES, Editor app = NO.
python3 -c "
import re, glob

for pbx in glob.glob('**/project.pbxproj', recursive=True):
    with open(pbx, 'r') as f:
        content = f.read()

    original = content

    if 'LumosEditor' in pbx:
        # App target: SKIP_INSTALL = NO, INSTALL_PATH = /Applications
        content = re.sub(r'SKIP_INSTALL = \(\s*YES,?\s*\);', 'SKIP_INSTALL = NO;', content)
        content = re.sub(r'SKIP_INSTALL = YES;', 'SKIP_INSTALL = NO;', content)
        content = re.sub(r'INSTALL_PATH = .*?;', 'INSTALL_PATH = \"/Applications\";', content)
    else:
        # Static lib targets: fix array form to scalar YES
        content = re.sub(r'SKIP_INSTALL = \(\s*YES,?\s*\);', 'SKIP_INSTALL = YES;', content)

    if content != original:
        with open(pbx, 'w') as f:
            f.write(content)
        print(f'  Patched {pbx}')
"

echo "Xcode project generation complete."
