#!/usr/bin/env python3
# Convert include guards to #pragma once
# Thomas Perl; 2016-12-04

import sys
import re
import shutil

for filename in sys.argv[1:]:
    d = open(filename).read()
    if '#pragma once' in d:
        print('Already converted:', filename)
        continue

    matches = re.findall('(#ifndef (.*)\n#define \\2\n)(?:.|\n)*(\n#endif(?:.*\\2.*)?\s*)$', d)
    if not matches:
        print('Could not find matches:', filename)

    for header, _, footer in matches:
        print('Converting:', filename)
        shutil.copy(filename, filename + '.bak')
        # Remove any trailing whitespace, but add newline at end of file
        d = '#pragma once\n' + d[:-len(footer)].rstrip().replace(header, '') + '\n'
        open(filename, 'w').write(d)
