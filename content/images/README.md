# Pictures for devices

Photographs a phone or a laptop can show.

**The how-to lives in the wiki: [Pictures on a device](https://github.com/olmopje/milsim-creator-framework/wiki/Device-Pictures).**
Converter, GitHub steps, size, troubleshooting -- all of it, written for
somebody who has never done this before.

These files are **not** part of the mod. They live here, are served by GitHub,
and are fetched by each player at runtime. Nothing in `addons/` refers to this
folder. Two test pictures are here: `locomotive.txt` and `montignac_map.txt`.

---

## The short version

```
MCFIMG
<one long line of base64>
MCFIMG
```

Point an item's picture field at the **raw** address of that file:

```
https://raw.githubusercontent.com/olmopje/milsim-creator-framework/main/content/images/locomotive.txt
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

The `MCFIMG` markers make the payload unambiguous. Without them the whole
response is decoded, which is fine for a file that is nothing but base64 and
wrong the moment anything wraps it: the letters in HTML are inside the base64
alphabet too.

## Limits, measured

A picture is refused past **160 000 base64 characters**, about 120 kB. What
limits it is decoding time, not the download. The 99 883 character map here --
74 900 bytes of JPEG -- decodes in 27 ms.

Jpeg is not a preference. The same map as a real png is 1 026 576 base64
characters, ten times the limit.

Each player keeps about twelve megabytes of the pictures they have seen most
recently; older ones are dropped, so the folder does not grow forever.

> **If you made a picture before 26 September 2026 and only ever saw a strip of
> it, this is why.** Nothing over about 6 kB had ever actually reached disk.
> `Substring` in Enforce will not return more than 8191 characters and says
> nothing when it stops, and the decoder was being handed a payload cut off
> there. The 120 000 this file used to promise had never once been true. Your
> `.txt` is fine and needs no changes -- update the mod.
