"""Puts the Game Master's authoring chrome onto a player-facing intel visual.

THE EDITOR IS THE THING ITSELF. A Game Master who rewrites a letter should be
looking at the letter, not at a form with three fields that claims to be one.
The old MCF_Intel_EditorMenu is a form; it stays for the views that have no
visual yet (DOCUMENT, MAP) and is not used by these two any more.

So each visual carries both halves: the read-only widgets a player sees, and
an edit box sitting in exactly the same box for the Game Master. One of each
pair is shown -- see ShowPaperAuthor in MCF_Intel_ShellMenu.

The tool column sits on the dim area to the LEFT of the sheet, which is empty
in both layouts. It does not fight the player's PREV / LOG / NEXT / PUT DOWN
row at the bottom, and it reads like a tool palette beside a page.

Idempotent: it removes a block it wrote before and writes the current one, so
re-running it after changing the anchors below is safe.

Run from anywhere:  python tools/add_intel_author_chrome.py
"""

import io
import re

BEGIN = "  // ---- MCF AUTHOR CHROME BEGIN (tools/add_intel_author_chrome.py) ----"
END = "  // ---- MCF AUTHOR CHROME END ----"

# One entry per visual: where its read-only widgets sit, so the edit box for
# each lands in the same box, and a GUID stem with room to grow.
VISUALS = [
    {
        "path": r"G:\MCF\addons\MCF_Ops\UI\layouts\MCF\MCF_IntelPaper.layout",
        "stem": "6A1C4F0B39D320",
        "first": 0x40,
        "name":    (0.250, 0.020, 0.750, 0.058),
        "heading": (0.345, 0.105, 0.645, 0.150),
        "stamp":   (0.345, 0.152, 0.645, 0.182),
        "body":    (0.340, 0.195, 0.650, 0.830),
        "head_size": 22, "head_ink": "0.13 0.12 0.16 1",
        "stamp_size": 14, "stamp_ink": "0.34 0.31 0.28 1",
        "body_size": 15, "body_ink": "0.13 0.12 0.16 1", "body_spacing": 22,
    },
    {
        "path": r"G:\MCF\addons\MCF_Ops\UI\layouts\MCF\MCF_IntelNotepad.layout",
        "stem": "6A1C4F0B39D330",
        "first": 0x60,
        "name":    (0.250, 0.026, 0.750, 0.064),
        "heading": (0.372, 0.108, 0.660, 0.152),
        "stamp":   (0.372, 0.154, 0.660, 0.182),
        "body":    (0.368, 0.196, 0.664, 0.820),
        "head_size": 20, "head_ink": "0.13 0.12 0.16 1",
        "stamp_size": 13, "stamp_ink": "0.34 0.31 0.28 1",
        "body_size": 14, "body_ink": "0.13 0.12 0.16 1", "body_spacing": 21,
    },
]

# The tool column, left of the sheet. Both visuals put the sheet at x > 0.31.
TOOLS = [
    ("ButtonType",      "TYPE",    0.100, 0.140),
    ("ButtonPage",      "PAGE",    0.146, 0.186),
    ("ButtonAddPage",   "+ PAGE",  0.206, 0.246),
    ("ButtonDropPage",  "- PAGE",  0.252, 0.292),
    ("ButtonSaveIntel", "SAVE",    0.312, 0.352),
]
TOOL_X = (0.150, 0.292)

BUTTON_BASE = '"{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout"'


class Guids:
    def __init__(self, stem, first):
        self.stem = stem
        self.n = first

    def __call__(self):
        out = "%s%02X" % (self.stem, self.n)
        self.n += 1
        return out


def slot(a, g, kind="FrameWidgetSlot"):
    return (
        '   Slot %s "{%s}" {\n'
        "    Anchor %.3f %.3f %.3f %.3f\n"
        "    PositionX 0\n    OffsetLeft 0\n    PositionY 0\n    OffsetTop 0\n"
        "    SizeX 0\n    OffsetRight 0\n    SizeY 0\n    OffsetBottom 0\n"
        "   }\n" % (kind, g, a[0], a[1], a[2], a[3])
    )


def editbox(name, a, g, size, ink, multiline=False, spacing=None):
    """A field the Game Master types in.

    MULTILINE IS A DIFFERENT CLASS, not a property: MultilineEditBoxWidget and
    EditBoxWidget share no parent, and a single-line box swallows Return and
    runs the text off to the right forever. A page body needs the former; a
    heading, a date and an object name want the latter.
    """
    cls = "MultilineEditBoxWidgetClass" if multiline else "EditBoxWidgetClass"
    s = '  %s "{%s}" {\n' % (cls, g())
    s += '   Name "%s"\n' % name
    s += slot(a, g())
    s += '   "Is Visible" 0\n'
    s += "   Clipping True\n"
    s += "   components {\n"
    # The event handler is what gives a field its focus and confirm events --
    # vanilla's SCR_EditBoxComponent finds exactly this on the widget.
    s += '    SCR_EventHandlerComponent "{%s}" {\n    }\n' % g()
    if not multiline:
        # The engine's own comment on SCR_EditBoxComponent: the filter "does
        # not work on MultilineEditBoxWidget".
        s += '    EditBoxFilterComponent "{%s}" {\n     m_iCharacterLimit 4000\n    }\n' % g()
    s += "   }\n"
    s += "   style blank\n"
    s += '   Text ""\n'
    s += '   "Font Size" %d\n' % size
    s += '   "Min Font Size" %d\n' % size
    if spacing:
        s += '   "Line Spacing" %d\n' % spacing
    # A widget draws white when it is told nothing, and every one of these
    # sits on cream paper.
    s += "   Color %s\n" % ink
    s += "  }\n"
    return s


def button(name, label, a, g):
    s = '  ButtonWidgetClass "{%s}" : %s {\n' % (g(), BUTTON_BASE)
    s += '   Name "%s"\n' % name
    s += slot(a, g())
    s += '   "Is Visible" 0\n'
    s += "   components {\n"
    s += '    SCR_ButtonTextComponent "{%s}" {\n' % g()
    s += "     m_bCanBeToggled 0\n"
    s += '     m_sText "%s"\n' % label
    s += "    }\n"
    s += "   }\n"
    s += "  }\n"
    return s


def note(name, a, g, text=""):
    s = '  RichTextWidgetClass "{%s}" {\n' % g()
    s += '   Name "%s"\n' % name
    s += slot(a, g())
    s += '   "Is Visible" 0\n'
    s += "   Clipping Ancestor\n   Wrap 1\n"
    s += '   Text "%s"\n' % text
    s += '   "Font Size" 12\n   "Min Font Size" 12\n   "Line Spacing" 17\n'
    s += "   Color 0.62 0.60 0.56 1\n"
    s += "  }\n"
    return s


def block(v):
    g = Guids(v["stem"], v["first"])
    s = BEGIN + "\n"
    s += editbox("NameEdit", v["name"], g, 18, "1 1 1 1")
    s += editbox("HeadingEdit", v["heading"], g, v["head_size"], v["head_ink"])
    s += editbox("StampEdit", v["stamp"], g, v["stamp_size"], v["stamp_ink"])
    s += editbox("BodyEdit", v["body"], g, v["body_size"], v["body_ink"],
                 multiline=True, spacing=v["body_spacing"])

    for name, label, y0, y1 in TOOLS:
        s += button(name, label, (TOOL_X[0], y0, TOOL_X[1], y1), g)

    s += note("AuthorNote", (TOOL_X[0], 0.364, TOOL_X[1], 0.470), g,
              "TYPE to write on the page, PAGE to see it as the player will. "
              "SAVE writes it to the object.")
    s += END + "\n"
    return s


def apply(v):
    with io.open(v["path"], "r", encoding="utf-8", newline="") as f:
        text = f.read().replace("\r\n", "\n")

    # Take out anything this tool wrote before, so re-running replaces rather
    # than stacks.
    text = re.sub(re.escape(BEGIN) + r"[\s\S]*?" + re.escape(END) + r"\n", "", text)

    # In before the root frame's final two closing braces, so the new widgets
    # are children of rootFrame like everything else.
    cut = text.rstrip().rfind("\n }\n}")
    if cut < 0:
        return v["path"] + "  FAULT: root frame not found"

    text = text[:cut + 1] + block(v) + text[cut + 1:]

    with io.open(v["path"], "w", encoding="utf-8", newline="") as f:
        f.write(text)

    with io.open(v["path"], "r", encoding="utf-8", newline="") as f:
        back = f.read()

    names = re.findall(r'Name "([^"]+)"', back)
    dupes = sorted(set(n for n in names if names.count(n) > 1 and n != "Background"))
    if dupes:
        return v["path"] + "  FAULT duplicate names: " + ", ".join(dupes)

    if "BodyEdit" not in back:
        return v["path"] + "  FAULT: readback"

    return "%s  %d widget names, no duplicates" % (v["path"], len(names))


for visual in VISUALS:
    print(apply(visual))
