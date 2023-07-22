"use strict"

// a 68000 clone with 2MB memory 
// registers are in order d0-d7 a0-a7

class cpu32{
	constructor(){
		var words=0x100000;
		this.registers=new Int32Array(16);
		this.memory=new Int16Array(words);
		this.pc=0; // *2 for physical address
		this.sp=words;
		this.sr=0xffff;
	}

	poke16(address,value16){
		this.memory[address]=value16;
	}

	poke32(address,value32){
		this.memory[address]=(value32 & 0xfff);
		this.memory[address+1]=(value32 >> 16);
	}

	run(cycles){
		while(cycles){
			var instruction=this.memory[this.pc++];
			


		}
	}
}

function logTitle(t){
	console.log(t);
}
function logObject(o){
	console.log(JSON.stringify(o));
}

function main(){
	console.log("ono");

	console.log("cpu32 perftest");

	var cpu=new cpu32();

	var t0=performance.now();

	var cycles=1e6;

	cpu.run(cycles);

	var t1=performance.now();

	var ms=t1-t0;

	console.dir({cycles,ms});
}
