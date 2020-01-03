# Bridge from Enocean sensors to Philips Hue

This project provides a minimalistic bridge which understands Enocean
protocol spoken by Enocean USB300 gateway and reacts on switch presses
by setting a virtual sensor in Philips Hue bridge to a specified value.
This way, it is possible to integrate for instance Eltako switches to
control Philips Hue lightning system.

The program also supports multiple Hue bridges as destination of commands,
even sending the same command to multiple bridges.


## Invocation

`enocean_to_hue <usb300 port> <mapping file> <bridge IP> <API key> <sensor ID> [<bridge IP> <API key> <sensor ID>]...`

Parameters:
   - `<usb300 port>` - USB300 Enocean USB stick serial port (typically /dev/ttyUSBx)
   - `<mapping file>` - file with mappings of switches/sensors to a value
   - `<bridge IP>` - IP address of Philips Hue bridge
   - `<API key>` - API key of the Hue bridge (see https://developers.meethue.com/develop/get-started-2/)
   - `<sensor ID>` - ID of a sensor to which post the state

## Syntax of the mapping file

The mapping file is parsed as text lines:
   - lines starting with '#' are treated as comments
   - empty lines are ignored
   - `<device id> <button> <state>` - set a mapping for a device's button
   - `bridge <index> [<index>]...` - set bridge indices which will get following commands

Button numbers:
   - 0 - release of a button
   - 1 - top-left button
   - 2 - bottom-left button
   - 3 - top-right button
   - 4 - bottom-right button
   - 5 - simultaneous press of both top buttons or top button of a single-rocker
   - 6 - simultaneous press of both bottom buttons or bottom button of a single-rocker
   - 7 - simultaneous press of top-left and bottom-right button
   - 8 - simultaneous press of top-right and bottom-left button

The mapping can also contain special button numbers:
   - -1 to map all button numbers 1-8 to specified value + button number
   - -2 to map all button numbers 0-8 (i.e., also button release event)
     to specified value + button number
   - -3 to map all button numbers 1-8 to specified value + button number
     and to map button release to negative value + button number

State to set must be positive integer, except for button 0, where state -1 is allowed
(in this case, the state set is the negated value of the last button pressed, i.e.,
you can detect release of the button).

Example mapping file:
```
# send commands to bridge 1
bridge 1

# living room switch - map buttons 1-8 to 11-18
fe:f2:37:a3 -1 10

# switch kitchen - explicit mapping of buttons
fe:f2:37:88 2 21
fe:f2:37:88 4 22
fe:f2:37:88 3 23
fe:f2:37:88 1 24

# office switch - map buttons 1-8 to 31-38 and release to 30
fe:f1:7b:67 -2 30

# bathroom door contact
01:c5:e2:89 0 1000	# open
01:c5:e2:89 1 1001	# closed

# all-off command sent to multiple bridges
bridge 1 2
fe:f1:7b:33 1 99
```

