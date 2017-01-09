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
  Statiton station;

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

  // TODO(keir): These two may not be needed.
  int rising_edge_time_us;
  int width_us;

  // Only valid for sync pulses.
  int ootx_bit;

  // Only valid for TYPE_SWEEP. If time is zero, then no matching sync pulse
  // was available to match against.
  int time_from_matching_sync;  // FIXME units
};


class LighthousePulseClassifier {
 public:
   bool NotifyRawPulse(int rising_edge_time_us,
                       int width_us,
                       Pulse* classified_pulse)
 
 private:
  // Some work needed here :)
};
