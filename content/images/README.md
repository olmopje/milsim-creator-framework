# Pictures for devices

Photographs a phone or a laptop can show.

**The how-to lives in the wiki: [Pictures on a device](https://github.com/olmopje/milsim-creator-framework/wiki/Device-Pictures).**
Converter, GitHub steps, size, troubleshooting -- all of it, written for
somebody who has never done this before.

These files are **not** part of the mod. They live here, are served by GitHub,
and are fetched by each player at runtime. Nothing in `addons/` refers to this
folder. Three pictures are here: `locomotive.txt`, `montignac_map.txt` and
`montignac_map_hq.txt`.

---

## The short version

A `.txt` holding base64, and nothing else required. A `data:image/jpeg;base64,`
header may be left on the front -- the decoder steps over it -- and `MCFIMG`
markers around the payload are supported but not needed.

Point an item's picture field at the **raw** address of the file:

```
https://raw.githubusercontent.com/olmopje/milsim-creator-framework/main/content/images/montignac_map_hq.txt
```

`raw.githubusercontent.com`, not `github.com` -- the second serves a web page
around the file.

## Why it is text and not an image

The engine will not load an image from a URL. Three routes were tried and
measured on 2026-09-10, and all three are shut:

- `ImageWidget.LoadImageTexture` refuses an http address outright.
- `RestContext.FILE`, the only file download, is marked
  `[Obsolete("Not supported, will be removed!")]` and is inert: it never calls
  back, never errors, and never writes the file.
- There is no script API to build a texture from bytes.

What does work is fetching the image **as text** and rebuilding it as a file on
the machine that draws it. `MCF_Device_ImageCache` writes it into the player's
own profile folder, choosing `.png` or `.jpg` from the magic bytes -- the loader
picks its decoder by file extension, so a jpeg written to a `.png` is refused
without a word.

The `MCFIMG` markers, where a file has them, make the payload unambiguous. A
host that wraps the text in a viewer page would otherwise be decoded as though
the page were the picture, because the letters in HTML are inside the base64
alphabet too. A raw file needs no markers.

## Limits, measured

Up to **4 000 000 base64 characters**, about 3 MB of picture. That figure is
about memory rather than time: past 300 000 characters the decode is spread
over frames, 20 000 at a time, so a large picture costs a wait and never a
dropped frame.

Measured 2026-09-26, decode plus write plus texture load:

```
   262 144 chars ->   196 608 bytes ->  75 ms
   524 288 chars ->   393 216 bytes -> 131 ms
 1 048 576 chars ->   786 432 bytes -> 236 ms
 2 097 152 chars -> 1 572 864 bytes -> 449 ms
```

Linear, about 0.22 microseconds a character. `montignac_map_hq.txt` --
545 069 characters, 408 801 bytes, 1800x1049 -- came out over 28 frames.

A string is not the limit and never was: Enforce built and indexed 5 242 880
characters end to end with every probe correct. The 163 840 this file used to
imply was where a test loop happened to stop, not a ceiling.

Jpeg, not png, and not a preference. The same map as a real png is 1 026 576
base64 characters against 545 069 as a jpeg at full resolution, and on a screen
in game they are indistinguishable.

Each player keeps about twelve megabytes of the pictures they have seen most
recently; older ones are dropped, so the folder does not grow forever.

> **If you made a picture before 26 September 2026 and only ever saw a strip of
> it, this is why.** Nothing over about 6 kB had ever actually reached disk.
> `Substring` in Enforce will not return more than 8191 characters and says
> nothing when it stops, and the decoder was being handed a payload cut off
> there. Your `.txt` is fine and needs no changes -- update the mod.
