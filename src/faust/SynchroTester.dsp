import("stdfaust.lib");

// Expect an incoming 16-bit unipolar sawtooth wave x, where
//    max = 1<<15  /// signed int16 max
//   x[n] = (x[n-1] + 1) % max
//     f0 = [sampling rate] / max

// Calculate the offset between the incoming signal and the generated signal.
// To do so, generate a unipolar sawtooth wave y and subtract it from x.
// The offset, d, is
//       / x - y      , y < x
//   d = |
//       \ (x + 1) - y, otherwise
process = _ <: _,-(saw),+(1-saw) : select2(saw >= _)
with {
    INT16MAX = 1<<15;
    saw = os.lf_sawpos_reset(ma.SR / INT16MAX, button("reset"));
};