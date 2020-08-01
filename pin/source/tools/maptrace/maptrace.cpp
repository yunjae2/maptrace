/* vim: set shiftwidth=4 softtabstop=4 tabstop=4: */
/*
 * Copyright 2002-2019 Intel Corporation.
 *
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/*! @file
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <iomanip>
using std::string;
using std::hex;
using std::ios;
using std::setw;
using std::cerr;
using std::dec;
using std::endl;

#define DEBUG 0

/* ===================================================================== */
/* Signitures for memory allocator functions
 *
 * 4 MSBs in addr are used for the signitures
 *
 * 0000: memory ref
 * 1000: malloc
 * 1001: calloc
 * 1010: realloc
 * 1011: free
 * 1100: mmap?
 * 1111: icount
 */
#define ClearSign(_addr)			((_addr) & ((0x1UL << 60) - 1))
#define SetSign(_addr, _sign)		(ClearSign(_addr) | ((_sign) << 60))

#define SignRef(_addr)				SetSign(_addr, 0x0UL);
#define SignMalloc(_addr)			SetSign(_addr, 0x8UL);
#define SignCalloc(_addr)			SetSign(_addr, 0x9UL);
#define SignRealloc(_addr)			SetSign(_addr, 0xaUL);
#define SignFree(_addr)				SetSign(_addr, 0xbUL);
#define SignMmap(_addr)				SetSign(_addr, 0xcUL);
#define SignIcount(_addr)			SetSign(_addr, 0xfUL);

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

std::ofstream TraceFile;
#if DEBUG
std::ofstream DebugTraceFile;
#endif

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
		"o", "maptrace.out", "specify trace file name");
KNOB<string> KnobFuncs(KNOB_MODE_APPEND, "pintool",
		"f", "foo", "Target function names to trace");

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

static INT32 Usage()
{
	cerr <<
		"This tool produces a memory address trace.\n"
		"For each (dynamic) instruction reading or writing to memory the the ip and ea are recorded\n"
		"\n";

	cerr << KNOB_BASE::StringKnobSummary();

	cerr << endl;

	return -1;
}

static VOID RecordMem(VOID * addr, INT32 size)
{
	ADDRINT address = SignRef((ADDRINT) addr);

#if DEBUG
	DebugTraceFile << addr << " " << size << endl;
#endif
	TraceFile.write((char *)&address, sizeof(ADDRINT));
	TraceFile.write((char *)&size, sizeof(INT32));
}

static VOID * WriteAddr;
static INT32 WriteSize;

static VOID RecordWriteAddrSize(VOID * addr, INT32 size)
{
	WriteAddr = addr;
	WriteSize = size;
}

static VOID RecordMemWrite(VOID)
{
	RecordMem(WriteAddr, WriteSize);
}

/* ===================================================================== */

BOOL is_target(RTN rtn)
{
	UINT32 nr_funcs = KnobFuncs.NumberOfValues();
	UINT32 i;

	for (i = 0; i < nr_funcs; i++)
	{
		if (RTN_Name(rtn) == KnobFuncs.Value(i))
			return true;
	}

	return false;
}

INT64 activated = 0;

VOID Activate(VOID)
{
	activated++;
}

VOID Deactivate(VOID)
{
	activated--;
}

VOID Instruction(INS ins, VOID *v)
{
	if (activated <= 0)
		return;

	if (INS_IsStackRead(ins) || INS_IsStackWrite(ins))
		return;

	if (!INS_IsStandardMemop(ins))
		return;

	// instruments loads using a predicated call, i.e.
	// the call happens iff the load will be actually executed
	if (INS_IsMemoryRead(ins))
	{
		INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
				IARG_MEMORYREAD_EA,
				IARG_MEMORYREAD_SIZE,
				IARG_END);
	}

	if (INS_HasMemoryRead2(ins))
	{
		INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
				IARG_MEMORYREAD2_EA,
				IARG_MEMORYREAD_SIZE,
				IARG_END);
	}

	// instruments stores using a predicated call, i.e.
	// the call happens iff the store will be actually executed
	if (INS_IsMemoryWrite(ins))
	{
		INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordWriteAddrSize,
				IARG_MEMORYWRITE_EA,
				IARG_MEMORYWRITE_SIZE,
				IARG_END);

		if (INS_IsValidForIpointAfter(ins))
		{
			INS_InsertCall(
					ins, IPOINT_AFTER, (AFUNPTR)RecordMemWrite,
					IARG_END);
		}
		if (INS_IsValidForIpointTakenBranch(ins))
		{
			INS_InsertCall(
					ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)RecordMemWrite,
					IARG_END);
		}
	}
}

VOID Routine(RTN rtn, void *v)
{
	if (is_target(rtn)) {
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Activate, IARG_END);
		RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)Deactivate, IARG_END);
		RTN_Close(rtn);
	}
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
	TraceFile.close();
#if DEBUG
	DebugTraceFile.close();
#endif
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
	PIN_InitSymbols();

	if( PIN_Init(argc,argv) )
	{
		return Usage();
	}

	TraceFile.open(KnobOutputFile.Value().c_str(), ios::out | ios::binary);
#if DEBUG
	DebugTraceFile.open("posetrace_debug.out");
	DebugTraceFile << hex;
	DebugTraceFile.setf(ios::showbase);
#endif

	RTN_AddInstrumentFunction(Routine, 0);
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	// Never returns

	PIN_StartProgram();

	RecordMemWrite();
	RecordWriteAddrSize(0, 0);

	return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
