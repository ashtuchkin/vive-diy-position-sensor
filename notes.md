
### TODO

Next:
 * [ ] Add FTM input
 * [ ] Rework docs.
 * [ ] Add outputs, geo objects and coord conversions to settings
 * [ ] Output point distance.
 * [ ] Make USB Serial switchable between debug io and regular mode.
 * [ ] Assertion/termination system.

Later:
 * [ ] Create multi-sensor geometry processing unit
 * [ ] Increase precision by keeping an estimate of cycle and removing uncertainty of long pulses.
 * [ ] Remove Timestamp in favor of std::chrono::duration (http://en.cppreference.com/w/cpp/chrono/duration)
 * [ ] Remove Vector in favor of std::vector.
 * [ ] Rewrite _sbrk() to not allow heap to go to into stack.
 * [ ] Add unit testing
 * [ ] DataFrame: Check CRC32; Decode values.
 * [ ] Split PersistentSettings to Settings and Persistent<>
 * [ ] (Maybe) Introduce EASTL library and all its niceties like fixed_vector. Tried it and it looks problematic (platform not supported + threading issues).

### Style guide
https://google.github.io/styleguide/cppguide.html

### Git submodule management
 * Update submodules to latest on branches/tags: `git submodule update --remote`
 * When changing .gitmodules, do `git submodule sync`

### GCC Headers search for #include <..>

    /usr/local/Caskroom/gcc-arm-embedded/6_2-2016q4,20161216/gcc-arm-none-eabi-6_2-2016q4
      /arm-none-eabi/include/c++/6.2.1
      /arm-none-eabi/include/c++/6.2.1/arm-none-eabi
      /arm-none-eabi/include/c++/6.2.1/backward
      /lib/gcc/arm-none-eabi/6.2.1/include
      /lib/gcc/arm-none-eabi/6.2.1/include-fixed
      /arm-none-eabi/include
