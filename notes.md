### TODO

 * [ ] Rework docs.
 * [ ] Assertion/termination system for Teensy (clear eeprom if continuous; write exception to usb.)
 * [ ] Add FTM input

Later:
 * [ ] Create multi-sensor geometry processing unit
 * [ ] Add calibration mode to calculate base station geometry. Don't depend on having a full htc vive setup.
 * [ ] Provide .hex files for teensies 3.2 and 3.6. (see #14)
 * [ ] Increase precision by applying geometry adjustments for base stations. 1:1 with Unity.
 * [ ] Create Unity tutorial.
 * [ ] Write articles about timestamps, pipeline/modules, hashing, cross-platform unit testing (see https://news.ycombinator.com/item?id=13691115).
 * [ ] Increase precision by keeping an estimate of cycle and removing uncertainty of long pulses.
 * [ ] Re-check all last-success timestamps (LongTimestamp) - they don't survive the overflow.
 * [ ] Remove Timestamp in favor of std::chrono::duration (http://en.cppreference.com/w/cpp/chrono/duration)
 * [ ] Remove Vector in favor of std::vector.
 * [ ] Add stack overflow protection (or at least find out that it happened).
 * [ ] Cover all major cases with tests
 * [ ] Add polling mode for outputs
 * [ ] DataFrame: Check CRC32.
 * [ ] Split PersistentSettings to Settings and Persistent<>
 * [ ] Get rid of Teensy's Print. Use vsnprintf instead. debug_print, print_def, parse_def, DataChunkPrint
 * [ ] Avoid using double (-Wdouble-promotion). This will require killing all printf-s.
 * [ ] (Maybe) Introduce EASTL library and all its niceties like fixed_vector. Tried it and it looks problematic (platform not supported + threading issues).

### Style guide
https://google.github.io/styleguide/cppguide.html

### Git submodule management
 * Update submodules to latest on branches/tags: `git submodule update --remote`
 * When changing .gitmodules, do `git submodule sync`

### Example Base Station Data Frames

base0: fw 436, id 0x4242089a, desync 16, hw 9, accel [-4, 126, 127], mode B, faults 0 
    fcal0: phase 0.0500, tilt -0.0091, curve -0.0038, gibphase 1.8633, gibmag 0.0113 
    fcal1: phase 0.0241, tilt 0.0008, curve -0.0015, gibphase -0.7920, gibmag -0.0112 (2 total)
base1: fw 436, id 0x2f855022, desync 53, hw 9, accel [0, 118, 127], mode C, faults 0 
    fcal0: phase 0.0284, tilt -0.0036, curve -0.0010, gibphase 2.1758, gibmag 0.0086 
    fcal1: phase 0.0487, tilt -0.0080, curve -0.0023, gibphase 0.4695, gibmag -0.0086 (1 total)


### GCC Headers search for #include <..>

    /usr/local/Caskroom/gcc-arm-embedded/6_2-2016q4,20161216/gcc-arm-none-eabi-6_2-2016q4
      /arm-none-eabi/include/c++/6.2.1
      /arm-none-eabi/include/c++/6.2.1/arm-none-eabi
      /arm-none-eabi/include/c++/6.2.1/backward
      /lib/gcc/arm-none-eabi/6.2.1/include
      /lib/gcc/arm-none-eabi/6.2.1/include-fixed
      /arm-none-eabi/include
