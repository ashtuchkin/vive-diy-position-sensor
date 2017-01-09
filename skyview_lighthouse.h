// Code is so drafty you'll catch a cold standing near it.
// Author: mierle@gmail.com (Keir Mierle)

// Consumes raw lighthouse pulses in the form of rising-edge timestamp and
// pulse width from a single sensor, and parses that into higher-level events,
// like x sweep, y sweep, OOTX data, etc.
class ClassifiedPulse {
  enum Station {
    STATION_MASTER,
    STATION_SLAVE,
  };
  Station station;

  enum Type {
    TYPE_SYNC,
    TYPE_SWEEP,
  };
  Type type;

  enum SweepAxis {
    AXIS_X,
    AXIS_Y,
  };
  SweepAxis sweep_axis;

  // Units are in clock ticks.
  int rising_edge_timestamp;
  int pulse_width;

  // Only valid for TYPE_SWEEP. If time is zero, then no matching sync pulse
  // was available to match against.
  int time_from_matching_sync;

  // Only valid for sync pulses.
  int ootx_bit;
};

class LighthousePulseClassifier {
 public:
  // Times are in clock ticks, but the conversion to microseconds is needed to
  // classify the sync pulses according to the table provided by Valve.
  LighthousePulseClassifier(float time_units_per_us);

  
  // Add a pulse to the classifier, and classify it in the process. If the
  // pulse can't be classified or is unknown, false is returned and the
  // contents of classified_pulse are unchanged. Otherwise, classified_pulse
  // is filled in.
  bool AddPulse(int rising_edge_timestamp,
                int pulse_width,
                ClassifiedPulse* classified_pulse);

 private:
  // Some work needed here :)
};
