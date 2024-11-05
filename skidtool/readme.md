## skid30 skidtool acid500

This project contains the Musashi 68K emulator (Copyright 1998-2002 Karl Stenerud) MIT license 2013

The memory address AMI_BASE drives chip select on ami16 virtual system on a chip.

```
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
```

The amiga16 class in machine.h decodes library bits and maps a skeleton of their API to native implementations.

The acidmicro acidexec and aciddos classes in skidkick.h provide native implementation.

### Items of interest

rawDoFmt exec.library -522

The ACID 500 generates and runs additional 68000 instructions to keep things optimal.
