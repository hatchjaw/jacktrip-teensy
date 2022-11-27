import ("stdfaust.lib");
gain = (hslider("gain", 1, 0, 1, .01));
process = *(gain);