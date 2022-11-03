# Teensy as a JackTrip client

## Hello

This project is built with [PlatformIO](https://platformio.org). 
Supported hardware: Teensy 4.1; a host machine running Ubuntu 20.04.

```shell
# Clone the repo
git clone https://github.com/hatchjaw/jacktrip-teensy
# Build the program and upload it to teensy
pio run (--upload-port /dev/ttyACM<n>)
# ...and monitor serial output
pio device monitor (-p /dev/ttyACM<n>)
```

## Setup/Tools

There's a friendly, high-level
[guide](https://ccrma.stanford.edu/docs/common/IETF.html) 
to Jack and JackTrip on the CCRMA website.

### Jack

JackTrip uses Jack as its audio server. Install or update Jack as per the
instructions
[here](https://qjackctl.sourceforge.io/qjackctl-index.html#Installation).

### JackTrip

Included here as a submodule for reference. Follow installation 
instructions [here](https://jacktrip.github.io/jacktrip/Build/Linux/).
Additionally, it may be necessary to install QT's websockets module:

```shell
sudo apt install libqt5websockets5-dev
```

And, in order to use the gui:

```shell
sudo apt install qml-module-qtquick-controls2
```

### QJackCtl / Cadence

You might get on just fine with QJackCtl.
[Cadence](https://kx.studio/Applications:Cadence) potentially offers a better
experience if you need to connect to an external audio interface (perhaps
because it's not possible to change the sampling rate of your built-in audio
card). Install Cadence as per the 
[instructions](https://github.com/falkTX/Cadence/blob/master/INSTALL.md);
once installed, the tools _Catia_ and _Logs_ are very useful.


### PlatformIO

Install platformIO's 
[udev rules](https://docs.platformio.org/en/latest/core/installation/udev-rules.html)
or [Teensy's](https://www.pjrc.com/teensy/loader_linux.html). 
Both... shouldn't be a problem.

[platformio.ini](platformio.ini) is configured to upload by default, not just build:

```ini
[env]
;...
targets = upload
```

It also defines `AUDIO_BLOCK_SAMPLES` which sets Teensy's audio block size.

It _also_ specifices that the GUI Teensy Loader should be used for uploading.
The CLI version behaves weirdly; it tends to need two runs for the upload
process to work. The GUI app seems to get the job done more reliably. You may, 
however, see the following at the end of the output for `pio run`:
```shell
...
Uploading .pio/build/teensy41/firmware.hex
Hangup
*** [upload] Error 129
```
But relax; if Teensy Loader shows a "Programming" modal
with a progress bar, all should be well.

### Hardware

Connect a computer running a JackTrip hub server to an ethernet switch.
Teens(y|ies), running this program, with ethernet shield connected, should be 
attached, by an ethernet cable, to the switch.

### Teensy

[TyTools](https://github.com/Koromix/tytools) are really useful for working with
multiple Teensies.

### Ethernet

The wired connection on the machine running the JackTrip server should be
set to manual IPv4 mode (i.e. DHCP disabled), with **subnet mask** 
`255.255.255.0`, **address** matching `jackTripServerIP` as specified in `main.cpp`, 
and **gateway** `x.x.x.1` (where `x` are the first three octets of **address**).

If in doubt, try:

- subnet mask: `255.255.255.0`
- gateway: `192.168.1.1`
- address: `192.168.1.2`

## Running

If you're using an external audio interface, connect it. Open Cadence, click 
_Configure_, navigate to _Driver_, and select the appropriate _Output Device_
(probably something like "hw:USB,0 [USB Audio]"). Select the sample rate that
matches the value being sent with each JackTrip UDP packet, as specified in
[JackTripClient.h](src/JackTripClient.h).

Alternatively run a dummy driver with sample rate and buffer size of your 
choosing.

Verify, either via Cadence or QJackCtl that Jack is running, and 
doing so at the desired sample rate/buffer size.

_TODO: add scripts to automate this process_

After uploading to a Teensy (`pio run`), 
the program will wait for a serial connection (if the `WAIT_FOR_SERIAL` 
define is set).
Start JackTrip on the computer in hub server mode with buffer latency of 2
(rather than the default, 4)
```shell
jacktrip -S -q2
``` 
Specify patching mode (no autopatching) and instruct JackTrip to report
packet loss, buffer overflow/underruns with
```shell
jacktrip -S -q2 -p5 -I1
```

Then open a serial connection to Teensy (`pio device monitor`) and the program 
will resume.

Play some audio in an application — e.g. Audacity, Reaper — for which it is
possible to select jack as the output device. Then use QJackCtl or 
Cadence (Catia) to route audio from that application to the client, i.e. Teensy,
which will appear as `__ffff_[clientIP]`.

Plug some
headphones into Teensy and hear the audio that's being delivered from the
server over UDP. 
Route audio from the client to system playback; plug some headphones into your 
computer/audio interface and hear audio that's being sent from Teensy to the 
server.

The client is resilient to the changing state of the server. With a `loop()`
set up as follows:
```c++
JackTripClient jtc;
// setup, etc.
void loop() {
    if (!jtc.isConnected()) {
        jtc.connect(2500);
    }
}
```
if there's no JackTrip server, the client will poll TCP until a server appears
on the designated IP address and port. Similarly, If the server is shut down or
killed, Teensy will attempt to reconnect.

_NB If JackTrip is stopped and restarted, JackTripWorker may complain about "not 
receiving Datagrams (timeout)". As far as I can tell, JackTrip is indeed
receiving datagrams at this point, but `JackTripWorker::mSpawning` is stuck set
to `true`._

## Control via OSC

~~Using [oscsend](https://fukuchi.org/works/oscsend/index.html.en):~~

```shell
oscsend osc.udp://[teensy IP]:[OSC UDP port] /wfs/pos/[n] i [pos]
```

~~Or set up Reaper to send OSC messages, and create a custom .ReaperOSC config 
file (-see `~/.config/REAPER/OSC`)~~

`oscsend` doesn't expose a way to specify the IP of the local network interface,
so it's no good for multicast. Instead, use the app in directory 
`wfs-controller`.

## Notes

### Jacktrip protocol (Hub mode)

- A TCP handshake is used to exchange UDP ports between client and server.
- Client starts to send UDP packets, the server uses the header to initialize 
  jack parameters.
- Don't run jacktrip with loopback auto-patching. Just. No. 

### Alsa

If, between sample rate/buffer size/driver changes, audio locks up, to force
Alsa to restart:

```shell
name@comp:~$ lsof | grep pcm
sh 5079 name 70u CHR 116,6 13639 /dev/snd/pcmC0D0p

name@comp:~$ kill -9 5079
```

## TODO

- [ ] Fix clock drift
- [ ] Conform to Teensy naming conventions?
- [ ] Autoconfiguration with OSC
  - or an audio channel containing control data
- [ ] Quantify latency
- [ ] Arbitrary audio buffer size, number of channels, bit-depth.

## More Notes/Queries
- Sending audio from the server to Teensy, and back to the server, results in
  occasional dropouts; These appear to be delays, either in Teensy sending a
  UDP buffer, or in the JackTripServer receiving it. Either way, they're 
  probably due to the relative lack of sophistication in the JackTripClient 
  implementation.
- Maximum undistorted level for the audio shield's headphone output is 0.8. 
  Sending anything hotter than that (and back into an audio interface) results
  in a mangling of the signal. Something like modularity applied to sample 
  values; it's weird.
- JackTrip's UdpDataProtocol class ultimately derives from QThread. For each
  peer that connects to the server, a new instance of UdpDataProtocol, and
  thus a new thread, is created. Consequently, adding many clients/peers will
  have an effect on performance, and may even be a contributing factor in the
  dropouts described above.
  - JackTrip doesn't support multicast. NetJack does.
- Using a dummy driver it's possible to set a very low buffer size, consequently
  `AUDIO_BLOCK_SAMPLES` can be set as low as 8, with (initial) roundtrip latency
  of ~1.5 ms. 4 samples seems to be too small even for a dummy driver. 8 is a 
  little flakey; 16 can yield round-trip latency of as little as 1.8 ms.