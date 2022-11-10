declare name "Distributed WFS";
declare description "Basic WFS for a distributed setup consisting of modules that each handle two output channels.";
import("stdfaust.lib");

// Speed of sound (c)
CELERITY = 343;
// max Y distance, in meters, that a source can be from the speaker array
MAX_Y_DIST = 10;
// Number of samples it takes sound to travel one meter.
SAMPLES_PER_METRE = ma.SR/CELERITY;
// Number of samples it takes sound to traverse the speaker array
MAX_DELAY = N_SPEAKERS*SPEAKER_DIST*SAMPLES_PER_METRE;
// Number of speakers in the speaker array.
N_SPEAKERS = 16;
// Each module (Teensy) controls two speakers.
SPEAKERS_PER_MODULE = 2;
// distance (m) between individual speakers: TODO: MEASURE AND ADJUST.
SPEAKER_DIST = 0.25;
// Number of sound sources (i.e. mono channels)
N_SOURCES = 2;

// Set which speakers to control.
moduleID = hslider("moduleID", 0, 0, (N_SPEAKERS / SPEAKERS_PER_MODULE) - 1, 1);

// Simulate distance by changing gain and applying a lowpass as a function
// of distance
// TODO: reinstate gain (and lowpass)
distanceSim(distance) = *(dGain) : fi.lowpass(2, fc)
with{
    // Use inverse square law; I_2/I_1 = (d_1/d_2)^2
    // Assume sensible listening distance of 5 m from array.
    i1 = 1.; // Intensity 1...
    d1 = 5.; // ...at distance 5 m
    d2 = d1 + distance;
    i2 = i1 * (d1/d2)^2; //  
    dGain = i2;
    // dGain = (MAX_Y_DIST - distance*.5)/(MAX_Y_DIST);

    fc = dGain*15000 + 5000;
};

// Create a speaker array *perspective* for one source
speakerArray(x, y) = _ <: 
    par(i, SPEAKERS_PER_MODULE, distanceSim(hypotenuse(i)) : de.fdelay(MAX_DELAY, smallDelay(i)))
with{
    hypotenuse(j) = (x - (SPEAKER_DIST*(j + moduleID*2)))^2 + y^2 : sqrt;
    smallDelay(j) = (hypotenuse(j) - y)*SAMPLES_PER_METRE;
};

// Take each source...
sourcesArray(s) = par(i, ba.count(s), ba.take(i + 1, s) : 
    // ...and distribute it across the speaker array for this module.
    speakerArray(x(i), y(i)))
    :> par(i, SPEAKERS_PER_MODULE, _)
with{
    // Use normalised input co-ordinate space; scale to dimensions.

    // X position lies on the width of the speaker array
    x(p) = hslider("%p/x", 0, 0, 1, 0.001) : si.smoo : *(SPEAKER_DIST*N_SPEAKERS);
    // Y position is from zero (on the array) to a quasi-arbitrary maximum.
    y(p) = hslider("%p/y", 0, 0, 1, 0.001) : si.smoo : *(MAX_Y_DIST);
};

// Distribute input channels (i.e. sources) across the sources array.
process = sourcesArray(par(i, N_SOURCES, _));