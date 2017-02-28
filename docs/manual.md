# Configuration and Use

## General structure

## Configuration mode
Before this module starts giving meaningful results, it needs to be configured. 
Configuration is a set of commands that can be entered at the command prompt.

.. # comments.  
config>  
led pattern

 Command   | Description
-----------|------------
`reset`    | Clear configuration.
`view`     | Print current configuration. Used to check it and copy/paste to save in a safe place.
`validate` | Check current configuration for errors.
`reload`   | Load configuration from EEPROM.
`write`    | Save configuration to EEPROM. Only after this command Teensy will boot straight to Running mode.
`continue` | Exit configuration mode and proceed to running mode. Configuration is not saved by default; use `write` command for that.

Module configuration:

Module |
-------|
`sensor# pin <pin#> <polarity> <input_type> <input_params>` |
`base0 origin <x> <y> <z> matrix <9xM>` |
`object# [sensor# <x> <y> <z>]+`  |
`stream# position object# > usb_serial` |
`stream# angles > usb_serial` |
`serial# 57600` |
`stream# mavlink object# ned <angle> > serial#`  |

## Running mode
In running mode, the module provides a stream of output data as set by the configuration above.
It can be parsed and used as needed. This is the main operating mode of the module.

Led patterns

To switch into debug mode, press "Enter". This will stop the regular output on USB Serial, 
but continue everything else, like calculating geometry or writing data to other serial ports.

## Debug mode
Debug mode is equivalent to the Running mode with one exception: USB Serial 
stops regular output and instead starts executing debug commands and output debug data.

Almost all debug commands work as switches to enable or disable showing some part of internal state.
"Enabled" data is shown in a single portion mode by default, with a portion shown every time
the user presses the "Enter" key. Continuous output is possible using the "c" command (see below).

debug>

Command | Description
--------|-------------
`o` | Return to Running mode, with regular output to USB Serial.
`!` | Reset Teensy in Configuration mode.
`c [<period>]` | Enable continuous output with given period in milliseconds. Default period: 1 sec. Press "Enter" to return back to single mode.
----|----
`debug mem` | Switches printing debug info about memory and stack usage.
`sensor# pulses <count/show/off>` | Shows/hides pulses registered by given sensor.
`pp angles <count/show/off>` | Shows angles produced by PulseProcessor node.
`pp bits <count/show/off>` | Shows data frame bits produced by PulseProcessor node.
`pp <show/off>` | Show fix and cycle state of PulseProcessor.
`phase <show/off>` | Show fix and phase state of CyclePhaseClassifier.
`dataframe# <count/show/off>` | Show full data frames produced by DataFrameDecoder for a given base station #.
`geom# <count/show/off>` | Show position calculated for a given object.
`stream# <count/show/off>` | Show byte output for a given stream.
`coord <count/show/off>` | TODO
`mavlink <show/off>` | TODO

