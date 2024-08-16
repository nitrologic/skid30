#pragma once

// exec.h

// bespoke computer interface based on public Amiga interfaces 

// (C) 2024 Simon Armstrong



/* 

*
*   List Node 
*
   STRUCTURE	LN,0	; List Node
	APTR	LN_SUCC	; Pointer to next (successor)
	APTR	LN_PRED	; Pointer to previous (predecessor)
	UBYTE	LN_TYPE
	BYTE	LN_PRI	; Priority, for sorting
	APTR	LN_NAME	; ID string, null terminated
	LABEL	LN_SIZE	; Note: word aligned

*
* List Header
*

   STRUCTURE	LH, 0
	APTR	LH_HEAD
	APTR	LH_TAIL
	APTR	LH_TAILPRED
	UBYTE	LH_TYPE
	UBYTE	LH_pad
	LABEL	LH_SIZE; word aligned

 STRUCTURE  TC_Struct,LN_SIZE	    ; was "TC"
	UBYTE   TC_FLAGS
	UBYTE   TC_STATE
	BYTE    TC_IDNESTCNT	    ; intr disabled nesting
	BYTE    TC_TDNESTCNT	    ; task disabled nesting
	ULONG   TC_SIGALLOC	    ; sigs allocated
	ULONG   TC_SIGWAIT		    ; sigs we are waiting for
	ULONG   TC_SIGRECVD	    ; sigs we have received
	ULONG   TC_SIGEXCEPT	    ; sigs we take as exceptions
	;* Pointer to an extended task structure.  This structure is allocated
	;* by V36 Exec if the proper flags in tc_ETaskFlags are set.  This
	;* field was formerly defined as:
	;*		UWORD	TC_TRAPALLOC	    ; traps allocated
	;*		UWORD	TC_TRAPABLE	    ; traps enabled
	;* Please see the Exec AllocTrap() and FreeTrap() calls.
	;*
	APTR    tc_ETask		    ; pointer to extended task structure
	APTR    TC_EXCEPTDATA	    ; data for except proc
	APTR    TC_EXCEPTCODE	    ; exception procedure
	APTR    TC_TRAPDATA	    ; data for proc trap proc
	APTR    TC_TRAPCODE	    ; proc trap procedure
	APTR    TC_SPREG		    ; stack pointer
	APTR    TC_SPLOWER		    ; stack lower bound
	APTR    TC_SPUPPER		    ; stack upper bound + 2
	FPTR    TC_SWITCH		    ; task losing CPU (function pointer)
	FPTR    TC_LAUNCH		    ; task getting CPU (function pointer)
	STRUCT  TC_MEMENTRY,LH_SIZE     ; Allocated memory list.  Freed by RemTask()
	APTR    TC_Userdata		    ; For use by the task; no restrictions!
	LABEL   TC_SIZE

*/

#include <cstdint>

#define YEH_NA
#ifdef YEH_NA

typedef int8_t BYTE;
typedef uint8_t UBYTE;

typedef uint32_t ULONG;

typedef uint16_t UWORD;
typedef void VOID;

typedef int32_t APTR;
typedef int32_t LABEL;

struct List{
	APTR	LN_SUCC; //Pointer to next (successor)
	APTR	LN_PRED; //Pointer to previous (predecessor)
	UBYTE	LN_TYPE;
	BYTE	LN_PRI; //Priority, for sorting
	APTR	LN_NAME; //ID string, null terminated
	LABEL	LN_SIZE; //Note: word aligned
};

struct Node {
	struct  Node* ln_Succ;	/* Pointer to next (successor) */
	struct  Node* ln_Pred;	/* Pointer to previous (predecessor) */
	UBYTE   ln_Type;
	BYTE    ln_Pri;		/* Priority, for sorting */
	char* ln_Name;		/* ID string, null terminated */
};	/* Note: word aligned */

struct Task {
	struct  Node tc_Node;
	UBYTE   tc_Flags;
	UBYTE   tc_State;
	BYTE    tc_IDNestCnt;	    /* intr disabled nesting*/
	BYTE    tc_TDNestCnt;	    /* task disabled nesting*/
	ULONG   tc_SigAlloc;	    /* sigs allocated */
	ULONG   tc_SigWait;	    /* sigs we are waiting for */
	ULONG   tc_SigRecvd;	    /* sigs we have received */
	ULONG   tc_SigExcept;	    /* sigs we will take excepts for */
	UWORD   tc_TrapAlloc;	    /* traps allocated */
	UWORD   tc_TrapAble;	    /* traps enabled */
	APTR    tc_ExceptData;	    /* points to except data */
	APTR    tc_ExceptCode;	    /* points to except code */
	APTR    tc_TrapData;	    /* points to trap code */
	APTR    tc_TrapCode;	    /* points to trap data */
	APTR    tc_SPReg;		    /* stack pointer	    */
	APTR    tc_SPLower;	    /* stack lower bound    */
	APTR    tc_SPUpper;	    /* stack upper bound + 2*/
	VOID(*tc_Switch)();	    /* task losing CPU	  */
	VOID(*tc_Launch)();	    /* task getting CPU  */
	struct  List tc_MemEntry;	    /* Allocated memory. Freed by RemTask() */
	APTR    tc_UserData;	    /* For use by the task; no restrictions! */
};

class IExec{
public:
	virtual void allocMem() = 0;
	virtual void freeMem() = 0;
	virtual void availMem() = 0;
	virtual void waitPort() = 0;
	virtual void replyMsg() = 0;
	virtual void fakeTask() = 0;
	virtual void openLibrary() = 0;
	virtual void closeLibrary() = 0;
	virtual void openDevice() = 0;
	virtual void doIO() = 0;
	virtual void rawDoFmt() = 0;
	virtual void putMsg() = 0;
	virtual void getMsg() = 0;
	virtual void copyMem() = 0;
	virtual void allocSignal() = 0;
	virtual void setSignal() = 0;
	virtual void forbid() = 0;
	virtual void permit() = 0;
};
class IDos {
public:
	virtual void open()=0;
	virtual void close()=0;
	virtual void read()=0;
	virtual void write() = 0;
	virtual void input() = 0;
	virtual void output() = 0;
	virtual void seek() = 0;
	virtual void deletefile() = 0;
	virtual void rename() = 0;
	virtual void lock() = 0;
	virtual void unLock() = 0;
	virtual void dupLock() = 0;
	virtual void examine() = 0;
	virtual void exnext() = 0;
	virtual void info() = 0;
	virtual void createdir() = 0;
	virtual void currentdir() = 0;
	virtual void ioerr() = 0;
	virtual void createproc() = 0;
	virtual void exit() = 0;
	virtual void isinteractive() = 0;
	virtual void getvar() = 0;
	virtual void loadseg() = 0;
	virtual void unloadseg() = 0;
	virtual void delay() = 0;
	virtual void datestamp() = 0;
	virtual void readargs() = 0;
};
class IBench {
public:
	virtual void closeWorkBench() = 0;
};
class INonVolatile {
public:
	virtual void getCopy() = 0;
};
class IGraphics {
public:
	virtual void textLength() = 0;
	virtual void loadView() = 0;
	virtual void waitTOF() = 0;
	virtual void ownBlitter() = 0;
	virtual void disownBlitter() = 0;
};
class IFFPMath {
public:
};

#endif
