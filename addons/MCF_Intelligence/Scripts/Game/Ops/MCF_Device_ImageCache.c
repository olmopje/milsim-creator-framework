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
//!   RestContext.GET                text from a URL, http 200 in 70 ms
//!                                  -- see MCF_Device_ImageFetch for the two
//!                                  things that have to be right about it.
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

//! Where one picture has got to.
//!
//! WHY THE UI NEEDS THIS AT ALL. A photograph that has not arrived and one that
//! is never going to arrive look identical on screen: an empty space. The first
//! time that happened it was read as a bug, and it took a log dive to find out
//! that nothing had gone wrong at all -- the fetch simply had not finished. So
//! the cache reports its own state and the shell says which of the two it is.
enum MCF_EImageState
{
	NONE,     //!< Never asked for, or asked for and forgotten.
	LOADING,  //!< A request is out.
	READY,    //!< On disk, drawable.
	FAILED    //!< Asked for, came back wrong or not at all.
}

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
	//! The requests that are out, by key.
	//!
	//! THE FETCH OBJECT, NOT THE CALLBACK. Holding the RestCallback alone is not
	//! enough and the difference is invisible: the handlers are methods ON the
	//! fetch object, so when that object is collected the callback survives with
	//! nothing left to call. The request completes, http 200, and script never
	//! hears about it. Measured on 2026-09-10 -- a probe holding its own wrapper
	//! got the same URL back in 70 ms while this map, holding only the callback,
	//! had been silent for half an afternoon.
	protected static ref map<string, ref MCF_Device_ImageFetch> s_aPending = new map<string, ref MCF_Device_ImageFetch>();

	//! Keys that were asked for and did not arrive. Remembered so the shell can
	//! stop waiting and say so, and cleared whenever the same key is asked for
	//! again -- a player whose network came back deserves a second try.
	protected static ref map<string, bool> s_aFailed = new map<string, bool>();

	//! Where a cached image lives, whether or not it is there yet.
	//!
	//! The key is the caller's name for the picture, not the URL: a URL contains
	//! characters a path may not, and two devices naming the same photograph
	//! should share one file.
	static string PathFor(string key)
	{
		return FOLDER + Sanitise(key) + ".png";
	}

	//! Where a picture's dimensions are remembered.
	//!
	//! A SEPARATE LITTLE FILE, because the shape has to survive a restart. The
	//! aspect is known only while decoding, and a player who opens the same phone
	//! tomorrow has the picture on disk and no idea what shape it is. Guessing
	//! 4:3 for a 16:9 photograph is visible from across the room.
	protected static string SizePathFor(string key)
	{
		return FOLDER + Sanitise(key) + ".size";
	}

	//! Width divided by height, or 0 when it is not known.
	static float Aspect(string key)
	{
		if (key.IsEmpty())
			return 0;

		string path = SizePathFor(key);
		if (!FileIO.FileExists(path))
			return 0;

		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return 0;

		string line;
		file.ReadLine(line);
		file.Close();

		array<string> parts = {};
		line.Split(" ", parts, true);
		if (parts.Count() < 2)
			return 0;

		float width = parts[0].ToInt();
		float height = parts[1].ToInt();

		if (width <= 0 || height <= 0)
			return 0;

		return width / height;
	}

	static bool IsCached(string key)
	{
		return FileIO.FileExists(PathFor(key));
	}

	//! Where one picture has got to. See MCF_EImageState.
	static int StateOf(string key)
	{
		if (key.IsEmpty())
			return MCF_EImageState.NONE;

		if (IsCached(key))
			return MCF_EImageState.READY;

		if (s_aPending.Contains(key))
			return MCF_EImageState.LOADING;

		if (s_aFailed.Contains(key))
			return MCF_EImageState.FAILED;

		return MCF_EImageState.NONE;
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

		int width, height;
		if (Measure(bytes, width, height))
		{
			WriteSize(key, width, height);
			MCF_Core_Log.Debug("device image '" + key + "' cached, " + written.ToString()
				+ " byte(s), " + width.ToString() + "x" + height.ToString());
		}
		else
		{
			MCF_Core_Log.Debug("device image '" + key + "' cached, " + written.ToString()
				+ " byte(s), dimensions unreadable");
		}

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
	//! Fetches a picture named by its whole address.
	//!
	//! THE ONE DOOR. Two screens want pictures now -- a phone and the planning
	//! board -- and a third will. Splitting the url at each call site is how the
	//! two drift apart, so the splitting lives here and a caller passes what a
	//! mission maker typed.
	static void FetchUrl(string url)
	{
		string address = MCF_Device_Script.Trim(url);
		if (address.IsEmpty())
			return;

		int schemeEnd = address.IndexOf("//");
		if (schemeEnd < 0)
		{
			MCF_Core_Log.Warn("picture url '" + address + "' has no scheme -- it needs one, e.g. https://host/file.txt");
			return;
		}

		int slash = address.IndexOfFrom(schemeEnd + 2, "/");
		if (slash < 0)
		{
			MCF_Core_Log.Warn("picture url '" + address + "' has no path -- it needs one, e.g. https://host/file.txt");
			return;
		}

		string host = address.Substring(0, slash);
		string path = address.Substring(slash, address.Length() - slash);

		Fetch(host, path, address);
	}

	static void Fetch(string host, string path, string key)
	{
		// EVERY EXIT SAYS WHY. This method used to return silently in six places,
		// and when a picture failed to appear the log had not one line about it --
		// which made an ordinary missing field indistinguishable from a broken
		// engine call. Diagnosability is cheap here and was expensive to be
		// without.
		if (host.IsEmpty() || path.IsEmpty() || key.IsEmpty())
		{
			MCF_Core_Log.Warn("device image fetch asked for with an empty host, path or key -- host='"
				+ host + "' path='" + path + "' key='" + key + "'");
			return;
		}

		if (IsCached(key))
		{
			MCF_Core_Log.Debug("device image '" + key + "' already on disk");
			return;
		}

		// One request per key at a time. Two devices showing the same photograph
		// would otherwise both fetch it, and the second write could land while
		// the first is still open.
		if (s_aPending.Contains(key))
		{
			MCF_Core_Log.Debug("device image '" + key + "' already being fetched");
			return;
		}

		// A retry clears the previous verdict, so a player whose network came
		// back is not told forever that the picture is unavailable.
		s_aFailed.Remove(key);

		RestApi rest = GetGame().GetRestApi();
		if (!rest)
		{
			MCF_Core_Log.Warn("device image '" + key + "': no RestApi in this context -- nothing can be fetched here");
			s_aFailed.Insert(key, true);
			return;
		}

		RestContext context = rest.GetContext(host);
		if (!context)
		{
			MCF_Core_Log.Warn("device image '" + key + "': no RestContext for " + host);
			s_aFailed.Insert(key, true);
			return;
		}

		context.SetTimeout(20);

		MCF_Device_ImageFetch fetch = new MCF_Device_ImageFetch(key);
		s_aPending.Insert(key, fetch);

		context.GET(fetch.GetCallback(), path);

		MCF_Core_Log.Debug("device image '" + key + "': GET " + host + path);
	}

	//! Called by the fetch when it is over, either way.
	//!
	//! The verdict is remembered, not just the fact that it finished: a shell
	//! that is showing a spinner has to be told to stop, and the only difference
	//! between "keep waiting" and "give up" is this flag.
	static void FinishFetch(string key, bool ok)
	{
		s_aPending.Remove(key);

		if (!ok)
			s_aFailed.Insert(key, true);
	}

	//! Throws the whole cache away. Nothing calls this yet; it exists because a
	//! mod that writes to a player's disk should be able to stop.
	static void Clear(notnull array<string> keys)
	{
		foreach (string key : keys)
		{
			FileIO.DeleteFile(PathFor(key));
			FileIO.DeleteFile(SizePathFor(key));
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

	//! The picture's own idea of its size, read out of its header.
	//!
	//! Both formats say it plainly and neither needs decoding to find out. A png
	//! puts it in the IHDR chunk at a fixed offset; a jpeg puts it in whichever
	//! start-of-frame marker it happens to use, so the markers are walked until
	//! one of them is a frame header.
	protected static bool Measure(notnull array<int> bytes, out int width, out int height)
	{
		width = 0;
		height = 0;

		int count = bytes.Count();

		if (count > 24 && bytes[0] == 137 && bytes[1] == 80)
		{
			width = (bytes[16] << 24) | (bytes[17] << 16) | (bytes[18] << 8) | bytes[19];
			height = (bytes[20] << 24) | (bytes[21] << 16) | (bytes[22] << 8) | bytes[23];
			return width > 0 && height > 0;
		}

		if (count < 4 || bytes[0] != 255 || bytes[1] != 216)
			return false;

		int i = 2;
		while (i + 9 < count)
		{
			if (bytes[i] != 255)
			{
				i++;
				continue;
			}

			int marker = bytes[i + 1];

			// C0 to CF are the frame headers, except C4 (huffman tables), C8 and
			// CC, which are not frames at all despite sitting in the same range.
			if (marker >= 192 && marker <= 207 && marker != 196 && marker != 200 && marker != 204)
			{
				height = (bytes[i + 5] << 8) | bytes[i + 6];
				width = (bytes[i + 7] << 8) | bytes[i + 8];
				return width > 0 && height > 0;
			}

			// These carry no length field, so there is nothing to skip over.
			if (marker == 1 || marker == 216 || marker == 217 || (marker >= 208 && marker <= 215))
			{
				i = i + 2;
				continue;
			}

			int length = (bytes[i + 2] << 8) | bytes[i + 3];
			if (length < 2)
				return false;

			i = i + 2 + length;
		}

		return false;
	}

	protected static void WriteSize(string key, int width, int height)
	{
		FileHandle file = FileIO.OpenFile(SizePathFor(key), FileMode.WRITE);
		if (!file)
			return;

		file.WriteLine(width.ToString() + " " + height.ToString());
		file.Close();
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
//! THE SETTERS, NOT A SUBCLASS. Bohemia's REST API Usage page shows a
//! RestCallback subclass overriding OnSuccess / OnError / OnTimeout. That page
//! is older than the engine: those virtuals compile with "'OnSuccess' is
//! obsolete: Use RestCallback.SetOnSuccess() instead."
//!
//! THE HANDLER SHAPE IS FIXED AND THE COMPILER WILL NAME IT. The setters take a
//! method matching the prototype 'RestCallbackFunc', which is one argument, the
//! callback itself. Anything else is rejected by name:
//!
//!   OnSuccess(string data, int dataSize)  ->  "too many arguments"
//!   OnError(int errorCode)                ->  "argument 'errorCode' is not
//!                                              compatible"
//!   SetOnTimeout                          ->  "Undefined function"
//!
//! So the body is asked for afterwards, through the callback that was handed
//! back, and there is no timeout hook at all -- a request that dies quietly
//! dies quietly, which is why the shell keeps a deadline of its own.
//!
//! AND THE OBJECT OWNING THE HANDLERS HAS TO OUTLIVE THE CALL -- not just the
//! callback. The documentation says "If callback is not stored as ref then it
//! will be deleted after its execution finishes", which is true and is only
//! half of it. The handlers are methods on THIS object; hold the RestCallback
//! alone and it survives with nothing left to call. There is no error for that.
//! The request completes, the server answers 200, and script hears nothing --
//! which is indistinguishable from a request that never went out, and cost most
//! of an afternoon to tell apart. So the pending map holds the fetch, and the
//! fetch holds the callback.
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

	void OnSuccess(RestCallback callback)
	{
		string data = callback.GetData();

		MCF_Core_Log.Debug("device image '" + m_sKey + "': " + data.Length().ToString()
			+ " character(s) came back, http " + callback.GetHttpCode().ToString());

		bool ok = MCF_Device_ImageCache.StoreFromBase64(m_sKey, data);
		MCF_Device_ImageCache.FinishFetch(m_sKey, ok);
	}

	//! A failed fetch is not an error worth shouting about. A player behind a
	//! firewall has no photograph, which the shell has to treat as normal.
	void OnError(RestCallback callback)
	{
		MCF_Core_Log.Debug("device image '" + m_sKey + "' could not be fetched, http "
			+ callback.GetHttpCode().ToString());

		MCF_Device_ImageCache.FinishFetch(m_sKey, false);
	}
}
