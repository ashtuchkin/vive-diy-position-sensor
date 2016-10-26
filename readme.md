

# HTC Vive Lighthouse receiver for Teensy.

Uploader: tyc
Use build instructions in it's repo:
https://github.com/Koromix/ty

-------------------------------------------
TODO:
 * Fix incompatibility of teensy3 with gnu-arm-embedded (ambiguating new declaration of 'uint32_t random()')
   * Have a pull request: https://github.com/PaulStoffregen/cores/pull/187
   * Also warning: https://github.com/PaulStoffregen/cores/pull/187
 * Fix dtostrf -> fcvtf warning
 * Find a good name for the project & change the cmake file
 * Write instructions for cmake
 * Support cmake inline build with .gitignore and explicit /bin directory.


Update submodules to latest on branches/tags:
```
git submodule update --remote
```
When changing .gitmodules, don't forget to do `git submodule sync`
