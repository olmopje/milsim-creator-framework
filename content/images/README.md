# Pictures for devices

Photographs a phone or a laptop can show, and later the ones on a billboard.

These are **not** part of the mod. They live here, are served by GitHub, and are
fetched by each player at runtime. Nothing in `addons/` refers to this folder.

---

## Why the files look like this

The engine will not load an image from a URL. Three routes were tried and
measured on 2026-09-10, and all three are shut:

- `ImageWidget.LoadImageTexture` refuses an http address outright.
- `RestContext.FILE`, the only file download, is marked
  `[Obsolete("Not supported, will be removed!")]` and is inert: it never calls
  back, never errors, and never writes the file.
- There is no script API to build a texture from bytes.

What does work is fetching the image **as text** and rebuilding it as a file on
the machine that draws it. So every picture here is stored base64-encoded, in a
`.txt`, and `MCF_Device_ImageCache` turns it back into a `.png` in the player's
profile folder.

## The format

```
MCFIMG
<one long line of base64>
MCFIMG
```

The `MCFIMG` markers are what make the payload unambiguous. Without them the
whole response is decoded, which is fine for a file that is nothing but base64
and wrong the moment anything wraps it -- a host that serves a viewer page
around the text would otherwise be decoded as though it were the picture,
because the letters in HTML are inside the base64 alphabet too.

The decoder also checks the result: a png starts `137 80 78 71` and a jpeg
`255 216 255`. Anything else is refused and says so in the log rather than
writing a file the loader will silently reject.

## Making one

```sh
{ echo MCFIMG; base64 -w0 photo.jpg; echo; echo MCFIMG; } > photo.txt
```

On Windows, `certutil -encode` works but wraps lines and adds a header -- the
decoder skips characters outside the alphabet, so the wrapping is harmless, but
delete the `-----BEGIN CERTIFICATE-----` lines or put the markers around only
the base64.

## Using one

The URL goes in an item's **PICTURE URL** field, either in the Game Master's
*Edit device* screen or in
`addons/MCF_Ops/Configs/Devices/MCF_DeviceProfiles.conf`:

```
https://raw.githubusercontent.com/olmopje/milsim-creator-framework/main/content/images/photo.txt
```

`raw.githubusercontent.com`, not `github.com` -- the second serves a page around
the file.

## Two things to know

**Size.** A picture is refused past 160 000 base64 characters, which is about
120 kB. What limits that is decoding time rather than the download: 5216
characters decode in 7 ms, and the 99 883 character map in this folder -- 74 912
bytes of JPEG -- takes under a second. Much larger than that and the pause is
long enough to be felt in a frame. Resize before encoding anyway: a phone screen
is a few hundred pixels across and nothing here needs to be bigger.

> **If you made a picture before 26 September 2026 and only ever saw a strip of
> it, this is why.** Nothing over about 6 kB had ever actually reached disk.
> `Substring` in Enforce will not return more than 8191 characters and says
> nothing when it stops, and the decoder was being handed a payload that had
> been cut off there. The 120 000 this file used to promise had never once been
> true. Your `.txt` is fine and needs no changes -- update the mod.

**Changing a picture means a new filename.** The cache is keyed by URL, so a
player who already fetched `photo.txt` keeps the copy they have. Publish
`photo_2.txt` and point the item at that instead.

**Each player fetches their own.** A player who cannot reach GitHub sees the
text without the picture, and that reads as a photograph that has not loaded --
not as an error, because it is not one.
