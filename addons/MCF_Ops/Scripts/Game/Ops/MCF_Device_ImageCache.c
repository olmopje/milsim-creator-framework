//! Images from outside the mod, on a device screen.
//!
//! HOW THIS IS POSSIBLE AT ALL, because the obvious routes are all shut and it
//! is worth knowing which ones before reopening the question:
//!
//!   ImageWidget.LoadImageTexture on an http address   refused outright
//!   RestContext.FILE, the only file download          INERT -- never calls
//!                                                     back, never errors,
//!                                                     never writes the file,
//!                                                     and is marked
//!                                                     [Obsolete("Not supported")]
//!   bytes -> texture in script                        no API. ScreenshotTextureData
//!                                                     is an engine pointer with no
//!                                                     constructor
//!
//! What IS open, and what this class is built out of, every link measured on
//! 2026-09-10 rather than assumed:
//!
//!   RestContext.GET                text from a URL, supported, not obsolete
//!   base64 -> array<int>           in script, 5216 chars in 7 ms
//!   FileHandle.WriteArray          raw bytes to $profile:, one byte per element
//!   LoadImageTexture(.., true)     png from disk, no import, no conversion
//!
//! So an image travels as TEXT and is rebuilt as a file on the machine that
//! draws it. The one requirement falls outside the mod: whoever publishes the
//! image has to publish a base64 copy beside it. A raw .jpg URL cannot be used,
//! because RestCallback.GetData returns a string and binary in an Enforce
//! string dies at the first zero byte.
//!
//! EACH CLIENT FETCHES ITS OWN. Unlike device content, which the server resolves
//! and replicates, a texture has to exist on the machine drawing it. That means
//! a player behind a firewall sees no photograph -- and the shell has to treat
//! that as normal, not as an error, because it is.
//!
//! ON WRITING TO A PLAYER'S DISK. $profile: is the sanctioned place for a mod to
//! write and the only one FileIO.DeleteFile will accept, which is why the cache
//! lives there and why it can be cleaned up.

class MCF_Device_ImageCache
{
	//! Everything this writes lives under one folder, so it can be found, and
	//! deleted, without guessing.
	protected static const string FOLDER = "$profile:MCF/images/";

	//! Characters of base64. A 200 kB image is 270 000 of them, which is two
	//! seconds of decoding on the numbers measured -- long enough to be felt.
	//! Anything past this is refused rather than quietly stalling the frame.
	protected static const int MAX_ENCODED = 120000;

	//! What marks the payload inside whatever the host wrapped it in.
	//!
	//! NOT OPTIONAL PARANOIA. A document host serves a viewer page around the
	//! text unless asked for the raw form, and the decoder cannot tell the
	//! difference by itself: it skips characters outside the base64 alphabet,
	//! and the letters in HTML are inside it. Wrapped content would decode
	//! happily into rubbish. With markers the payload is unambiguous whatever
	//! surrounds it; without them the whole response is used, which is right
	//! for a file that really is nothing but base64.
	protected static const string MARKER = "MCFIMG";

	protected static ref array<int> s_aAlphabet;
	protected static ref map<string, ref RestCallback> s_aPending = new map<string, ref RestCallback>();

	//! Where a cached image lives, whether or not it is there yet.
	//!
	//! The key is the caller's name for the picture, not the URL: a URL contains
	//! characters a path may not, and two devices naming the same photograph
	//! should share one file.
	static string PathFor(string key)
	{
		return FOLDER + Sanitise(key) + ".png";
	}

	static bool IsCached(string key)
	{
		return FileIO.FileExists(PathFor(key));
	}

	//! Rebuilds an image from base64 text and puts it in the cache.
	//! \return True if the file is now on disk.
	static bool StoreFromBase64(string key, string encoded)
	{
		if (key.IsEmpty() || encoded.IsEmpty())
			return false;

		if (encoded.Length() > MAX_ENCODED)
		{
			MCF_Core_Log.Warn("device image '" + key + "' is " + encoded.Length().ToString()
				+ " encoded characters, past the " + MAX_ENCODED.ToString() + " this will decode -- refused");
			return false;
		}

		array<int> bytes = {};
		if (!Decode(Payload(encoded), bytes))
		{
			MCF_Core_Log.Warn("device image '" + key + "' is not valid base64");
			return false;
		}

		// Refuse it here rather than writing a file the loader will reject
		// silently. A response that decoded into something is not the same as a
		// response that decoded into a picture -- an HTML page will do the
		// first and never the second.
		if (!LooksLikeImage(bytes))
		{
			MCF_Core_Log.Warn("device image '" + key + "' decoded to " + bytes.Count().ToString()
				+ " byte(s) that are not a png or a jpeg -- the address is probably serving a page rather than the text."
				+ " First four: " + Head(bytes));
			return false;
		}

		FileIO.MakeDirectory(FOLDER);

		string path = PathFor(key);

		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
		{
			MCF_Core_Log.Warn("device image '" + key + "' could not be written to " + path);
			return false;
		}

		// One byte per element is what makes this a binary write rather than a
		// text one, and it is the whole reason this class can exist.
		int written = file.WriteArray(bytes, 1, bytes.Count());
		file.Close();

		if (written != bytes.Count())
		{
			MCF_Core_Log.Warn("device image '" + key + "' wrote " + written.ToString()
				+ " of " + bytes.Count().ToString() + " byte(s)");
			return false;
		}

		MCF_Core_Log.Debug("device image '" + key + "' cached, " + written.ToString() + " byte(s)");
		return true;
	}

	//! Puts a cached image into a widget.
	//! \return True if it drew. False is normal -- a client that never fetched
	//!         it, or could not, simply has no picture.
	static bool Show(notnull ImageWidget widget, string key)
	{
		string path = PathFor(key);

		if (!FileIO.FileExists(path))
			return false;

		// fromLocalStorage is what skips the resource database. Without it the
		// path is looked up as an imported asset and is not found.
		return widget.LoadImageTexture(0, path, false, true);
	}

	//! Fetches base64 text from a URL and caches what comes back.
	//!
	//! The host and the path are separate because RestApi wants a context per
	//! host and a request path per call.
	static void Fetch(string host, string path, string key)
	{
		if (host.IsEmpty() || path.IsEmpty() || key.IsEmpty())
			return;

		if (IsCached(key))
			return;

		// One request per key at a time. Two devices showing the same photograph
		// would otherwise both fetch it, and the second write could land while
		// the first is still open.
		if (s_aPending.Contains(key))
			return;

		RestApi rest = GetGame().GetRestApi();
		if (!rest)
			return;

		RestContext context = rest.GetContext(host);
		if (!context)
			return;

		context.SetTimeout(20);

		// Held in a map because RestCallback's own documentation says it is
		// deleted the moment it is not referenced: "If callback is not stored
		// as ref then it will be deleted after its execution finishes."
		MCF_Device_ImageFetch fetch = new MCF_Device_ImageFetch(key);
		s_aPending.Insert(key, fetch.GetCallback());

		context.GET(fetch.GetCallback(), path);
	}

	//! Called by the fetch when it is over, either way.
	static void FinishFetch(string key)
	{
		s_aPending.Remove(key);
	}

	//! Throws the whole cache away. Nothing calls this yet; it exists because a
	//! mod that writes to a player's disk should be able to stop.
	static void Clear(notnull array<string> keys)
	{
		foreach (string key : keys)
		{
			FileIO.DeleteFile(PathFor(key));
		}
	}

	// ------------------------------------------------------------- internals

	//! What is between the markers, or everything if there are none.
	protected static string Payload(string text)
	{
		int start = text.IndexOf(MARKER);
		if (start < 0)
			return text;

		start = start + MARKER.Length();

		int end = text.IndexOfFrom(start, MARKER);
		if (end < 0)
			return text.Substring(start, text.Length() - start);

		return text.Substring(start, end - start);
	}

	//! The first bytes of a png are 137 80 78 71; of a jpeg, 255 216 255.
	//! Anything else is not a picture, whatever it decoded from.
	protected static bool LooksLikeImage(notnull array<int> bytes)
	{
		if (bytes.Count() < 4)
			return false;

		if (bytes[0] == 137 && bytes[1] == 80 && bytes[2] == 78 && bytes[3] == 71)
			return true;

		return bytes[0] == 255 && bytes[1] == 216 && bytes[2] == 255;
	}

	//! The first four bytes, for a log line that says what went wrong rather
	//! than that something did.
	protected static string Head(notnull array<int> bytes)
	{
		string result = "";
		for (int i = 0; i < 4 && i < bytes.Count(); i++)
		{
			if (i > 0)
				result = result + " ";

			result = result + bytes[i].ToString();
		}

		return result;
	}

	//! A key becomes a filename, so it may not contain anything a path cannot.
	protected static string Sanitise(string key)
	{
		string result = key;
		result.Replace("/", "_");
		result.Replace("\\", "_");
		result.Replace(":", "_");
		result.Replace("..", "_");
		result.Replace(" ", "_");
		result.Replace("\n", "");
		result.Replace("\r", "");
		return result;
	}

	//! Base64 to bytes. Enforce has none of its own.
	//!
	//! The alphabet becomes a 128-entry table once, so the inner loop is an
	//! index rather than a search. Measured at 5216 characters in 7 ms, which
	//! is where MAX_ENCODED comes from rather than from a guess.
	protected static bool Decode(string encoded, notnull array<int> outBytes)
	{
		BuildAlphabet();

		outBytes.Clear();

		int accumulator;
		int bits;
		int length = encoded.Length();

		for (int i = 0; i < length; i++)
		{
			int code = encoded.Get(i).ToAscii();
			if (code < 0 || code > 127)
				continue;

			int value = s_aAlphabet[code];

			// Everything outside the alphabet: newlines, and the '=' padding,
			// which carries no bits of its own.
			if (value < 0)
				continue;

			accumulator = (accumulator << 6) | value;
			bits = bits + 6;

			if (bits < 8)
				continue;

			bits = bits - 8;
			outBytes.Insert((accumulator >> bits) & 0xFF);
		}

		return !outBytes.IsEmpty();
	}

	protected static void BuildAlphabet()
	{
		if (s_aAlphabet)
			return;

		s_aAlphabet = {};
		for (int i = 0; i < 128; i++)
		{
			s_aAlphabet.Insert(-1);
		}

		string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		for (int j = 0; j < chars.Length(); j++)
		{
			s_aAlphabet[chars.Get(j).ToAscii()] = j;
		}
	}
}

//! One in-flight fetch.
//!
//! A class rather than a pair of static functions because the callback has to
//! remember which key it was fetching, and RestCallback carries no state of its
//! own beyond the response.
class MCF_Device_ImageFetch
{
	protected string m_sKey;
	protected ref RestCallback m_Callback;

	void MCF_Device_ImageFetch(string key)
	{
		m_sKey = key;

		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnSuccess);
		m_Callback.SetOnError(OnError);
	}

	RestCallback GetCallback()
	{
		return m_Callback;
	}

	protected void OnSuccess(RestCallback cb)
	{
		MCF_Device_ImageCache.StoreFromBase64(m_sKey, cb.GetData());
		MCF_Device_ImageCache.FinishFetch(m_sKey);
	}

	//! A failed fetch is not an error worth shouting about. A player behind a
	//! firewall has no photograph, which the shell has to treat as normal.
	protected void OnError(RestCallback cb)
	{
		MCF_Core_Log.Debug("device image '" + m_sKey + "' could not be fetched, http "
			+ cb.GetHttpCode().ToString());

		MCF_Device_ImageCache.FinishFetch(m_sKey);
	}
}
