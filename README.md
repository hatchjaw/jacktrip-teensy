# Teensy as a JackTrip client

## Building

The project is built with [Platformio](https://platformio.org). 
Supported hardware: Teensy 4.1; a host machine running Ubuntu 20.04.

```shell
# Clone the repo
git clone https://github.com/hatchjaw/jacktrip-teensy
# Build the program and upload it to teensy
pio run -e client[N]
```
where `N` is the client number. See [platformio.ini](platformio.ini).

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

PlatformIO is configured to upload by default:

```ini
[env]
;...
targets = upload
```

The various `client` working environments assist with assigning distinct MAC
address and IP to Teens(y|ies). PlatformIO also defines `AUDIO_BLOCK_SAMPLES`
which sets Teensy's audio block size.

### Hardware

Teens(y|ies), running this program, with ethernet shield connected, should be 
attached, by an ethernet cable, to a computer running a jacktrip server.

### Ethernet

The wired connection on the machine running the jacktrip server should be
set to manual IPv4 mode (i.e. DHCP disabled), with **subnet mask** 
`255.255.255.0`, **address** matching `serverIP` as specified in `main.cpp`, 
and **gateway** `x.x.x.1` (where `x` are the first three octets of **address**).

If in doubt, try:

- subnet mask: `255.255.255.0`
- gateway: `192.168.1.1`
- address: `192.168.1.2`

## Running

If you're using an external audio interface, connect it. Open Cadence, click 
_Configure_, navigate to _Driver_, and select the appropriate _Output Device_
(probably something like "hw:USB,0 [USB Audio]"). Select the sample rate that
matches the value being sent with each jacktrip UDP packet, as specified in
[JackTripClient.h](src/JackTripClient.h).

Verify, either via Cadence or QJackCtl that Jack is running, and 
doing so at the desired sample rate.

After uploading to a Teensy (`pio run -e [working environment]`), 
the program will wait for a serial connection (if the `WAIT_FOR_SERIAL` 
define is set).
Start a jacktrip server with `jacktrip -S -q2`.
Then you can open a serial connection to (a) Teensy and the program will
resume (`pio device monitor -p /dev/ttyACMX`).

Play some audio in an application — e.g. Audacity — for which it is possible to 
select jack as the output device. Then use QJackCtl or Cadence (Catia) to route
audio from that application to the client, i.e. Teensy.
In Audacity specifically, select JACK Audio Connection Kit as the host, and
`__ffff_[clientIP]` as the output device.

Plug some
headphones into Teensy and hear the audio that's being delivered from the
server over UDP. 
Route audio from the client to system playback; plug some headphones into your 
computer and hear audio that's being sent from Teensy to the server.

## Notes

### Jacktrip protocol (Hub mode)

- TCP connection to exchange UDP ports between client and server
- Client starts to send UDP packets, the server uses the header to initialize jack parameters

## TODO

- [ ] Fix clock drift
  - ...except it doesn't appear to be a problem after all.
- [ ] Conform to Teensy naming conventions?
- [ ] Autoconfiguration with OSC
  - or an audio channel containing control data
- [ ] Quantify latency
- [ ] Arbitrary audio buffer size, number of channels, bit-depth.