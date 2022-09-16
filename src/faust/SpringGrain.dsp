declare name "SpringGrain";
declare author 	"Tommy Rushton";
import("stdfaust.lib");

NUMGRAINS = 10;
MAXGRAINDURATION = .75;
TSIZE = 44100;

grainStart = hslider("Grain start", 754, 0, TSIZE, 1 );
grainSize = hslider("Grain length (s)", .001, .001, MAXGRAINDURATION, .001);
grainSpeed = hslider("Grain speed",1, -3, 3, 0.01);
grainDensity = hslider("Grain density", 2, .1, 25, .1);
grainRegularity = hslider("Rhythm", 0, 0, 1, 0.01);
freezeWrite = checkbox("Freeze");

// Envelopes for gating grains
//------------------------------------------------------------
envTri(d, t) = en.ar(d/2, d/2, t);
//------------------------------------------------------------
envSin(d, t) = (envTri(d, t)*ma.PI/2) : sin;
//------------------------------------------------------------
envSqrt(d, t) = envTri(d, t) : sqrt;
//------------------------------------------------------------
envHann(d, t) = (1-cos(ma.PI*envTri(d, t)))/2;

// Modulo that handles negative numbers
modulo(b, a) = a : %(b) : +(b) : %(b);

//-------------`(sparsePeriodicTrigger)`-----------------
// Emits +1 impulses ("trigger events") at an average frequency,
// with the distribution adjustable from purely periodic to purely random.
//
// ### Usage
// ```
// sparsePeriodicTrigger(f0, periodicity, pnoise) : _
// ```
//
// Where:
//
// * `f0`: average number of triggers per second.
// * `periodicity`: coefficient of distribution noise.  0 <= periodicity <= 1.  0 = random distribution, 1 = regularly spaced pulses.
// * `pnoise`: random source of probability.  Pure white noise is good.
//
// Courtesy of https://github.com/myklemykle/weather_organ/blob/master/weatherorgan.dsp
//----------------------------
sparsePeriodicTrigger(f0, periodicity, noise) =
  (
    +(rate)               // add the rate;
    <: _, >=(1)           // if greater than 1 ...
    : _, *(1+w*noise) : - // ... subtract 1+(w*noise)
  ) ~ _
  <: _, _' : <            // emit 1 if the value decreased, 0 otherwise.
with {
  w = max(0, min(1, 1 - periodicity));
  rate = f0/ma.SR;
};

sparseTrigger(noiseIndex) = sparsePeriodicTrigger(grainDensity, grainRegularity, no.noises(NUMGRAINS, noiseIndex));

// A grain generator is a lookup table of samples.
grain(trigger, instance, signal) = rwtable(TSIZE, 0., int(writeIndex), signal, int(readIndex))
// It is windowed.
: *(envHann(duration, trigger))
with {
    duration = grainSize;
    // Constantly write input to the table...
    writePos = _ ~ +(1) : %(TSIZE);
    // ...unless 'Freeze' is true.
    writeIndex = ba.if(freezeWrite, 0, writePos);

    // Convert grain size from seconds to samples
    grainLengthSamps = duration * ma.SR;
    // The clock starts when the trigger is not zero, and counts down for the length of the grain.
    clock = max(grainLengthSamps * (trigger != 0)) ~ (-(1) : max(0));

    // Prevent retriggering while the clock is running.
    // uniqueTrigger = ba.if(clock == 0, trigger, 0);

    // Constantly update the read position, within the bounds of the grain size and table size...
    // Set a per-lookup offset.
    grainOffset = int(instance*TSIZE/NUMGRAINS);
    // Add a bit of inter-grain wobble to the sample increment.
    sampleIncrement = grainSpeed * (1 + (instance - (NUMGRAINS/2))/85);
    readPos = _ ~ +(sampleIncrement) : modulo(grainSize * ma.SR) : +(grainStart) : +(grainOffset) : modulo(TSIZE);
    // ...but only update the read poisition if the clock is running.
    readIndex = ba.if(clock > 0, readPos, 0);
};

process = _,_ <: par(i, NUMGRAINS, grain(sparseTrigger(i), i)) :> _,_;