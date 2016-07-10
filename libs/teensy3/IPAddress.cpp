#include "Arduino.h"
#include "IPAddress.h"

size_t IPAddress::printTo(Print& p) const
{
	int i=0;
	while (1) {
		p.print(_address.bytes[i], DEC);
		if (++i >= 4) return 4;
		p.write('.');
	}
}

bool IPAddress::fromString(const char *address)
{
	unsigned int acc = 0; // Accumulator
	unsigned int dots = 0;

	while (*address) {
		char c = *address++;
		if (c >= '0' && c <= '9') {
			acc = acc * 10 + (c - '0');
			if (acc > 255) {
				// Value out of [0..255] range
				return false;
			}
		} else if (c == '.') {
			if (dots == 3) {
				// Too much dots (there must be 3 dots)
				return false;
			}
			_address.bytes[dots++] = acc;
			acc = 0;
		} else {
			// Invalid char
			return false;
		}
	}
	if (dots != 3) {
		// Too few dots (there must be 3 dots)
		return false;
	}
	_address.bytes[3] = acc;
	return true;
}

