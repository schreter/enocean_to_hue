# Bridge from Enocean sensors to Philips Hue

This project provides a minimalistic bridge which understands Enocean
protocol spoken by Enocean USB300 gateway and reacts on switch presses
by setting a virtual sensor in Philips Hue bridge to a specified value.
This way, it is possible to integrate for instance Eltako switches to
control Philips Hue lightning system.

## Invocation

`enocean_to_hue <usb300 port> <bridge IP> <API key> <sensor ID> <mapping file>`

Parameters:
   - `<usb300 port>` - USB300 Enocean USB stick serial port (typically /dev/ttyUSBx)
   - `<bridge IP>` - IP address of Philips Hue bridge
   - `<API key>` - API key of the Hue bridge (see https://developers.meethue.com/develop/get-started-2/)
   - `<sensor ID>` - ID of a sensor to which post the state
   - `<mapping file>` - file with mappings of switches/sensors to a value

## Syntax of the mapping file

The mapping file is parsed as text lines:
   - lines starting with '#' are treated as comments
   - empty lines are ignored
   - `<device id> <button> <state>` - set a mapping for a device's button

Example mapping file:
```
# living room switch - map buttons 1-8 to 11-18 and release to 10
fe:f2:37:a3 -1 10

# switch kitchen - explicit mapping of buttons
fe:f2:37:88 2 21
fe:f2:37:88 4 22
fe:f2:37:88 3 23
fe:f2:37:88 1 24

# office switch - map buttons 1-8 to 31-38 and release to 30
fe:f1:7b:67 -1 30

# bathroom door contact
01:c5:e2:89 0 1000	# open
01:c5:e2:89 1 1001	# closed
```
