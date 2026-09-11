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

def col(value):
    """sRGB in, linear out.

    A widget's `Color` in a .layout is a LINEAR triple. Writing the sRGB floats
    a designer works in makes everything come out pale: the window bodies were
    authored as #282D32 and rendered mid-grey, and the accent as #C26314 and
    rendered a washed tangerine. Alpha is not a colour and passes through.

    Textures are not affected -- the importer handles their colour space -- so
    an image tinted `1 1 1 1` was always right and is why the panel and the
    wallpaper looked correct while every flat fill did not.
    """
    parts = value.split()
    out = []

    for i, raw in enumerate(parts):
        c = float(raw)

        if i < 3:
            if c <= 0.04045:
                c = c / 12.92
            else:
                c = ((c + 0.055) / 1.055) ** 2.4

        out.append("%.5f" % c)

    return " ".join(out)


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
        s += p + " Color %s\n" % col(colour)
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
        s += p + " Color %s\n" % col(colour)
    if align:
        # THE PROPERTY IS "Horizontal Alignment" AND ITS VALUE IS A WORD.
        # Two wrong guesses preceded this one: `Alignment 0.5 0` is a
        # FrameWidgetSlot property (the pivot, and it is ignored outright when
        # a slot's min and max anchors differ, which is every stretched box
        # here), and "Text Horizontal Align" does not exist at all -- it
        # appears nowhere in the engine binary, and an unknown key is dropped
        # in silence. The operations board has carried that same non-property
        # for months.
        s += p + ' "Horizontal Alignment" %s\n' % align
    s += p + "}\n"
    return s


def editbox(name, a, ind, size=12, limit=4000, spacing=None, colour=None):
    """A field you can type in.

    `style blank` and our own EditBoxWidgetClass rather than an override of
    SCR_EditBoxComponent: the override route wants the base component's own
    GUID reused or it silently paints a label reading "Editbox" at half width,
    which cost the handset an evening.
    """
    p = " " * ind
    s = p + 'EditBoxWidgetClass "%s" {\n' % g()
    s += p + ' Name "%s"\n' % name
    s += slot(a, ind + 1)
    s += p + " Clipping True\n"
    s += p + " components {\n"
    s += p + '  EditBoxFilterComponent "%s" {\n' % g()
    s += p + "   m_iCharacterLimit %d\n" % limit
    s += p + "  }\n"
    s += p + " }\n"
    s += p + " style blank\n"
    s += p + ' Text ""\n'
    s += p + ' "Font Size" %d\n' % size
    s += p + ' "Min Font Size" %d\n' % size
    if spacing:
        s += p + ' "Line Spacing" %d\n' % spacing
    if colour:
        # AN EDIT BOX'S TEXT IS WHITE UNLESS IT IS TOLD OTHERWISE, and two of
        # these sit on a white page. Nobody reads white on white.
        s += p + " Color %s\n" % col(colour)
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
            bg_sel=None, tex=None, label_align=None, ink=None):
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
    s += p + "   m_BackgroundDefault %s\n" % col(bg)
    s += p + "   m_BackgroundHovered %s\n" % col(bg_hi)
    s += p + "   m_BackgroundSelected %s\n" % col(sel)
    s += p + "   m_BackgroundSelectedHovered %s\n" % col(sel)
    s += p + "   m_BackgroundClicked %s\n" % col("1 1 1 0.22")
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
        if ink:
            # A BUTTON'S LABEL IS WHITE BY DEFAULT. Seventy-eight of these are
            # spreadsheet cells on a paper-white grid, where white is nothing.
            s += p + "     Color %s\n" % col(ink)
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
            s += p + "     Color %s\n" % col(colour)
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
    ("Files",    "files",    "files",    880, 520),
    ("Mail",     "mail",     "mail",     870, 545),
    ("Messages", "messages", "chat",     750, 505),
    ("Contacts", "contacts", "contacts", 690, 465),
    ("Photos",   "photos",   "photos",   730, 505),
    ("Notes",    "notes",    "list",     640, 460),
    ("Terminal", "terminal", "term",     680, 400),
    ("Settings", "settings", "list",     660, 440),
    ("Calls",    "phone",    "list",     660, 440),
    ("Archive",  "doc",      "list",     660, 440),

    # The editors. They are not apps on the device -- nothing in a device
    # profile mentions them -- they are what a file opens into, chosen by its
    # extension, and they take one item rather than a list.
    ("Text editor", "notes",    "text",  760, 520),
    ("Spreadsheet", "doc",      "sheet", 900, 560),
    ("Document",    "doc",      "doc",   780, 580),
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


def pane_head(p, ind, title="", tools=True):
    """The heading strip. `tools` is the Game Master's Add/Edit/Del/Save row.

    THE EDITORS MUST NOT HAVE IT. Its fourth button is called `<W>Save`, which
    is the name the editors gave their own SAVE -- two widgets with one name in
    one window, and FindAnyWidget returns whichever it reaches first. The
    editors bound their handler to the toolbar's hidden tick and the visible
    SAVE button did nothing at all. An editor has no list to add to or delete
    from either, so the row was wrong twice over.
    """
    s = txt(p + "Head", (0.022, 0.020, 0.690, 0.086), ind, 14, text=title)
    s += img(p + "HeadRule", (0.022, 0.098, 0.978, 0.1005), ind, colour=LINE)
    if tools:
        s += toolbar(p, ind)
    return s


def pane_list(p, ind):
    s = pane_head(p, ind)
    # The location line. Empty for every app but the file manager, which is the
    # only one of the five sharing this pane that has anywhere to be.
    s += txt(p + "Where", (0.022, 0.100, 0.690, 0.150), ind, 11, colour=DIM_INK)
    s += scroll(p + "ListScroll", (0.018, 0.158, 0.392, 0.980), ind, p + "EntryList")
    s += img(p + "Split", (0.400, 0.112, 0.4015, 0.980), ind, colour=LINE)
    s += txt(p + "ReadHeading", (0.415, 0.120, 0.980, 0.186), ind, 15,
             rich=True, wrap=True, spacing=20)
    s += txt(p + "ReadStamp", (0.415, 0.192, 0.980, 0.240), ind, 11, colour=DIM_INK)
    s += scroll(p + "ReadScroll", (0.412, 0.256, 0.982, 0.980), ind,
                p + "ReadBody", kind="rich", size=12)
    s += txt(p + "Hint", (0.415, 0.470, 0.980, 0.540), ind, 12, colour=FAINT_INK,
             align="Center")
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
    s += txt(p + "Initial", (0.354, 0.130, 0.396, 0.172), ind, 14, align="Center")
    s += txt(p + "Who", (0.408, 0.118, 0.980, 0.158), ind, 14)
    s += txt(p + "When", (0.408, 0.156, 0.980, 0.190), ind, 10, colour=DIM_INK)
    s += img(p + "ChatRule", (0.348, 0.198, 0.980, 0.1995), ind, colour=LINE)
    s += scroll(p + "ThreadScroll", (0.346, 0.210, 0.982, 0.980), ind, p + "Thread")
    return s


def pane_contacts(p, ind):
    s = pane_head(p, ind)
    s += scroll(p + "ListScroll", (0.018, 0.112, 0.352, 0.980), ind, p + "EntryList")
    s += img(p + "Split", (0.360, 0.112, 0.3615, 0.980), ind, colour=LINE)
    s += img(p + "Big", (0.615, 0.150, 0.725, 0.3131), ind,
             colour="0.35 0.38 0.42 1", tex="circle")
    s += txt(p + "BigText", (0.615, 0.188, 0.725, 0.272), ind, 30, align="Center")
    s += txt(p + "Name", (0.380, 0.352, 0.965, 0.410), ind, 18, align="Center")
    s += txt(p + "Number", (0.380, 0.414, 0.965, 0.456), ind, 12,
             colour=DIM_INK, align="Center")
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
                     ind, 10, colour=DIM_INK, align="Center")
            i += 1
    s += txt(p + "GridHint", (0.030, 0.520, 0.969, 0.580), ind, 12,
             colour=FAINT_INK, align="Center")
    return s


def pane_term(p, ind):
    s = pane_head(p, ind)
    s += img(p + "Glass", (0.018, 0.112, 0.982, 0.980), ind, colour=TERM_BG)
    s += scroll(p + "TermScroll", (0.030, 0.124, 0.970, 0.968), ind,
                p + "Term", kind="rich", size=11, colour="0.81 0.84 0.86 1")
    return s


# --------------------------------------------------------- the file manager
def pane_files(p, ind):
    """A tree on the left, the folder's contents on the right.

    NOT the list-and-reader every other app uses. A file manager's left half is
    where you are, not what is in it, and its right half is a table of files --
    which is why the first build read as a message list with folders on top of
    it. Opening a file here does not fill a reading pane; it opens the file in
    the editor its extension calls for, the way a machine does.
    """
    s = pane_head(p, ind)
    s += txt(p + "Where", (0.022, 0.100, 0.690, 0.150), ind, 11, colour=DIM_INK)
    s += scroll(p + "TreeScroll", (0.016, 0.158, 0.300, 0.980), ind, p + "TreeList")
    s += img(p + "Split", (0.308, 0.112, 0.3095, 0.980), ind, colour=LINE)
    s += txt(p + "ColName", (0.330, 0.112, 0.700, 0.152), ind, 9, colour=FAINT_INK,
             text="NAME")
    s += txt(p + "ColWhen", (0.740, 0.112, 0.972, 0.152), ind, 9, colour=FAINT_INK,
             align="Right", text="MODIFIED")
    s += img(p + "ColRule", (0.318, 0.156, 0.982, 0.1575), ind, colour=LINE)
    s += scroll(p + "FileScroll", (0.318, 0.162, 0.982, 0.980), ind, p + "FileList")
    s += txt(p + "Hint", (0.330, 0.400, 0.972, 0.460), ind, 12, colour=FAINT_INK,
             align="Center")
    return s


# ------------------------------------------------------------- the text editor
def pane_text(p, ind):
    """A plain-text editor: a dark field, a numbered gutter, one column of
    monospaced-looking text. It is the cheapest of the three to draw and the
    one a player will open most, because most of what is worth finding on a
    seized machine was typed into a text file."""
    s = pane_head(p, ind, tools=False)
    s += img(p + "Glass", (0.016, 0.112, 0.984, 0.980), ind, colour=TERM_BG)
    s += txt(p + "Gutter", (0.022, 0.122, 0.062, 0.972), ind, 11,
             colour=FAINT_INK, align="Right", rich=True, spacing=20)
    s += img(p + "GutterRule", (0.070, 0.112, 0.0712, 0.980), ind, colour=LINE)

    # TWO BODIES, ONE SHOWING. A player reads; a Game Master types. An edit box
    # that is always there would let a player rewrite the evidence they were
    # sent to find, and a read-only pane would leave the Game Master with
    # nowhere to write it in the first place.
    s += scroll(p + "BodyScroll", (0.080, 0.118, 0.978, 0.930), ind,
                p + "FileBody", kind="rich", size=12, colour="0.84 0.87 0.89 1")
    s += editbox(p + "BodyEdit", (0.080, 0.118, 0.978, 0.930), ind, 12, 6000, 19)
    s += img(p + "BarFill", (0.016, 0.934, 0.984, 0.980), ind, colour="1 1 1 0.05")
    s += txt(p + "Status", (0.030, 0.940, 0.700, 0.976), ind, 10, colour=FAINT_INK)
    s += flatbtn(p + "FileSave", (0.800, 0.938, 0.972, 0.976), ind, label="SAVE",
                 size=11, bg=ACCENT, bg_hi="0.90 0.54 0.18 1", tex="tile",
                 label_align="centre")
    return s


# ------------------------------------------------------------- the spreadsheet
def pane_sheet(p, ind):
    """A grid. The body is read as comma-separated lines -- the first line is
    the header row -- which is a convention a mission maker can type into a
    config without learning anything, and is what a spreadsheet is."""
    s = pane_head(p, ind, tools=False)

    # The formula bar. ONE edit box for seventy-eight cells: a spreadsheet is
    # the one editor where the field you type in is not where the value lives,
    # and copying that is both correct and seventy-seven widgets cheaper.
    s += img(p + "BarFill", (0.016, 0.108, 0.984, 0.156), ind, colour="1 1 1 0.06")
    s += txt(p + "Ref", (0.026, 0.114, 0.080, 0.150), ind, 11, align="Center")
    s += img(p + "RefRule", (0.086, 0.112, 0.0872, 0.152), ind, colour=LINE)
    s += editbox(p + "Formula", (0.098, 0.114, 0.828, 0.150), ind, 11, 200)
    s += flatbtn(p + "Commit", (0.838, 0.112, 0.906, 0.152), ind, icon="check",
                 icon_px=15, bg="1 1 1 0.08", bg_hi="1 1 1 0.18", tex="tile")
    s += flatbtn(p + "FileSave", (0.914, 0.112, 0.978, 0.152), ind, label="SAVE",
                 size=10, bg=ACCENT, bg_hi="0.90 0.54 0.18 1", tex="tile",
                 label_align="centre")

    s += img(p + "Paper", (0.016, 0.166, 0.984, 0.980), ind, colour="0.93 0.94 0.95 1")

    COLS = 6
    ROWS = 13
    x0, gutter, cw = 0.016, 0.046, 0.153
    y0, hh, rh = 0.166, 0.042, 0.0555

    # the header strip and the row-number strip, in the grey a spreadsheet uses
    s += img(p + "HeadFill", (x0, y0, 0.984, y0 + hh), ind, colour="0.80 0.82 0.84 1")
    s += img(p + "SideFill", (x0, y0, x0 + gutter, 0.980), ind, colour="0.80 0.82 0.84 1")

    letters = ["A", "B", "C", "D", "E", "F"]
    for c in range(COLS):
        cx = x0 + gutter + c * cw
        s += txt(p + "Col%d" % c, (cx, y0 + 0.006, cx + cw, y0 + hh - 0.004), ind, 10,
                 colour="0.26 0.28 0.31 1", align="Center", text=letters[c])
        s += img(p + "VRule%d" % c, (cx, y0, cx + 0.0012, 0.980), ind,
                 colour="0.72 0.74 0.76 1")

    for r in range(ROWS):
        ry = y0 + hh + r * rh
        s += txt(p + "Row%d" % r, (x0, ry + 0.010, x0 + gutter - 0.006, ry + rh - 0.006),
                 ind, 9, colour="0.26 0.28 0.31 1", align="Right", text=str(r + 1))
        s += img(p + "HRule%d" % r, (x0, ry, 0.984, ry + 0.0012), ind,
                 colour="0.72 0.74 0.76 1")

        for c in range(COLS):
            cx = x0 + gutter + c * cw
            s += flatbtn(p + "Cell%d_%d" % (r, c),
                         (cx + 0.0012, ry + 0.0012, cx + cw, ry + rh),
                         ind, label="", text_l=8, size=10,
                         bg="1 1 1 0", bg_hi="0.55 0.62 0.72 0.35",
                         bg_sel="0.29 0.47 0.72 0.45",
                         ink="0.12 0.13 0.15 1")

    return s


# ---------------------------------------------------------- the document viewer
def pane_doc(p, ind):
    """A page on a desk. White, inset, with its title set larger than its body
    -- which is all that separates a word processor from a text editor to look
    at, and it is enough for a player to know which one they are holding."""
    s = pane_head(p, ind, tools=False)
    s += img(p + "Desk", (0.016, 0.112, 0.984, 0.980), ind, colour="0.11 0.12 0.14 1")
    s += img(p + "Page", (0.170, 0.128, 0.830, 0.980), ind, colour="0.97 0.97 0.96 1")
    # DocTitle, not Title: the window's own title bar is already called
    # `<W>Title`, and two widgets with one name in one window is how the SAVE
    # button ended up bound to something invisible.
    s += txt(p + "DocTitle", (0.206, 0.160, 0.794, 0.216), ind, 17,
             colour="0.10 0.11 0.13 1", rich=True, wrap=True, spacing=22)
    s += editbox(p + "TitleEdit", (0.206, 0.160, 0.794, 0.216), ind, 16, 200,
                 colour="0.10 0.11 0.13 1")
    s += img(p + "Rule", (0.206, 0.226, 0.794, 0.2275), ind, colour="0.72 0.72 0.70 1")
    s += txt(p + "Stamp", (0.206, 0.236, 0.794, 0.272), ind, 10,
             colour="0.42 0.43 0.44 1")
    s += scroll(p + "BodyScroll", (0.200, 0.290, 0.800, 0.930), ind,
                p + "FileBody", kind="rich", size=12, colour="0.13 0.14 0.16 1")
    s += editbox(p + "BodyEdit", (0.206, 0.290, 0.794, 0.930), ind, 12, 6000, 20,
                 colour="0.13 0.14 0.16 1")
    s += flatbtn(p + "FileSave", (0.626, 0.938, 0.798, 0.976), ind, label="SAVE",
                 size=11, bg=ACCENT, bg_hi="0.90 0.54 0.18 1", tex="tile",
                 label_align="centre")
    s += txt(p + "Status", (0.206, 0.940, 0.610, 0.976), ind, 10, colour=FAINT_INK)
    return s


PANES = {"list": pane_list, "mail": pane_mail, "chat": pane_chat,
         "contacts": pane_contacts, "photos": pane_photos, "term": pane_term,
         "files": pane_files, "text": pane_text, "sheet": pane_sheet,
         "doc": pane_doc}


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

    PIN_W, PIN_PITCH = 0.0235, 0.0265
    for i in range(6):
        x0 = 0.1210 + i * PIN_PITCH
        b += flatbtn("Pin%d" % i, (x0, 0.10, x0 + PIN_W, 0.90), ind + 1,
                     icon="apps", icon_px=22, bg="1 1 1 0", bg_hi="1 1 1 0.12")
        b += img("PinBadge%d" % i, (x0 + 0.0125, 0.02, x0 + 0.0245, 0.40),
                 ind + 1, colour=ACCENT, tex="circle", visible=False)
        b += txt("PinBadgeText%d" % i, (x0 + 0.0125, 0.06, x0 + 0.0245, 0.36),
                 ind + 1, 9, align="Center", visible=False)
    b += img("PinRule", (0.2810, 0.14, 0.2820, 0.86), ind + 1, colour=LINE)

    TASK_W, TASK_PITCH = 0.0505, 0.0530
    for i in range(10):
        x0 = 0.2870 + i * TASK_PITCH
        b += flatbtn("Task%d" % i, (x0, 0.13, x0 + TASK_W, 0.87), ind + 1,
                     icon="apps", icon_px=15, pad_l=7, label="", text_l=26,
                     size=10, bg="1 1 1 0.06", bg_hi="1 1 1 0.14",
                     tex="rounded")
        # INSIDE the button's own footprint. At 0.84-0.90 it hung below the
        # button and read as a stray orange line rather than as the mark on
        # the window you are looking at.
        b += img("TaskLit%d" % i, (x0 + 0.0045, 0.790, x0 + TASK_W - 0.0045, 0.850),
                 ind + 1, colour=ACCENT, visible=False)

    b += img("TrayRule", (0.8180, 0.14, 0.8190, 0.86), ind + 1, colour=LINE)
    b += img("TrayNet", (0.8250, 0.30, 0.8389, 0.70), ind + 1,
             colour="0.81 0.84 0.86 1", tex="net")
    b += img("TrayVol", (0.8460, 0.30, 0.8599, 0.70), ind + 1,
             colour="0.81 0.84 0.86 1", tex="vol")
    b += img("TrayBatt", (0.8670, 0.30, 0.8809, 0.70), ind + 1,
             colour="0.81 0.84 0.86 1", tex="batt")
    b += txt("TrayPct", (0.8850, 0.30, 0.9180, 0.70), ind + 1, 10, colour=DIM_INK)
    b += txt("TrayClock", (0.9230, 0.10, 0.9850, 0.55), ind + 1, 13,
             align="Right", text="00:00")
    b += txt("TrayDate", (0.9230, 0.54, 0.9850, 0.92), ind + 1, 10,
             colour=DIM_INK, align="Right")
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
                     ind + 1, 11, colour=DIM_INK, align="Center")
            i += 1

    b += img("LFootRule", (0.030, 0.828, 0.970, 0.8295), ind + 1, colour=LINE)
    # The launcher is 0.322 of the screen wide and 0.333 of it tall, so a disc
    # 0.063 wide has to be 0.061 tall, not the 0.106 it was.
    b += img("LFace", (0.045, 0.850, 0.108, 0.911), ind + 1, colour=ACCENT,
             tex="circle")
    b += txt("LFaceText", (0.045, 0.862, 0.108, 0.902), ind + 1, 12, align="Center")
    b += txt("LUser", (0.125, 0.862, 0.560, 0.902), ind + 1, 12)
    b += flatbtn("LLock", (0.620, 0.848, 0.960, 0.916), ind + 1, icon="power",
                 icon_px=15, pad_l=12, label="Lock screen", text_l=36, size=11,
                 bg="1 1 1 0.05", bg_hi="1 1 1 0.14", tex="tile")
    return frame("Launcher", (0.0080, 0.4050, 0.3300, 0.9380), ind, b,
                 visible=False)


# ============================================================= context menu
def ctxmenu(ind):
    """What a right-click offers a Game Master.

    Placed at the cursor by the shell, so its slot is a point anchor and its
    size is set from script -- the same construction a window uses, and for the
    same reason: a thing that appears where the mouse is has no fixed anchor.
    """
    b = img("CtxBack", (0.0, 0.0, 1.0, 1.0), ind + 1,
            colour="0.137 0.157 0.176 0.98", tex="tile")
    rows = [("CtxItem0", "add", "New file"),
            ("CtxItem1", "folder", "New folder"),
            ("CtxItem2", "edit", "Rename"),
            ("CtxItem3", "delete", "Delete")]

    for i, (name, icon, label) in enumerate(rows):
        y0 = 0.035 + i * 0.240
        b += flatbtn(name, (0.030, y0, 0.970, y0 + 0.215), ind + 1,
                     icon=icon, icon_px=16, pad_l=10, label=label, text_l=34,
                     size=12, bg="1 1 1 0", bg_hi="1 1 1 0.12")

    return frame("CtxMenu", (0.0, 0.0, 0.0, 0.0), ind, b, visible=False)


# ================================================================ lock screen
def lockscreen(ind):
    b = img("LockDim", (0.0, 0.0, 1.0, 1.0), ind + 1, colour="0.04 0.05 0.06 0.80")
    b += txt("LockClock", (0.0, 0.070, 1.0, 0.205), ind + 1, 62, align="Center")
    b += txt("LockDate", (0.0, 0.212, 1.0, 0.252), ind + 1, 13, colour=DIM_INK,
             align="Center")
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
                 10, colour=FAINT_INK, align="Right", visible=False)
    # 16:10, so a disc 0.082 wide is 0.1312 tall.
    b += img("LockFace", (0.4590, 0.520, 0.5410, 0.6512), ind + 1, colour=ACCENT,
             tex="circle")
    b += txt("LockFaceText", (0.4590, 0.552, 0.5410, 0.620), ind + 1, 34,
             align="Center")
    b += txt("LockUser", (0.300, 0.676, 0.700, 0.716), ind + 1, 18, align="Center")
    b += txt("LockHost", (0.300, 0.720, 0.700, 0.752), ind + 1, 11,
             colour=DIM_INK, align="Center")
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
             colour=FAINT_INK, align="Center")
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
screen += flatbtn("DeskClick", (0.0, 0.0, 1.0, 1.0), 3, bg="1 1 1 0", bg_hi="1 1 1 0")

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
                  align="Center", text=lb)

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
screen += ctxmenu(3)
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

# ---------------------------------------------- two widgets, one name
# THIS IS THE MOST EXPENSIVE FAULT THIS FILE CAN PRODUCE and it is silent.
# FindAnyWidget and GetButtonText return whichever match they reach first, so
# a second widget with the same name does not fail -- it quietly steals every
# lookup. It cost the editors their SAVE button (bound to the Game Master
# toolbar's hidden tick, which is also called `<W>Save`) and their body text
# (written into the pane's container, also called `<W>Body`).
#
# `Background` is the one legitimate repeat: a button's fill MUST be called
# that, it is the only child SCR_ButtonTextComponent tints, and it is only ever
# looked up through its own button.
import re as _re
_names = _re.findall(r'Name\s+"([^"]+)"', doc)
_seen = {}
for _name in _names:
    _seen[_name] = _seen.get(_name, 0) + 1
_dupes = sorted(n for n, c in _seen.items() if c > 1 and n != "Background")
if _dupes:
    raise SystemExit("DUPLICATE WIDGET NAMES: " + ", ".join(_dupes))

print("bytes:", len(doc), "guids:", _n[0], "lines:", doc.count("\n"),
      "names:", len(_names))
