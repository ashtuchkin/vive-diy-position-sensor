#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "skyview_lighthouse.h"

TEST_CASE("Simple lighthouse classifier", "[pulse_classifier]") {
  LighthousePulseClassifier classifier(10);
  ClassifiedPulse classified_pulse;

  REQUIRE(classifier.AddPulse(10, 20, &classified_pulse) == false);
}
