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

**Size.** 5216 base64 characters decode in 7 ms; reckon on about 90 ms for a
50 kB image. Anything past 120 000 characters is refused, which is roughly 90 kB
of picture. Resize before encoding: a phone screen is a few hundred pixels
across and nothing here needs to be larger.

**Changing a picture means a new filename.** The cache is keyed by URL, so a
player who already fetched `photo.txt` keeps the copy they have. Publish
`photo_2.txt` and point the item at that instead.

**Each player fetches their own.** A player who cannot reach GitHub sees the
text without the picture, and that reads as a photograph that has not loaded --
not as an error, because it is not one.
