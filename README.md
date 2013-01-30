## What is this ##
Efficient and fast sound (i.e. record) recognition software.

## Where can You use it ##
* You can listen to live radio station and log how many your advertisements are
  played per day
* You can scan your music library for multiple recordings
* You can match special operator messages when trying to initiate a VoIP call
  (for example, "phone is out of radio coverage")

## Compilation ##

You need to have sox (and optionally, jack) installed in your system. On Unix
platforms:

    $ cmake .
    $ make

See `INSTALL.mingw` for instructions how to do it for Windows.

## Usage ##

Simplest way to do it:

    $ ./src/soundpatty -qa dump -c config.cfg sample.wav > samplefile.txt
    $ ./src/soundpatty -qacapture -cconfig.cfg -x -ssamplefile.txt catch_me.wav
    FOUND for catch_me.wav, processed 8.467125 sec

Get `sample.wav` and `catch_me.wav` from here:
http://github.com/downloads/Motiejus/SoundPatty/sample.wav
http://github.com/downloads/Motiejus/SoundPatty/catch_me.wav

Sample file (`-s` parameter) is what you get when you do a `dump` action (`-a
dump`), NOT the original audio sample (in fact, audio sample can be acquired
from JACK).

Also, you can try jack like this:

    $ mplayer -ao jack:name=mp 

(in another console)

    $ ./soundpatty -a capture -c config.cfg -s samplefile.txt -d jack mp:out_0

For automatic automatic jack channel capturing:

    $ ./soundpatty -a capture -c config.cfg -s samplefile.txt -d jack -m

This option is suitable for VoIP. You need just to open the Jack port with
callee (caller) audio and SoundPatty will recognize and alert for you :-)

## Percent of Silence in the record ##

SoundPatty can be used to calculate how much silence relatively to record
length was in the record.

    $ ./soundpatty -a aggregate -c silence.cfg a_file.wav

Output:

    295.222, 94.9146%

1. `295.222` is the length of silence in the record, in seconds.
2. `94.9146%` is the percentage of silence in the record. Note that it can be
   derived from the 2'nd value and file length, so this value is there for
   convenience.

### Please note  ###
Create sample files with the same driver you will capture, otherwise capturing
might not work.

## TODO ##

Technical debt:

1. Be [consistent][dsl]. Rewrite config.cfg to exploit Lua (and get rid of that
   ./over.sh).
2. Port old waf tests to cmake.
3. Validate command-line arguments better (see issues page).
4. Run through valgrind.

Algorithmically:

* Make capturing algorithm more tolerant about items in the first positions
  (say, 30% allowed ranges) and more restrictive at the end. This would help
  immensely for determining whether record is "recognize-able" or not.
* Test and substitute map<int, Range>::equal_range in SoundPatty::do_checking,
  performance cost now. O(n) instead of O(log n)
* Logger displays fractions of seconds incorrectly for fractions below 1000

## Gory details ##

This section tries to explain how SoundPatty works behind the scenes.

SoundPatty finds "chunks", i.e. parts of the soundfile (at least "chunklen"
length, as defined in the config file), of which wave peaks are between
treshold_min and treshold_max values, and writes them down (-a dump).

Then it finds similar chunks (with some error) when you run "capture".

That's how SoundPatty works essentially.

For example, if you have `treshold1_min=0.5` and `treshold1_max=1.0`,
periods from soundfile will be collected with the following properties:

* wave peaks will be between 0.5 and 1.0 of the possible volume.
* minimum length of the "chunk" will be "chunklen" seconds.

This is what you get when you run `-a dump`:
`A;B;C`
A is the treshold number (e.g. `treshold1_min` it is 1)
B is the start of the chunk, from the beginning of the file, in seconds
C is the length of the chunk

For example, if you have a sound file like:
1 second of silence (vol. less than 0.05)
0.5 seconds of noise (vol. ~0.3)
1 second of silence (vol. less than 0.05)

And this config:

    treshold0_min: 0
    treshold0_max: 0.1
    treshold1_min: 0.15
    treshold1_max: 0.5
    treshold2_min: 0.6
    treshold2_max: 1

Then when you run "-a dump" you will get something like:

    0;0;1.0 # treshold no. 0, starts at 0'th second, duration 1s
    1;1.0;0.5 # treshold 1, starts at 1 second, duration 0.5s
    0;1.5;1 # treshold 0, starts at 1.5 sec, duration 1s

Other parameters are for the matching algorithm that are hopefully more
understandable.

It all might seem very trivial. Yes It Is. But the whole thing worked
surprisingly well for us.

At first I thought that it will be very file-specific to pick the
tresholds, but for speech-only more or less anything half sensible works
very well (that produces a lot of output in dump file, of course). I
used config.cfg in all production environments (and changing these did
not alter performance very much). If your files are speech, I would
recomment to stick to it. If you experience bad performance, look at the
sound plots and see why it goes wrong (and send me some samples for my
inspection, I might give some clues).


And a reply from enlightened user (it may clarify a bit more):


So chunks can be of different sizes, but are always above the minimum
chunk length. The threshold is not a number of dots (as the above
paragraph suggests, even though contracted by the fractional numbers in
config.cfg), but a measurement for peak values - in percentage in
relation to the maximum peak?  Read on, I am writing as I am
learning/understanding.

What is the application of specifying several thresholds - to make the
resulting chunks smaller, and to segment the audio file into different
peak levels?

> For example, if you have treshold1_min=0.5 and treshold1_max=1.0,
> periods from soundfile will be collected with the following properties:
> * wave peaks will be between 0.5 and 1.0 of the possible volume.
> * minimum length of the "chunk" will be "chunklen" seconds.

Ok, got it, so I can filter out low volume input (noise or GSM
artefacts). With "possible volume" do you refer to the "maximum volume"
of needle.wav (think "normalize"?), or is that simply an encoding
property?

> This is what you get when you run "-a dump":
> A;B;C
> A is the treshold number (for treshold1_min <- it is 1)
> B is the start of the chunk, from the beginning of the file, in seconds
> C is the length of the chunk

Allright, so the thresholds draw horizontal lines on a peak/time graph of
the audio, and the chunks constitute the vertial lines that intersect
with the threshold lines where they hit a threshold boundary.

[dsl]: https://plus.google.com/101059401507686219909/posts/2BUDmhx8CMr

