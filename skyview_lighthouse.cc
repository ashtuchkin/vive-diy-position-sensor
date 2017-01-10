// Author: mierle@gmail.com (Keir Mierle)

#include "skyview_lighthouse.h"

bool DetectSyncPulse(int rising_edge_timestamp, int pulse_width,
                     ClassifiedPulse* classified_pulse) {
  // TODO
  return false;
}

bool DetectSweepPulse(int rising_edge_timestamp, int pulse_width) {
  return false;
}

LighthousePulseClassifier::LighthousePulseClassifier(float time_units_per_us) {
}

bool LighthousePulseClassifier::AddPulse(int rising_edge_timestamp,
                                         int pulse_width,
                                         ClassifiedPulse* classified_pulse) {

  if (DetectSyncPulse(rising_edge_timestamp, pulse_width, classified_pulse)) {
    classified_pulse->type = ClassifiedPulse::TYPE_SYNC;
    // Sync pulse detected & details computed.
    // TODO - put it into the "pulse-with-sweep-incoming" slot
  } else if (DetectSweepPulse(rising_edge_timestamp, pulse_width)) {
    // This is the hard part; need to look back at the last event that is
    // supposed to send a sweep. If it's there, and the time is within
    // reasonable bounds, then classify this as a sweep.
    classified_pulse->type = ClassifiedPulse::TYPE_SWEEP;
  } else {
    // Ruh-roh, the pulse is looking strange. Do error handling here.
  }
  return false;
}

// Conversion table for sync pulses:
//
//      skip    data    axis    ticks   time (us)
//  j0    0       0       0     3000      62.5
//  k0    0       0       1     3500      72.9
//  j1    0       1       0     4000      83.3
//  k1    0       1       1     4500      93.8
//  j2    1       0       0     5000      104
//  k2    1       0       1     5500      115
//  j3    1       1       0     6000      125
//  k3    1       1       1     6500      135

