#ACID500

No kickstart required.

The ACID500 skidtool is an ode to the Amiga home computer.

This project contains the Musashi 68K emulator (Copyright 1998-2002 Karl Stenerud) MIT license 2013

The memory address AMI_BASE drives chip select on ami16 virtual system on a chip.

enum ami_mem_map {
	AMI_BASE = 0x800000,
	EXEC_BASE = 0x801000,
	DOS_BASE = 0x802000,
	INTUITION_BASE = 0x803000,
	NONVOLATILE_BASE = 0x804000,
	GRAPHICS_BASE = 0x805000,
	MATHFFP_BASE = 0x806000,
	WORKBENCH_BASE = 0x80c000,
	TASK_BASE = 0x80e000
};

The ami16 class decodes library bits and maps a skeleton of their API to ACID500 native implementations.


[test logs](../MyACID500/log.txt)
