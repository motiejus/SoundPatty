## What is this ##
Efficient and fast sound (i.e. record) recognition software.

## Where can You use it ##
* You can listen to live radio station and log how many your advertisements are
  played per day
* You can scan your music library for multiple recordings
* You can match special operator messages when trying to initiate a VoIP call
  (for example, "phone is out of radio coverage")

## Usage ##
Let us launch an example test:

    ./waf configure test

This configures, builds, installs SoundPatty to current directory, downloads
`sample.wav` and `catch_me.wav`.  sample.wav is a sample we want to capture in
a `catch_me.wav` stream. Listen to both wavs if unclear :-)

SoundPatty supports any format sox supports and Jack. File inputs usage is
self-descriptive in `./waf test`.

You can try jack like this:

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

    0: 295.222, 94.9146%

1. `0:` means volume treshold. Since there is only one treshold in `silent.cfg`,
   only one value is seen.
2. `295.222` is the length of silence in the record, in seconds.
3. `94.9146%` is the percentage of silence in the record. Note that it can be
   derived from the 2'nd value and file length, so this value is there for
   convenience.

### Please note  ###
Create sample files with the same driver you will capture, otherwise capturing
might not work.

## TODO ##

* Make capturing algorithm more tolerant about items in the first positions
  (say, 30% allowed ranges) and more restrictive at the end. This would help
  immensely for determining whether record is "recognize-able" or not.
* Test and substitute map<int, Range>::equal_range in SoundPatty::do_checking,
  performance cost now. O(n) instead of O(log n)
* Force sample creation and detection with same input driver (jack and file)
* Logger displays fractions of seconds incorrectly for fractions below 1000
* Run through valgrind.
