#!/usr/bin/env python3
"""
Generates MCF_IntelDesktop.layout -- the laptop's KDE-shaped shell.

WHY THIS FILE IS GENERATED AND THE PHONE'S WAS NOT. The handset has one screen
at a time; the desktop has ten windows, each with a title bar, three chrome
buttons and a pane of its own, and five of those panes are the same shape. That
is several thousand lines of near-identical blocks, which is exactly the kind of
file a person makes one silent typo in. Change a number here and every window
moves together.

GEOMETRY. Everything inside Screen is a fraction of Screen's own box, and Screen
is placed by the menu every frame at 16:10 -- a laptop is landscape, and that
ratio is the one real difference between this and the handset. Window boxes are
the exception: they are absolute reference units set at runtime, because a
window that can be dragged has no fixed anchor by definition.
"""
import os

OUT = os.environ.get("MCF_LAYOUT_OUT", "UI/layouts/MCF/MCF_IntelDesktop.layout")

# ---------------------------------------------------------------- resources
A = "UI/images/MCF_Desktop/"
ART = {
    "files":     "{6A1C4F0B39D35501}" + A + "icon_files.edds",
    "folder":    "{6A1C4F0B39D35502}" + A + "icon_folder.edds",
    "mail":      "{6A1C4F0B39D35503}" + A + "icon_mail.edds",
    "messages":  "{6A1C4F0B39D35504}" + A + "icon_messages.edds",
    "contacts":  "{6A1C4F0B39D35505}" + A + "icon_contacts.edds",
    "photos":    "{6A1C4F0B39D35506}" + A + "icon_photos.edds",
    "notes":     "{6A1C4F0B39D35507}" + A + "icon_notes.edds",
    "terminal":  "{6A1C4F0B39D35508}" + A + "icon_terminal.edds",
    "settings":  "{6A1C4F0B39D35509}" + A + "icon_settings.edds",
    "phone":     "{6A1C4F0B39D3550A}" + A + "icon_phone.edds",
    "apps":      "{6A1C4F0B39D3550B}" + A + "icon_apps.edds",
    "doc":       "{6A1C4F0B39D3550C}" + A + "icon_doc.edds",
    "drive":     "{6A1C4F0B39D3550D}" + A + "icon_drive.edds",
    "net":       "{6A1C4F0B39D3550E}" + A + "glyph_network.edds",
    "vol":       "{6A1C4F0B39D3550F}" + A + "glyph_volume.edds",
    "batt":      "{6A1C4F0B39D35510}" + A + "glyph_battery.edds",
    "power":     "{6A1C4F0B39D35511}" + A + "glyph_power.edds",
    "search":    "{6A1C4F0B39D35512}" + A + "glyph_search.edds",
    "lock":      "{6A1C4F0B39D35513}" + A + "glyph_lock.edds",
    "chevron":   "{6A1C4F0B39D35514}" + A + "glyph_chevron.edds",
    "add":       "{6A1C4F0B39D35515}" + A + "glyph_add.edds",
    "delete":    "{6A1C4F0B39D35516}" + A + "glyph_delete.edds",
    "edit":      "{6A1C4F0B39D35517}" + A + "glyph_edit.edds",
    "check":     "{6A1C4F0B39D35518}" + A + "glyph_check.edds",
    "addimage":  "{6A1C4F0B39D35519}" + A + "glyph_addimage.edds",
    "min":       "{6A1C4F0B39D3551A}" + A + "glyph_min.edds",
    "max":       "{6A1C4F0B39D3551B}" + A + "glyph_max.edds",
    "restore":   "{6A1C4F0B39D3551C}" + A + "glyph_restore.edds",
    "close":     "{6A1C4F0B39D3551D}" + A + "glyph_close.edds",
    "grip":      "{6A1C4F0B39D3551E}" + A + "glyph_grip.edds",
    "tile":      "{6A1C4F0B39D3551F}" + A + "tile_window.edds",
    "flat":      "{6A1C4F0B39D35520}" + A + "tile_flat.edds",
    "barpanel":  "{6A1C4F0B39D35521}" + A + "bar_panel.edds",
    "bartitle":  "{6A1C4F0B39D35522}" + A + "bar_titlebar.edds",
    "barhead":   "{6A1C4F0B39D35523}" + A + "bar_header.edds",
    "wall":      "{6A1C4F0B39D35524}" + A + "wallpaper_desktop.edds",
    # borrowed from the handset: one circle and one rounded tile for the whole
    # framework, so a radius is never defined twice.
    "circle":    "{6A1C4F0B39D3520B}UI/images/MCF_Phone/circle.edds",
    "rounded":   "{6A1C4F0B39D3520A}UI/images/MCF_Phone/tile_rounded.edds",
}
ROW_LAYOUT    = "{6A1C4F0B39D35600}UI/layouts/MCF/MCF_PhoneRow.layout"
BUBBLE_LAYOUT = "{6A1C4F0B39D3564F}UI/layouts/MCF/MCF_PhoneBubble.layout"
BTN_LAYOUT    = "{6A1C4F0B39D35120}UI/layouts/MCF/MCF_PhoneButton.layout"

# ------------------------------------------------------------------ palette
SURFACE   = "0.157 0.176 0.196 1"
SURFACE_2 = "0.192 0.216 0.239 1"
LINE      = "1 1 1 0.09"
LINE_HARD = "1 1 1 0.16"
ACCENT    = "0.76 0.39 0.08 1"
DIM_INK   = "0.60 0.63 0.67 1"
FAINT_INK = "0.43 0.46 0.50 1"
TERM_BG   = "0.075 0.086 0.098 1"

_n = [0]


def g():
    _n[0] += 1
    return "{6A1C4F0B39E0%04X}" % _n[0]


def slot(a, ind):
    p = " " * ind
    return (
        p + 'Slot FrameWidgetSlot "%s" {\n' % g() +
        p + " Anchor %.4f %.4f %.4f %.4f\n" % a +
        p + " PositionX 0\n" + p + " OffsetLeft 0\n" +
        p + " PositionY 0\n" + p + " OffsetTop 0\n" +
        p + " SizeX 0\n" + p + " OffsetRight 0\n" +
        p + " SizeY 0\n" + p + " OffsetBottom 0\n" +
        p + "}\n"
    )


def img(name, a, ind, colour=None, tex=None, visible=True):
    p = " " * ind
    s = p + 'ImageWidgetClass "%s" {\n' % g()
    s += p + ' Name "%s"\n' % name
    s += slot(a, ind + 1)
    if colour:
        s += p + " Color %s\n" % colour
    if tex:
        s += p + ' Texture "%s"\n' % ART[tex]
        s += p + ' Image ""\n'
    if not visible:
        s += p + ' "Is Visible" 0\n'
    s += p + "}\n"
    return s


def txt(name, a, ind, size, colour=None, align=None, text="", rich=False,
        wrap=False, visible=True, spacing=None):
    p = " " * ind
    cls = "RichTextWidgetClass" if rich else "TextWidgetClass"
    s = p + '%s "%s" {\n' % (cls, g())
    s += p + ' Name "%s"\n' % name
    s += slot(a, ind + 1)
    if rich:
        s += p + " Clipping Ancestor\n"
        if wrap:
            s += p + " Wrap 1\n"
    if not visible:
        s += p + ' "Is Visible" 0\n'
    s += p + ' Text "%s"\n' % text
    s += p + ' "Font Size" %d\n' % size
    s += p + ' "Min Font Size" %d\n' % size
    if spacing:
        s += p + ' "Line Spacing" %d\n' % spacing
    if colour:
        s += p + " Color %s\n" % colour
    if align:
        # NOT `Alignment`. That property is accepted and ignored on a
        # TextWidget -- every centred label on the first build came out hard
        # left. "Text Horizontal Align" is the one the operations board uses
        # and it is 0 left, 1 centre, 2 right.
        s += p + ' "Text Horizontal Align" %d\n' % align
    s += p + "}\n"
    return s


def frame(name, a, ind, body, visible=True):
    p = " " * ind
    s = p + 'FrameWidgetClass "%s" {\n' % g()
    s += p + ' Name "%s"\n' % name
    s += slot(a, ind + 1)
    if not visible:
        s += p + ' "Is Visible" 0\n'
    s += p + " {\n"
    s += body
    s += p + " }\n"
    s += p + "}\n"
    return s


def flatbtn(name, a, ind, icon=None, label=None, icon_px=22, pad_l=10,
            text_l=40, size=12, bg="1 1 1 0", bg_hi="1 1 1 0.10",
            bg_sel=None, tex=None, label_align=None):
    """A blank button with an overlay inside it.

    THE OVERLAY IS NOT OPTIONAL. A ButtonWidget's children sit in
    ButtonWidgetSlot, which aligns rather than positions, so anything that
    needs two children side by side needs an overlay in between. And `style
    blank` is not optional either: without it the button paints a white block,
    which cost an evening on the handset's home bar.
    """
    p = " " * ind
    sel = bg_sel or ACCENT
    s = p + 'ButtonWidgetClass "%s" {\n' % g()
    s += p + ' Name "%s"\n' % name
    s += slot(a, ind + 1)
    s += p + " Clipping True\n"
    s += p + " components {\n"
    s += p + '  SCR_ButtonTextComponent "%s" {\n' % g()
    s += p + "   m_bCanBeToggled 0\n"
    s += p + "   m_BackgroundDefault %s\n" % bg
    s += p + "   m_BackgroundHovered %s\n" % bg_hi
    s += p + "   m_BackgroundSelected %s\n" % sel
    s += p + "   m_BackgroundSelectedHovered %s\n" % sel
    s += p + "   m_BackgroundClicked 1 1 1 0.22\n"
    s += p + '   m_sText ""\n'
    s += p + "  }\n"
    s += p + " }\n"
    s += p + " style blank\n"
    s += p + " {\n"
    s += p + '  OverlayWidgetClass "%s" {\n' % g()
    s += p + '   Name "%sBox"\n' % name
    s += p + '   Slot ButtonWidgetSlot "%s" {\n' % g()
    s += p + "    HorizontalAlign 3\n    VerticalAlign 3\n"
    s += p + "   }\n"
    s += p + "   {\n"
    if tex:
        s += p + '    ImageWidgetClass "%s" {\n' % g()
        s += p + '     Name "Background"\n' 
        s += p + '     Slot OverlayWidgetSlot "%s" {\n' % g()
        s += p + "      HorizontalAlign 3\n      VerticalAlign 3\n"
        s += p + "     }\n"
        s += p + '     Texture "%s"\n' % ART[tex]
        s += p + '     Image ""\n'
        s += p + "    }\n"
    if icon:
        centred = label is None
        s += p + '    ImageWidgetClass "%s" {\n' % g()
        s += p + '     Name "%sIcon"\n' % name
        s += p + '     Slot OverlayWidgetSlot "%s" {\n' % g()
        s += p + "      HorizontalAlign %d\n" % (1 if centred else 0)
        s += p + "      VerticalAlign 1\n"
        if not centred:
            s += p + "      Padding %d 0 0 0\n" % pad_l
        s += p + "     }\n"
        s += p + "     Size %d %d\n" % (icon_px, icon_px)
        s += p + '     Texture "%s"\n' % ART[icon]
        s += p + '     Image ""\n'
        s += p + "    }\n"
    if label is not None:
        s += p + '    TextWidgetClass "%s" {\n' % g()
        s += p + '     Name "%sText"\n' % name
        s += p + '     Slot OverlayWidgetSlot "%s" {\n' % g()
        if label_align == "centre":
            s += p + "      HorizontalAlign 1\n      VerticalAlign 1\n"
            s += p + "      Padding 4 2 4 2\n"
        else:
            s += p + "      HorizontalAlign 0\n      VerticalAlign 1\n"
            s += p + "      Padding %d 0 8 0\n" % text_l
        s += p + "     }\n"
        s += p + '     Text "%s"\n' % label
        s += p + '     "Font Size" %d\n' % size
        s += p + '     "Min Font Size" %d\n' % size
        s += p + "    }\n"
    s += p + "   }\n"
    s += p + "  }\n"
    s += p + " }\n"
    s += p + "}\n"
    return s


def scroll(name, a, ind, inner, kind="vertical", size=12, colour=None):
    """A scroll pane shaped like the handset's, with the width override OFF.

    AllowWidthOverride 1 is what pinned every list on the phone to 200 units
    inside a glass twice that wide, and there is no runtime setter to undo it.
    Align 3 plus the override off lets the content stretch to the viewport.
    """
    p = " " * ind
    s = p + 'ScrollLayoutWidgetClass "%s" {\n' % g()
    s += p + ' Name "%s"\n' % name
    s += slot(a, ind + 1)
    s += p + " components {\n"
    s += p + '  SCR_GamepadScrollComponent "%s" {\n' % g()
    s += p + "  }\n"
    s += p + " }\n"
    s += p + " style Small\n"
    s += p + " {\n"
    s += p + '  SizeLayoutWidgetClass "%s" {\n' % g()
    s += p + '   Name "%sWidth"\n' % name
    s += p + '   Slot AlignableSlot "%s" {\n' % g()
    s += p + "    HorizontalAlign 3\n    VerticalAlign 0\n"
    s += p + "   }\n"
    s += p + "   AllowWidthOverride 0\n"
    s += p + "   WidthOverride 200\n"
    s += p + "   {\n"
    if kind == "vertical":
        s += p + '    VerticalLayoutWidgetClass "%s" {\n' % g()
        s += p + '     Name "%s"\n' % inner
        s += p + '     Slot AlignableSlot "%s" {\n' % g()
        s += p + "      HorizontalAlign 3\n      VerticalAlign 0\n"
        s += p + "     }\n"
        s += p + "     {\n     }\n"
        s += p + "    }\n"
    else:
        s += p + '    RichTextWidgetClass "%s" {\n' % g()
        s += p + '     Name "%s"\n' % inner
        s += p + '     Slot AlignableSlot "%s" {\n' % g()
        s += p + "      HorizontalAlign 3\n      VerticalAlign 0\n"
        s += p + "     }\n"
        s += p + "     Clipping Ancestor\n     Wrap 1\n"
        s += p + '     Text ""\n'
        s += p + '     "Font Size" %d\n' % size
        s += p + '     "Min Font Size" %d\n' % size
        s += p + '     "Line Spacing" %d\n' % (size + 9)
        if colour:
            s += p + "     Color %s\n" % colour
        s += p + "    }\n"
    s += p + "   }\n"
    s += p + "  }\n"
    s += p + " }\n"
    s += p + "}\n"
    return s


# ============================================================== app windows
# One window per app kind. Five of them share the list-and-reader shape, which
# is the whole argument for generating this file.
WINDOWS = [
    ("Files",    "files",    "list",     780, 470),
    ("Mail",     "mail",     "mail",     870, 545),
    ("Messages", "messages", "chat",     750, 505),
    ("Contacts", "contacts", "contacts", 690, 465),
    ("Photos",   "photos",   "photos",   730, 505),
    ("Notes",    "notes",    "list",     640, 460),
    ("Terminal", "terminal", "term",     680, 400),
    ("Settings", "settings", "list",     660, 440),
    ("Calls",    "phone",    "list",     660, 440),
    ("Archive",  "doc",      "list",     660, 440),
]

BAR_H = 0.072          # title bar, as a share of the window
TOOL_Y = (0.018, 0.088)


def toolbar(p, ind):
    """The Game Master's row, above the list. Hidden unless authoring.

    ON THE DESKTOP THESE ARE A TOOLBAR AND NOT ROWS IN THE LIST. The handset
    had no room for anything else; a window has a header with 300 units of
    empty in it, and a verb that changes the mission should not have to be
    scrolled to.
    """
    s = ""
    xs = [(0.700, 0.760), (0.770, 0.830), (0.840, 0.900), (0.910, 0.980)]
    for (name, icon), (x0, x1) in zip(
            [("Add", "add"), ("Edit", "edit"), ("Del", "delete"), ("Save", "check")], xs):
        s += flatbtn(p + name, (x0, TOOL_Y[0], x1, TOOL_Y[1]), ind,
                     icon=icon, icon_px=17, bg="1 1 1 0.07", bg_hi="1 1 1 0.18",
                     tex="rounded")
    return s


def pane_head(p, ind, title=""):
    s = txt(p + "Head", (0.022, 0.020, 0.690, 0.086), ind, 14, text=title)
    s += img(p + "HeadRule", (0.022, 0.098, 0.978, 0.1005), ind, colour=LINE)
    s += toolbar(p, ind)
    return s


def pane_list(p, ind):
    s = pane_head(p, ind)
    s += scroll(p + "ListScroll", (0.018, 0.112, 0.392, 0.980), ind, p + "EntryList")
    s += img(p + "Split", (0.400, 0.112, 0.4015, 0.980), ind, colour=LINE)
    s += txt(p + "ReadHeading", (0.415, 0.120, 0.980, 0.186), ind, 15,
             rich=True, wrap=True, spacing=20)
    s += txt(p + "ReadStamp", (0.415, 0.192, 0.980, 0.240), ind, 11, colour=DIM_INK)
    s += scroll(p + "ReadScroll", (0.412, 0.256, 0.982, 0.980), ind,
                p + "ReadBody", kind="rich", size=12)
    s += txt(p + "Hint", (0.415, 0.470, 0.980, 0.540), ind, 12, colour=FAINT_INK,
             align=1)
    return s


def pane_mail(p, ind):
    s = pane_head(p, ind)
    s += scroll(p + "ListScroll", (0.018, 0.112, 0.352, 0.980), ind, p + "EntryList")
    s += img(p + "Split", (0.360, 0.112, 0.3615, 0.980), ind, colour=LINE)
    s += img(p + "Card", (0.375, 0.118, 0.980, 0.330), ind,
             colour="1 1 1 0.05", tex="tile")
    s += txt(p + "FromLabel", (0.392, 0.132, 0.600, 0.172), ind, 9,
             colour=FAINT_INK, text="FROM")
    s += txt(p + "From", (0.392, 0.170, 0.965, 0.218), ind, 13)
    s += txt(p + "Date", (0.392, 0.224, 0.965, 0.262), ind, 10, colour=DIM_INK)
    s += img(p + "CardRule", (0.392, 0.270, 0.965, 0.2715), ind, colour=LINE)
    s += txt(p + "Subject", (0.392, 0.278, 0.965, 0.322), ind, 15,
             rich=True, wrap=True, spacing=20)
    s += scroll(p + "ReadScroll", (0.372, 0.350, 0.982, 0.980), ind,
                p + "ReadBody", kind="rich", size=12)
    return s


def pane_chat(p, ind):
    s = pane_head(p, ind)
    s += scroll(p + "ListScroll", (0.018, 0.112, 0.330, 0.980), ind, p + "EntryList")
    s += img(p + "Split", (0.338, 0.112, 0.3395, 0.980), ind, colour=LINE)
    s += img(p + "Avatar", (0.354, 0.118, 0.396, 0.180), ind,
             colour="0.35 0.38 0.42 1", tex="circle")
    s += txt(p + "Initial", (0.354, 0.130, 0.396, 0.172), ind, 14, align=1)
    s += txt(p + "Who", (0.408, 0.118, 0.980, 0.158), ind, 14)
    s += txt(p + "When", (0.408, 0.156, 0.980, 0.190), ind, 10, colour=DIM_INK)
    s += img(p + "ChatRule", (0.348, 0.198, 0.980, 0.1995), ind, colour=LINE)
    s += scroll(p + "ThreadScroll", (0.346, 0.210, 0.982, 0.980), ind, p + "Thread")
    return s


def pane_contacts(p, ind):
    s = pane_head(p, ind)
    s += scroll(p + "ListScroll", (0.018, 0.112, 0.352, 0.980), ind, p + "EntryList")
    s += img(p + "Split", (0.360, 0.112, 0.3615, 0.980), ind, colour=LINE)
    s += img(p + "Big", (0.615, 0.150, 0.725, 0.330), ind,
             colour="0.35 0.38 0.42 1", tex="circle")
    s += txt(p + "BigText", (0.615, 0.200, 0.725, 0.288), ind, 30, align=1)
    s += txt(p + "Name", (0.380, 0.352, 0.965, 0.410), ind, 18, align=1)
    s += txt(p + "Number", (0.380, 0.414, 0.965, 0.456), ind, 12,
             colour=DIM_INK, align=1)
    s += flatbtn(p + "Call", (0.545, 0.478, 0.795, 0.552), ind,
                 label="CALL", size=12, bg="0.16 0.60 0.27 1",
                 bg_hi="0.22 0.72 0.34 1", tex="tile", label_align="centre")
    s += img(p + "CardRule", (0.400, 0.590, 0.960, 0.5915), ind, colour=LINE)
    s += txt(p + "NoteLabel", (0.400, 0.604, 0.960, 0.646), ind, 9, colour=FAINT_INK)
    s += scroll(p + "NoteScroll", (0.396, 0.652, 0.964, 0.980), ind,
                p + "Note", kind="rich", size=12, colour=DIM_INK)
    return s


def pane_photos(p, ind):
    s = pane_head(p, ind)
    cols = [(0.030, 0.255), (0.268, 0.493), (0.506, 0.731), (0.744, 0.969)]
    rows = [(0.130, 0.400), (0.415, 0.685), (0.700, 0.970)]
    i = 0
    for r0, r1 in rows:
        for c0, c1 in cols:
            s += flatbtn(p + "Tile%d" % i, (c0, r0, c1, r1), ind,
                         bg="1 1 1 0.07", bg_hi="1 1 1 0.18")
            s += img(p + "Shot%d" % i, (c0, r0, c1, r1), ind, colour="1 1 1 1")
            s += txt(p + "Cap%d" % i, (c0 + 0.008, r1 - 0.075, c1 - 0.008, r1 - 0.012),
                     ind, 10, colour=DIM_INK, align=1)
            i += 1
    s += txt(p + "GridHint", (0.030, 0.520, 0.969, 0.580), ind, 12,
             colour=FAINT_INK, align=1)
    return s


def pane_term(p, ind):
    s = pane_head(p, ind)
    s += img(p + "Glass", (0.018, 0.112, 0.982, 0.980), ind, colour=TERM_BG)
    s += scroll(p + "TermScroll", (0.030, 0.124, 0.970, 0.968), ind,
                p + "Term", kind="rich", size=11, colour="0.81 0.84 0.86 1")
    return s


PANES = {"list": pane_list, "mail": pane_mail, "chat": pane_chat,
         "contacts": pane_contacts, "photos": pane_photos, "term": pane_term}


def window(i, label, icon, kind, ind):
    p = "W%d" % i
    b = ""
    # The body, then the chrome on top of it, so a title bar always wins a
    # click against whatever the pane put under it.
    b += img(p + "Back", (0.0, 0.0, 1.0, 1.0), ind + 1, colour=SURFACE, tex="tile")
    b += frame(p + "Body", (0.0, BAR_H, 1.0, 1.0), ind + 1,
               PANES[kind](p, ind + 3))
    # The title bar is a BUTTON, not an image: an ImageWidget never receives a
    # click, which is the handset's home-bar lesson, and a drag starts with a
    # click like anything else.
    b += img(p + "BarSkin", (0.0, 0.0, 1.0, BAR_H), ind + 1,
             colour="1 1 1 1", tex="bartitle")
    b += flatbtn(p + "Bar", (0.0, 0.0, 1.0, BAR_H), ind + 1,
                 bg="1 1 1 0", bg_hi="1 1 1 0.06")
    b += img(p + "Icon", (0.012, 0.014, 0.038, 0.058), ind + 1,
             colour="1 1 1 1", tex=icon)
    b += txt(p + "Title", (0.048, 0.012, 0.800, 0.060), ind + 1, 12.5 // 1,
             text=label)
    b += flatbtn(p + "Min", (0.874, 0.010, 0.910, 0.062), ind + 1,
                 icon="min", icon_px=11, bg="1 1 1 0", bg_hi="1 1 1 0.14")
    b += flatbtn(p + "Max", (0.916, 0.010, 0.952, 0.062), ind + 1,
                 icon="max", icon_px=11, bg="1 1 1 0", bg_hi="1 1 1 0.14")
    b += flatbtn(p + "Close", (0.958, 0.010, 0.994, 0.062), ind + 1,
                 icon="close", icon_px=11, bg="1 1 1 0",
                 bg_hi="0.77 0.28 0.23 1")
    b += img(p + "BarRule", (0.0, BAR_H, 1.0, BAR_H + 0.0025), ind + 1, colour=LINE)
    return frame(p, (0.0, 0.0, 0.0, 0.0), ind, b, visible=False)


# ==================================================================== panel
def panel(ind):
    b = img("PanelBack", (0.0, 0.0, 1.0, 1.0), ind + 1, colour="1 1 1 1",
            tex="barpanel")
    b += flatbtn("Kick", (0.0000, 0.0, 0.1150, 1.0), ind + 1, icon="apps",
                 icon_px=21, pad_l=12, label="Applications", text_l=42, size=12,
                 bg="1 1 1 0", bg_hi="1 1 1 0.10")
    b += img("KickRule", (0.1150, 0.14, 0.1160, 0.86), ind + 1, colour=LINE)

    for i in range(6):
        x0 = 0.1210 + i * 0.0325
        b += flatbtn("Pin%d" % i, (x0, 0.10, x0 + 0.0290, 0.90), ind + 1,
                     icon="apps", icon_px=23, bg="1 1 1 0", bg_hi="1 1 1 0.12")
        b += img("PinBadge%d" % i, (x0 + 0.0175, 0.02, x0 + 0.0300, 0.40),
                 ind + 1, colour=ACCENT, tex="circle", visible=False)
        b += txt("PinBadgeText%d" % i, (x0 + 0.0175, 0.06, x0 + 0.0300, 0.36),
                 ind + 1, 9, align=1, visible=False)
    b += img("PinRule", (0.2110, 0.14, 0.2120, 0.86), ind + 1, colour=LINE)

    for i in range(10):
        x0 = 0.2170 + i * 0.0515
        b += flatbtn("Task%d" % i, (x0, 0.13, x0 + 0.0495, 0.87), ind + 1,
                     icon="apps", icon_px=16, pad_l=7, label="", text_l=27,
                     size=11, bg="1 1 1 0.06", bg_hi="1 1 1 0.14",
                     tex="rounded")
        b += img("TaskLit%d" % i, (x0, 0.84, x0 + 0.0495, 0.90), ind + 1,
                 colour=ACCENT, visible=False)

    b += img("TrayRule", (0.7380, 0.14, 0.7390, 0.86), ind + 1, colour=LINE)
    b += img("TrayNet", (0.7480, 0.28, 0.7619, 0.72), ind + 1,
             colour="0.81 0.84 0.86 1", tex="net")
    b += img("TrayVol", (0.7690, 0.28, 0.7829, 0.72), ind + 1,
             colour="0.81 0.84 0.86 1", tex="vol")
    b += img("TrayBatt", (0.7900, 0.28, 0.8039, 0.72), ind + 1,
             colour="0.81 0.84 0.86 1", tex="batt")
    b += txt("TrayPct", (0.8080, 0.28, 0.8560, 0.72), ind + 1, 10, colour=DIM_INK)
    b += txt("TrayClock", (0.8600, 0.10, 0.9800, 0.55), ind + 1, 13,
             align=2, text="00:00")
    b += txt("TrayDate", (0.8600, 0.54, 0.9800, 0.92), ind + 1, 10,
             colour=DIM_INK, align=2)
    return frame("Panel", (0.0, 0.9440, 1.0, 1.0), ind, b)


# ================================================================= launcher
def launcher(ind):
    b = img("LauncherBack", (0.0, 0.0, 1.0, 1.0), ind + 1,
            colour="0.137 0.157 0.176 0.98", tex="tile")
    b += img("LSearchBack", (0.045, 0.040, 0.955, 0.118), ind + 1,
             colour="1 1 1 0.07", tex="tile")
    b += img("LSearchIcon", (0.065, 0.058, 0.098, 0.100), ind + 1,
             colour="0.55 0.58 0.62 1", tex="search")
    b += txt("LSearchHint", (0.115, 0.055, 0.940, 0.103), ind + 1, 12,
             colour=FAINT_INK, text="Applications")
    b += img("LRule", (0.030, 0.140, 0.970, 0.1415), ind + 1, colour=LINE)

    cols = [(0.040, 0.270), (0.280, 0.510), (0.520, 0.750), (0.760, 0.990)]
    rows = [(0.165, 0.365), (0.375, 0.575), (0.585, 0.785)]
    i = 0
    for r0, r1 in rows:
        for c0, c1 in cols:
            b += flatbtn("LApp%d" % i, (c0, r0, c1, r1), ind + 1,
                         bg="1 1 1 0", bg_hi="1 1 1 0.09")
            b += img("LIcon%d" % i, (c0 + 0.072, r0 + 0.028, c1 - 0.072, r1 - 0.082),
                     ind + 1, colour="1 1 1 1", tex="apps")
            b += txt("LLabel%d" % i, (c0 + 0.010, r1 - 0.078, c1 - 0.010, r1 - 0.020),
                     ind + 1, 11, colour=DIM_INK, align=1)
            i += 1

    b += img("LFootRule", (0.030, 0.828, 0.970, 0.8295), ind + 1, colour=LINE)
    b += img("LFace", (0.045, 0.852, 0.108, 0.958), ind + 1, colour=ACCENT,
             tex="circle")
    b += txt("LFaceText", (0.045, 0.878, 0.108, 0.940), ind + 1, 12, align=1)
    b += txt("LUser", (0.125, 0.876, 0.560, 0.940), ind + 1, 12)
    b += flatbtn("LLock", (0.620, 0.862, 0.960, 0.952), ind + 1, icon="power",
                 icon_px=15, pad_l=12, label="Lock screen", text_l=36, size=11,
                 bg="1 1 1 0.05", bg_hi="1 1 1 0.14", tex="tile")
    return frame("Launcher", (0.0080, 0.4050, 0.3300, 0.9380), ind, b,
                 visible=False)


# ================================================================ lock screen
def lockscreen(ind):
    b = img("LockDim", (0.0, 0.0, 1.0, 1.0), ind + 1, colour="0.04 0.05 0.06 0.80")
    b += txt("LockClock", (0.0, 0.070, 1.0, 0.205), ind + 1, 62, align=1)
    b += txt("LockDate", (0.0, 0.212, 1.0, 0.252), ind + 1, 13, colour=DIM_INK,
             align=1)
    for i in range(3):
        y = 0.300 + i * 0.062
        b += img("Note%dCard" % i, (0.375, y, 0.625, y + 0.054), ind + 1,
                 colour="0.16 0.18 0.20 0.90", tex="tile", visible=False)
        b += img("Note%dIcon" % i, (0.388, y + 0.010, 0.406, y + 0.044), ind + 1,
                 colour="1 1 1 1", tex="messages", visible=False)
        b += txt("Note%dTitle" % i, (0.418, y + 0.006, 0.560, y + 0.030), ind + 1,
                 11, visible=False)
        b += txt("Note%dWho" % i, (0.418, y + 0.028, 0.560, y + 0.050), ind + 1,
                 10, colour=FAINT_INK, visible=False)
        b += txt("Note%dWhen" % i, (0.560, y + 0.012, 0.612, y + 0.042), ind + 1,
                 10, colour=FAINT_INK, align=2, visible=False)
    b += img("LockFace", (0.4590, 0.520, 0.5410, 0.660), ind + 1, colour=ACCENT,
             tex="circle")
    b += txt("LockFaceText", (0.4590, 0.556, 0.5410, 0.632), ind + 1, 34,
             align=1)
    b += txt("LockUser", (0.300, 0.676, 0.700, 0.716), ind + 1, 18, align=1)
    b += txt("LockHost", (0.300, 0.720, 0.700, 0.752), ind + 1, 11,
             colour=DIM_INK, align=1)
    b += img("LockFieldBack", (0.3900, 0.782, 0.5750, 0.836), ind + 1,
             colour="1 1 1 0.09", tex="tile")
    b += ('%sEditBoxWidgetClass "%s" {\n' % (" " * (ind + 1), g()) +
          '%s Name "LockField"\n' % (" " * (ind + 1)) +
          slot((0.3990, 0.790, 0.5650, 0.828), ind + 2) +
          "%s Clipping True\n" % (" " * (ind + 1)) +
          "%s components {\n" % (" " * (ind + 1)) +
          '%s  EditBoxFilterComponent "%s" {\n' % (" " * (ind + 1), g()) +
          "%s   m_iCharacterLimit 24\n" % (" " * (ind + 1)) +
          "%s  }\n" % (" " * (ind + 1)) +
          "%s }\n" % (" " * (ind + 1)) +
          "%s style blank\n" % (" " * (ind + 1)) +
          '%s Text ""\n' % (" " * (ind + 1)) +
          '%s "Font Size" 13\n' % (" " * (ind + 1)) +
          '%s "Min Font Size" 13\n' % (" " * (ind + 1)) +
          "%s}\n" % (" " * (ind + 1)))
    b += flatbtn("LockGo", (0.5830, 0.782, 0.6100, 0.836), ind + 1,
                 label=">", size=14, bg=ACCENT, bg_hi="0.90 0.54 0.18 1",
                 tex="tile", label_align="centre")
    b += txt("LockMsg", (0.300, 0.852, 0.700, 0.892), ind + 1, 11,
             colour=FAINT_INK, align=1)
    return frame("LockScreen", (0.0, 0.0, 1.0, 1.0), ind, b)


# ============================================================== editor pane
def editor(ind):
    b = img("EditorDim", (0.0, 0.0, 1.0, 1.0), ind + 1, colour="0.03 0.04 0.05 0.92")
    b += txt("EditorTitle", (0.280, 0.120, 0.720, 0.176), ind + 1, 16)
    b += img("EditorFieldBack", (0.280, 0.192, 0.720, 0.640), ind + 1,
             colour="1 1 1 0.06", tex="tile")
    b += ('%sEditBoxWidgetClass "%s" {\n' % (" " * (ind + 1), g()) +
          '%s Name "EditorField"\n' % (" " * (ind + 1)) +
          slot((0.294, 0.206, 0.706, 0.626), ind + 2) +
          "%s Clipping True\n" % (" " * (ind + 1)) +
          "%s components {\n" % (" " * (ind + 1)) +
          '%s  EditBoxFilterComponent "%s" {\n' % (" " * (ind + 1), g()) +
          "%s   m_iCharacterLimit 1000\n" % (" " * (ind + 1)) +
          "%s  }\n" % (" " * (ind + 1)) +
          "%s }\n" % (" " * (ind + 1)) +
          "%s style blank\n" % (" " * (ind + 1)) +
          '%s Text ""\n' % (" " * (ind + 1)) +
          '%s "Font Size" 14\n' % (" " * (ind + 1)) +
          '%s "Min Font Size" 14\n' % (" " * (ind + 1)) +
          '%s "Line Spacing" 19\n' % (" " * (ind + 1)) +
          "%s}\n" % (" " * (ind + 1)))
    b += flatbtn("EditorSave", (0.280, 0.662, 0.492, 0.722), ind + 1,
                 label="SAVE", size=12, bg="1 1 1 0.10", bg_hi="1 1 1 0.22",
                 tex="tile", label_align="centre")
    b += flatbtn("EditorCancel", (0.508, 0.662, 0.720, 0.722), ind + 1,
                 label="CANCEL", size=12, bg="1 1 1 0.10", bg_hi="1 1 1 0.22",
                 tex="tile", label_align="centre")
    return frame("EditorPane", (0.0, 0.0, 1.0, 1.0), ind, b, visible=False)


# ================================================================== assemble
screen = img("Wallpaper", (0.0, 0.0, 1.0, 1.0), 3, colour="1 1 1 1", tex="wall")

DESK = [("DeskHome", "folder", "Home"),
        ("DeskDocs", "folder", "Documents"),
        ("DeskFile", "doc", "readme.txt")]
for k, (nm, ic, lb) in enumerate(DESK):
    y = 0.020 + k * 0.115
    screen += flatbtn(nm, (0.010, y, 0.098, y + 0.112), 3,
                      bg="1 1 1 0", bg_hi="1 1 1 0.09")
    screen += img(nm + "Art", (0.038, y + 0.010, 0.070, y + 0.068), 3,
                  colour="1 1 1 1", tex=ic)
    screen += txt(nm + "Label", (0.006, y + 0.074, 0.102, y + 0.104), 3, 11,
                  align=1, text=lb)

for i, (label, icon, kind, w, h) in enumerate(WINDOWS):
    screen += window(i, label, icon, kind, 3)

screen += panel(3)
screen += launcher(3)

# THE DRAG CATCHER, AND WHY IT HAS TO EXIST. Enfusion has no OnMouseMove and no
# mouse capture: a title bar learns that the button went down and then hears
# nothing more, and the release lands on whatever widget the cursor is over by
# then -- which, after dragging a window, is never the title bar. This is a
# transparent button that covers the screen while a drag is running, so the
# release always has somewhere to land. It is invisible and it is only up for
# the duration of the drag.
screen += flatbtn("DragCatch", (0.0, 0.0, 1.0, 1.0), 3,
                  bg="1 1 1 0", bg_hi="1 1 1 0")
screen += lockscreen(3)
screen += editor(3)

doc = "FrameWidgetClass {\n"
doc += ' Name "rootFrame"\n'
doc += " {\n"
doc += img("Dim", (0.0, 0.0, 1.0, 1.0), 2, colour="0 0 0 0.90")
doc += frame("Screen", (0.5, 0.5, 0.5, 0.5), 2, screen)
doc += " }\n"
doc += "}\n"

with open(OUT, "w") as fh:
    fh.write(doc)

META = '''MetaFileClass {
 Name "{6A1C4F0B39E00000}UI/layouts/MCF/MCF_IntelDesktop.layout"
 Configurations {
  LayoutResourceClass PC {
  }
  LayoutResourceClass XBOX_ONE : PC {
  }
  LayoutResourceClass XBOX_SERIES : PC {
  }
  LayoutResourceClass PS4 : PC {
  }
  LayoutResourceClass PS5 : PC {
  }
  LayoutResourceClass HEADLESS : PC {
  }
 }
}
'''
with open(OUT + ".meta", "w") as fh:
    fh.write(META)

print("bytes:", len(doc), "guids:", _n[0], "lines:", doc.count("\n"))
