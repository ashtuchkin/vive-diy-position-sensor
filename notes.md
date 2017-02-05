
### TODO

Until public:
 * [ ] Check it's working.
  
 * [ ] Make Mavlink & Text outputs work
   * [ ] Mavlink - do the filtering?
   * [ ] Conversion to NED.
 * [ ] Update geometry once per 4 phases, or be explicit about it.
 * [ ] Timing: convert to string & microseconds (mavlink).
 * [ ] Assertion system.

 * [ ] Extract the "Pipeline" concept in a separate class.
 * [ ] input.h - make circular buffer non-static; make inputs non static in general.

Next:
 * [ ] Add FTM input
 * [ ] Rework docs.
 * [ ] Add outputs to settings
 * [ ] Move settings to their particular nodes
 * [ ] Make USB Serial switchable between debug io and regular mode.

Later:
 * [ ] Create multi-sensor geometry processing unit
 * [ ] Add unit testing
 * [ ] DataFrame: Check CRC32; Decode values.


### Style guide
https://google.github.io/styleguide/cppguide.html

### Git submodule management
 * Update submodules to latest on branches/tags: `git submodule update --remote`
 * When changing .gitmodules, do `git submodule sync`
