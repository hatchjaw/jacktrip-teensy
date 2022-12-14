# Notes

## JACK Settings

### ALSA driver

With interface Focusrite Scarlett 2i2

![ALSA settings](alsa-settings.png)

Any lower than 64/3 doesn't work.

### Dummy driver

![Dummy driver settings](dummy-driver-settings.png)

Note Fs set to 44101 Hz as a means to experiment with clock-drift issues.
JackTrip accepts sampling rates from an enumerated list
of standard rates (32, 44.1, 48 KHz etc.), but accepts Fs +/- 100 Hz.

## Routing

![JACK routing](routing.png)

Reaper outputs 3/4 routed to Teensy JackTrip client inputs (send_1, send_2).
Teensy JackTrip outputs (receive_1, receive_2) routed to Reaper inputs 3/4.

## Round-trip latency

Server running at 44.1/64 using ALSA driver and hardware interface.
Reaper playing 4.41 Hz square wave (top track).
Reaper recording inputs 3/4 from Teensy JTC (bottom track).

Latency ~4 ms

![Round-trip latency](round-trip-1.png)

Detail:
![Round-trip latency](round-trip-detail-1.png)

### Increase over time

Another recording taken ~38 minutes later:

![Round-trip latency](round-trip-1-later.png)

Latency is now ~90 ms

## Packet loss

Server running at 44.1/64 using ALSA driver and hardware interface.
Reaper playing 4.41 Hz square wave (top track).
Reaper recording inputs 3/4 from Teensy JTC (bottom track).

Dropout of exactly 64 samples in the round-trip signal.

![Packet drop](dropout-1.png)

Detail:
![Packet drop](packet-drop-64.png)

Round-trip latency subsequently increases from ~4 ms to ~5 ms.