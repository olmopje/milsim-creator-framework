"""Structural check on a device profile config.

WHY THIS EXISTS. A single missing `}` in MCF_DeviceProfiles.conf does not
fail the load -- the parser recovers at the wrong nesting level and reports
`Unknown keyword/data 'MCF_Device_App'` once per block it then cannot place.
On the laptop that silently cost seven of eight apps: no mail, no messages,
no contacts, no launcher entries and no taskbar pins, with nothing in the
game that said why. The braces balanced; only the nesting was wrong.

Run it after editing a profile config by hand.
"""

import io
import re
import sys

PATHS = [r"G:\MCF\addons\MCF_Ops\Configs\Devices\MCF_DeviceProfiles.conf"]

# What may appear at which depth. Depth 0 is the file, 1 the library's
# m_aProfiles, 2 a profile, 3 its m_aApps, 4 an app, 5 its m_aItems.
EXPECT = {
    "MCF_Device_Profile": 2,
    "MCF_Device_App": 4,
    "MCF_Device_Item": 6,
}


def scan(path):
    with io.open(path, "r", encoding="utf-8", newline="") as f:
        text = f.read().replace("\r\n", "\n")

    depth = 0
    line = 1
    i = 0
    n = len(text)
    faults = []
    counts = {k: 0 for k in EXPECT}
    word = ""

    while i < n:
        c = text[i]

        if c == "\n":
            line += 1
            word = ""
            i += 1
            continue

        # A quoted string is opaque: a GUID is written "{...}" and those
        # braces are not structure.
        if c == '"':
            i += 1
            while i < n and text[i] != '"':
                if text[i] == "\n":
                    line += 1
                i += 1
            i += 1
            continue

        if c.isalnum() or c == "_":
            word += c
            i += 1
            continue

        if word in EXPECT:
            counts[word] += 1
            if depth != EXPECT[word]:
                faults.append("line %d: %s at depth %d, expected %d"
                              % (line, word, depth, EXPECT[word]))
        word = ""

        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth < 0:
                faults.append("line %d: one } too many" % line)
                depth = 0

        i += 1

    if depth != 0:
        faults.append("end of file at depth %d -- %d closing brace(s) missing" % (depth, depth))

    return counts, faults


bad = False
for path in PATHS:
    counts, faults = scan(path)
    print(path)
    print("  " + ", ".join("%s x%d" % (k, v) for k, v in counts.items()))
    if faults:
        bad = True
        for f in faults:
            print("  FAULT " + f)
    else:
        print("  structure OK")

sys.exit(1 if bad else 0)
