# Converts embedded/user_config.conf (see example in example/example.conf) to embedded/user_config_map.hpp

import re

ADDR_REGEX = r'^([0-9a-fA-F]{2}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2})$'
ADDR_MAP = {}

def addMapping(addr, button, value):
    if button < -2 or button > 8:
        raise Exception("Button ID must be in the range [-2..8]")
    if button == -1:
        for idx in range(1, 9):
            addMapping(addr, idx, value + idx)
    elif button == -2:
        for idx in range(0, 9):
            addMapping(addr, idx, value + idx)
    else:
        if not addr in ADDR_MAP:
            ADDR_MAP[addr] = {}
        ADDR_MAP[addr][button] = value

with open("embedded/user_config.conf", "r") as source:
    for line in source.read().splitlines():
        if line.startswith('#') or line == '':
            continue
        else:
            # decode address number action
            tuple = line.split()
            if len(tuple) < 3:
                raise Exception("Invalid tuple, expected 3 elements: " + str(tuple))
            # decode address
            if not re.match(ADDR_REGEX, tuple[0]):
                raise Exception("Invalid address " + tuple[0] + ", expected ##:##:##:## where # is a hex digit")
            addr = int(re.sub(ADDR_REGEX, '0x\\1\\2\\3\\4', tuple[0]), 16)
            button = int(tuple[1])
            value = int(tuple[2])
            addMapping(addr, button, value)

with open("embedded/user_config_map.hpp", "w") as out:
    out.write("static int32_t map_action(uint32_t addr, uint8_t button)\n{\n switch (addr) {\n")
    for addr in ADDR_MAP.keys():
        out.write("    case 0x{:x}:\n      switch (button) {{\n".format(addr))
        buttons = ADDR_MAP[addr]
        for button in buttons.keys():
            out.write("        case {:d}: return {:d};\n".format(button, buttons[button]))
        out.write("        default: return 0;\n      }\n")
    out.write("    default: return 0;\n  }\n}\n")

print "Mapping written into embedded/user_config_map.hpp, all done"