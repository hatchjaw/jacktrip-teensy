# PIR 9 - Audio

## Building

The project is built with [Platformio](https://platformio.org). 
Supported hardware: Teensy 4.1

```shell
# Clone the repo
git clone https://github.com/hatchjaw/jacktrip-teensy
# Build the program and upload it to teensy
pio run -t upload
```

## Setup

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

### Jack

JackTrip uses Jack as its audio server. Install or update Jack as per the
instructions 
[here](https://qjackctl.sourceforge.io/qjackctl-index.html#Installation).

### QJackControl / Cadence

You might get on just fine with QJackControl.
[Cadence](https://kx.studio/Applications:Cadence) potentially offers a better
experience if you need to connect to an external audio interface (perhaps
because it's not possible to change the sampling rate of your built-in audio
device). Install Cadence as per the 
[instructions](https://github.com/falkTX/Cadence/blob/master/INSTALL.md)

### PlatformIO

Install platformIO's 
[udev rules](https://docs.platformio.org/en/latest/core/installation/udev-rules.html)
or [Teensy's](https://www.pjrc.com/teensy/loader_linux.html), but not both.

Select the 'teensy41' build profile, and, if using PlatformIO under
CLion, remove the 'Build' prelaunch step for PlatformIO run/debug 
configurations — better still, just use `pio run -t upload` to build and upload
to teensy from the terminal.

### Hardware

Teensy, running this program, with ethernet shield connected, should be 
attached, by an ethernet cable, to a computer running a jacktrip server.

### Ethernet

The wired connection on the machine running the jacktrip server should be
set to manual IPv4 mode (i.e. DHCP disabled), with **subnet mask** 
`255.255.255.0`, **address** matching `peerAddress` as specified in `main.cpp`, 
and **gateway** `x.x.x.1` (where `x` are the first three octets of **address**).

If in doubt, try:

- subnet mask: `255.255.255.0`
- gateway: `192.168.1.1`
- address: `192.168.1.2`

### Build options

The fields and defines at the start of `main.cpp` are options which may be useful to tinker with (e.g. DHCP or manual ip
configuration).

## Running

If you're using an external audio interface, connect it. Open Cadence, click 
_Configure_, navigate to _Driver_, and select the appropriate _Output Device_
(probably something like "hw:USB,0 [USB Audio]"). Select the sample rate that
matches the value being sent with each jacktrip UDP packet, as specified in
`main.cpp`.

Verify, either via Cadence or QJackCtl that Jack is running, and 
doing so at the desired sample rate.

After uploading to the Teensy (`pio run -t upload`), the program will wait 
for a serial connection (if the `WAIT_FOR_SERIAL` define is set).
Start a jacktrip server with `jacktrip -S -q 2` (maybe with `-p 2` also).
Then you can open a serial connection to Teensy and the program will
resume (`pio device monitor`).

Play some audio in an application — e.g. Audacity — for which it is possible to 
select jack as the output device. Then use QJackCtl or Cadence (Catia) to route
audio from that application to the client, i.e. Teensy.
In Audacity specifically, select JACK Audio Connection Kit as the host, and
`__ffff_[ip of teensy]` as the output device.

Plug some
headphones into Teensy and hear the audio that's being delivered from the
server over UDP. 
Route audio from the client to system playback; plug some headphones into your 
computer and hear audio that's being sent from Teensy to the server.

## Notes

### Jacktrip protocol (Hub mode)

- TCP connection to exchange UDP ports between client and server
- Client starts to send UDP packets, the server uses the header to initialize jack parameters
- When at least a second client is connected, the server starts broadcasting the mixed audio to everyone.

### TODO

- Transmit audio
- Filter microphone audio
- Autoconfiguration with OpenSoundControl
- Restart strategy after receiving an exit packet (currently need a program reset)
