# EmonTx Shield Continuous Monitoring Firmware

This firmware uses EmonTx continuous monitoring, and exposes the data to an I2C
bus (as a passive node). Multiple EmonTx devices can be connected to the bus,
and a main device with network connectivity exposes the measured values to a
logging server.

For more details about the project where this code is used, check the
[emonpub project](https://github.com/llpamies/emonpub).
