/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
 * PSX assembly interpreter.
 */

#include "psxcommon.h"
#include "r3000a.h"
#include "gte.h"
#include "psxhle.h"
#include "psxinterpreter.h"
#include <assert.h>
//#include "debug.h"
#define ProcessDebug()

static int branch = 0;
static int branch2 = 0;
static u32 branchPC;

// These macros are used to assemble the repassembler functions

#ifdef PSXCPU_LOG
#define debugI() PSXCPU_LOG("%s\n", disR3000AF(psxRegs.code, psxRegs.pc)); 
#else
#define debugI()
#endif

// Subsets
void (*psxBSC[64])();
void (*psxSPC[64])();
void (*psxREG[32])();
void (*psxCP0[32])();
void (*psxCP2[64])(struct psxCP2Regs *regs);
void (*psxCP2BSC[32])();

static u32 fetchNoCache(u32 pc)
{
	u32 *code = (u32 *)PSXM(pc);
	return ((code == NULL) ? 0 : SWAP32(*code));
}

/*
Formula One 2001 :
Use old CPU cache code when the RAM location is updated with new code (affects in-game racing)
*/
static struct cache_entry {
	u32 tag;
	u32 data[4];
} ICache[256];

static u32 fetchICache(u32 pc)
{
	// cached?
	if (pc < 0xa0000000)
	{
		// this is not how the hardware works but whatever
		struct cache_entry *entry = &ICache[(pc & 0xff0) >> 4];

		if (((entry->tag ^ pc) & 0xfffffff0) != 0 || pc < entry->tag)
		{
			u32 *code = (u32 *)PSXM(pc & ~0x0f);
			if (!code)
				return 0;

			entry->tag = pc;
			// treat as 4 words, although other configurations are said to be possible
			switch (pc & 0x0c)
			{
				case 0x00: entry->data[0] = SWAP32(code[0]);
				case 0x04: entry->data[1] = SWAP32(code[1]);
				case 0x08: entry->data[2] = SWAP32(code[2]);
				case 0x0c: entry->data[3] = SWAP32(code[3]);
			}
		}
		return entry->data[(pc & 0x0f) >> 2];
	}

	return fetchNoCache(pc);
}

u32 (*fetch)(u32 pc) = fetchNoCache;

static void delayRead(int reg, u32 bpc) {
	u32 rold, rnew;

//	SysPrintf("delayRead at %x!\n", psxRegs.pc);

	rold = psxRegs.GPR.r[reg];
	psxBSC[psxRegs.code >> 26](); // branch delay load
	rnew = psxRegs.GPR.r[reg];

	psxRegs.pc = bpc;

	branch = 0;

	psxRegs.GPR.r[reg] = rold;
	execI(); // first branch opcode
	psxRegs.GPR.r[reg] = rnew;

	psxBranchTest();
}

static void delayWrite(int reg, u32 bpc) {

/*	SysPrintf("delayWrite at %x!\n", psxRegs.pc);

	SysPrintf("%s\n", disR3000AF(psxRegs.code, psxRegs.pc-4));
	SysPrintf("%s\n", disR3000AF(PSXMu32(bpc), bpc));*/

	// no changes from normal behavior

	psxBSC[psxRegs.code >> 26]();

	branch = 0;
	psxRegs.pc = bpc;

	psxBranchTest();
}

static void delayReadWrite(int reg, u32 bpc) {

//	SysPrintf("delayReadWrite at %x!\n", psxRegs.pc);

	// the branch delay load is skipped

	branch = 0;
	psxRegs.pc = bpc;

	psxBranchTest();
}

// this defines shall be used with the tmp 
// of the next func (instead of _Funct_...)
#define _tFunct_  ((tmp      ) & 0x3F)  // The funct part of the instruction register 
#define _tRd_     ((tmp >> 11) & 0x1F)  // The rd part of the instruction register 
#define _tRt_     ((tmp >> 16) & 0x1F)  // The rt part of the instruction register 
#define _tRs_     ((tmp >> 21) & 0x1F)  // The rs part of the instruction register 
#define _tSa_     ((tmp >>  6) & 0x1F)  // The sa part of the instruction register

int psxTestLoadDelay(int reg, u32 tmp) {
	if (tmp == 0) return 0; // NOP
	switch (tmp >> 26) {
		case 0x00: // SPECIAL
			switch (_tFunct_) {
				case 0x00: // SLL
				case 0x02: case 0x03: // SRL/SRA
					if (_tRd_ == reg && _tRt_ == reg) return 1; else
					if (_tRt_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				case 0x08: // JR
					if (_tRs_ == reg) return 2;
					break;
				case 0x09: // JALR
					if (_tRd_ == reg && _tRs_ == reg) return 1; else
					if (_tRs_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				// SYSCALL/BREAK just a break;

				case 0x20: case 0x21: case 0x22: case 0x23:
				case 0x24: case 0x25: case 0x26: case 0x27: 
				case 0x2a: case 0x2b: // ADD/ADDU...
				case 0x04: case 0x06: case 0x07: // SLLV...
					if (_tRd_ == reg && (_tRt_ == reg || _tRs_ == reg)) return 1; else
					if (_tRt_ == reg || _tRs_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				case 0x10: case 0x12: // MFHI/MFLO
					if (_tRd_ == reg) return 3;
					break;
				case 0x11: case 0x13: // MTHI/MTLO
					if (_tRs_ == reg) return 2;
					break;

				case 0x18: case 0x19:
				case 0x1a: case 0x1b: // MULT/DIV...
					if (_tRt_ == reg || _tRs_ == reg) return 2;
					break;
			}
			break;

		case 0x01: // REGIMM
			switch (_tRt_) {
				case 0x00: case 0x01:
				case 0x10: case 0x11: // BLTZ/BGEZ...
					// Xenogears - lbu v0 / beq v0
					// - no load delay (fixes battle loading)
					break;

					if (_tRs_ == reg) return 2;
					break;
			}
			break;

		// J would be just a break;
		case 0x03: // JAL
			if (31 == reg) return 3;
			break;

		case 0x04: case 0x05: // BEQ/BNE
			// Xenogears - lbu v0 / beq v0
			// - no load delay (fixes battle loading)
			break;

			if (_tRs_ == reg || _tRt_ == reg) return 2;
			break;

		case 0x06: case 0x07: // BLEZ/BGTZ
			// Xenogears - lbu v0 / beq v0
			// - no load delay (fixes battle loading)
			break;

			if (_tRs_ == reg) return 2;
			break;

		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: // ADDI/ADDIU...
			if (_tRt_ == reg && _tRs_ == reg) return 1; else
			if (_tRs_ == reg) return 2; else
			if (_tRt_ == reg) return 3;
			break;

		case 0x0f: // LUI
			if (_tRt_ == reg) return 3;
			break;

		case 0x10: // COP0
			switch (_tFunct_) {
				case 0x00: // MFC0
					if (_tRt_ == reg) return 3;
					break;
				case 0x02: // CFC0
					if (_tRt_ == reg) return 3;
					break;
				case 0x04: // MTC0
					if (_tRt_ == reg) return 2;
					break;
				case 0x06: // CTC0
					if (_tRt_ == reg) return 2;
					break;
				// RFE just a break;
			}
			break;

		case 0x12: // COP2
			switch (_tFunct_) {
				case 0x00: 
					switch (_tRs_) {
						case 0x00: // MFC2
							if (_tRt_ == reg) return 3;
							break;
						case 0x02: // CFC2
							if (_tRt_ == reg) return 3;
							break;
						case 0x04: // MTC2
							if (_tRt_ == reg) return 2;
							break;
						case 0x06: // CTC2
							if (_tRt_ == reg) return 2;
							break;
					}
					break;
				// RTPS... break;
			}
			break;

		case 0x22: case 0x26: // LWL/LWR
			if (_tRt_ == reg) return 3; else
			if (_tRs_ == reg) return 2;
			break;

		case 0x20: case 0x21: case 0x23:
		case 0x24: case 0x25: // LB/LH/LW/LBU/LHU
			if (_tRt_ == reg && _tRs_ == reg) return 1; else
			if (_tRs_ == reg) return 2; else
			if (_tRt_ == reg) return 3;
			break;

		case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2e: // SB/SH/SWL/SW/SWR
			if (_tRt_ == reg || _tRs_ == reg) return 2;
			break;

		case 0x32: case 0x3a: // LWC2/SWC2
			if (_tRs_ == reg) return 2;
			break;
	}

	return 0;
}

void psxDelayTest(int reg, u32 bpc) {
	u32 tmp = fetch(bpc);
	branch = 1;

	switch (psxTestLoadDelay(reg, tmp)) {
		case 1:
			delayReadWrite(reg, bpc); return;
		case 2:
			delayRead(reg, bpc); return;
		case 3:
			delayWrite(reg, bpc); return;
	}
	psxBSC[psxRegs.code >> 26]();

	branch = 0;
	psxRegs.pc = bpc;

	psxBranchTest();
}

static u32 psxBranchNoDelay(void) {
	u32 temp;

	psxRegs.code = fetch(psxRegs.pc);
	switch (_Op_) {
		case 0x00: // SPECIAL
			switch (_Funct_) {
				case 0x08: // JR
					return _u32(_rRs_);
				case 0x09: // JALR
					temp = _u32(_rRs_);
					if (_Rd_) { _SetLink(_Rd_); }
					return temp;
			}
			break;
		case 0x01: // REGIMM
			switch (_Rt_) {
				case 0x00: // BLTZ
					if (_i32(_rRs_) < 0)
						return _BranchTarget_;
					break;
				case 0x01: // BGEZ
					if (_i32(_rRs_) >= 0)
						return _BranchTarget_;
					break;
				case 0x08: // BLTZAL
					if (_i32(_rRs_) < 0) {
						_SetLink(31);
						return _BranchTarget_;
					}
					break;
				case 0x09: // BGEZAL
					if (_i32(_rRs_) >= 0) {
						_SetLink(31);
						return _BranchTarget_;
					}
					break;
			}
			break;
		case 0x02: // J
			return _JumpTarget_;
		case 0x03: // JAL
			_SetLink(31);
			return _JumpTarget_;
		case 0x04: // BEQ
			if (_i32(_rRs_) == _i32(_rRt_))
				return _BranchTarget_;
			break;
		case 0x05: // BNE
			if (_i32(_rRs_) != _i32(_rRt_))
				return _BranchTarget_;
			break;
		case 0x06: // BLEZ
			if (_i32(_rRs_) <= 0)
				return _BranchTarget_;
			break;
		case 0x07: // BGTZ
			if (_i32(_rRs_) > 0)
				return _BranchTarget_;
			break;
	}

	return (u32)-1;
}

static int psxDelayBranchExec(u32 tar) {
	execI();

	branch = 0;
	psxRegs.pc = tar;
	psxRegs.cycle += BIAS;
	psxBranchTest();
	return 1;
}

static int psxDelayBranchTest(u32 tar1) {
	u32 tar2, tmp1, tmp2;

	tar2 = psxBranchNoDelay();
	if (tar2 == (u32)-1)
		return 0;

	debugI();

	/*
	 * Branch in delay slot:
	 * - execute 1 instruction at tar1
	 * - jump to tar2 (target of branch in delay slot; this branch
	 *   has no normal delay slot, instruction at tar1 was fetched instead)
	 */
	psxRegs.pc = tar1;
	tmp1 = psxBranchNoDelay();
	if (tmp1 == (u32)-1) {
		return psxDelayBranchExec(tar2);
	}
	debugI();
	psxRegs.cycle += BIAS;

	/*
	 * Got a branch at tar1:
	 * - execute 1 instruction at tar2
	 * - jump to target of that branch (tmp1)
	 */
	psxRegs.pc = tar2;
	tmp2 = psxBranchNoDelay();
	if (tmp2 == (u32)-1) {
		return psxDelayBranchExec(tmp1);
	}
	debugI();
	psxRegs.cycle += BIAS;

	/*
	 * Got a branch at tar2:
	 * - execute 1 instruction at tmp1
	 * - jump to target of that branch (tmp2)
	 */
	psxRegs.pc = tmp1;
	return psxDelayBranchExec(tmp2);
}

static void doBranch(u32 tar) {
	u32 tmp;

	branch2 = branch = 1;
	branchPC = tar;

	// check for branch in delay slot
	if (psxDelayBranchTest(tar))
		return;

	psxRegs.code = fetch(psxRegs.pc);

	debugI();

	psxRegs.pc += 4;
	psxRegs.cycle += BIAS;

	// check for load delay
	tmp = psxRegs.code >> 26;
	switch (tmp) {
		case 0x10: // COP0
			switch (_Rs_) {
				case 0x00: // MFC0
				case 0x02: // CFC0
					psxDelayTest(_Rt_, branchPC);
					return;
			}
			break;
		case 0x12: // COP2
			switch (_Funct_) {
				case 0x00:
					switch (_Rs_) {
						case 0x00: // MFC2
						case 0x02: // CFC2
							psxDelayTest(_Rt_, branchPC);
							return;
					}
					break;
			}
			break;
		case 0x32: // LWC2
			psxDelayTest(_Rt_, branchPC);
			return;
		default:
			if (tmp >= 0x20 && tmp <= 0x26) { // LB/LH/LWL/LW/LBU/LHU/LWR
				psxDelayTest(_Rt_, branchPC);
				return;
			}
			break;
	}

	psxBSC[psxRegs.code >> 26]();

	branch = 0;
	psxRegs.pc = branchPC;

	psxBranchTest();
}

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
void psxADDI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) + _Imm_ ; }		// Rt = Rs + Im 	(Exception on Integer Overflow)
void psxADDIU() { if (!_Rt_) return; _rRt_ = _u32(_rRs_) + _Imm_ ; }		// Rt = Rs + Im
void psxANDI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) & _ImmU_; }		// Rt = Rs And Im
void psxORI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) | _ImmU_; }		// Rt = Rs Or  Im
void psxXORI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) ^ _ImmU_; }		// Rt = Rs Xor Im
void psxSLTI() 	{ if (!_Rt_) return; _rRt_ = _i32(_rRs_) < _Imm_ ; }		// Rt = Rs < Im		(Signed)
void psxSLTIU() { if (!_Rt_) return; _rRt_ = _u32(_rRs_) < ((u32)_Imm_); }		// Rt = Rs < Im		(Unsigned)

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/
void psxADD()	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) + _u32(_rRt_); }	// Rd = Rs + Rt		(Exception on Integer Overflow)
void psxADDU() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) + _u32(_rRt_); }	// Rd = Rs + Rt
void psxSUB() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) - _u32(_rRt_); }	// Rd = Rs - Rt		(Exception on Integer Overflow)
void psxSUBU() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) - _u32(_rRt_); }	// Rd = Rs - Rt
void psxAND() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) & _u32(_rRt_); }	// Rd = Rs And Rt
void psxOR() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) | _u32(_rRt_); }	// Rd = Rs Or  Rt
void psxXOR() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) ^ _u32(_rRt_); }	// Rd = Rs Xor Rt
void psxNOR() 	{ if (!_Rd_) return; _rRd_ =~(_u32(_rRs_) | _u32(_rRt_)); }// Rd = Rs Nor Rt
void psxSLT() 	{ if (!_Rd_) return; _rRd_ = _i32(_rRs_) < _i32(_rRt_); }	// Rd = Rs < Rt		(Signed)
void psxSLTU() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) < _u32(_rRt_); }	// Rd = Rs < Rt		(Unsigned)

/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/
void psxDIV() {
    if (!_i32(_rRt_)) {
        _i32(_rHi_) = _i32(_rRs_);
        if (_i32(_rRs_) & 0x80000000) {
            _i32(_rLo_) = 1;
        } else {
            _i32(_rLo_) = 0xFFFFFFFF;
        }
/*
 * Notaz said that this was "not needed" for ARM platforms and could slow it down so let's disable for ARM. 
 * This fixes a crash issue that can happen when running Amidog's CPU test.
 * (It still stays stuck to a black screen but at least it doesn't crash anymore)
 */
#if !defined(__arm__) && !defined(__aarch64__)
    } else if (_i32(_rRs_) == 0x80000000 && _i32(_rRt_) == 0xFFFFFFFF) {
        _i32(_rLo_) = 0x80000000;
        _i32(_rHi_) = 0;
#endif
    } else {
        _i32(_rLo_) = _i32(_rRs_) / _i32(_rRt_);
        _i32(_rHi_) = _i32(_rRs_) % _i32(_rRt_);
    }
}

void psxDIV_stall() {
	psxRegs.muldivBusyCycle = psxRegs.cycle + 37;
	psxDIV();
}

void psxDIVU() {
	if (_rRt_ != 0) {
		_rLo_ = _rRs_ / _rRt_;
		_rHi_ = _rRs_ % _rRt_;
	}
	else {
		_i32(_rLo_) = 0xffffffff;
		_i32(_rHi_) = _i32(_rRs_);
	}
}

void psxDIVU_stall() {
	psxRegs.muldivBusyCycle = psxRegs.cycle + 37;
	psxDIVU();
}

void psxMULT() {
	u64 res = (s64)((s64)_i32(_rRs_) * (s64)_i32(_rRt_));

	psxRegs.GPR.n.lo = (u32)(res & 0xffffffff);
	psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff);
}

void psxMULT_stall() {
	// approximate, but maybe good enough
	u32 rs = _rRs_;
	u32 lz = __builtin_clz(((rs ^ ((s32)rs >> 21)) | 1));
	u32 c = 7 + (2 - (lz / 11)) * 4;
	psxRegs.muldivBusyCycle = psxRegs.cycle + c;
	psxMULT();
}

void psxMULTU() {
	u64 res = (u64)((u64)_u32(_rRs_) * (u64)_u32(_rRt_));

	psxRegs.GPR.n.lo = (u32)(res & 0xffffffff);
	psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff);
}

void psxMULTU_stall() {
	// approximate, but maybe good enough
	u32 lz = __builtin_clz(_rRs_ | 1);
	u32 c = 7 + (2 - (lz / 11)) * 4;
	psxRegs.muldivBusyCycle = psxRegs.cycle + c;
	psxMULTU();
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/
#define RepZBranchi32(op)      if(_i32(_rRs_) op 0) doBranch(_BranchTarget_);
#define RepZBranchLinki32(op)  { _SetLink(31); if(_i32(_rRs_) op 0) { doBranch(_BranchTarget_); } }

void psxBGEZ()   { RepZBranchi32(>=) }      // Branch if Rs >= 0
void psxBGEZAL() { RepZBranchLinki32(>=) }  // Branch if Rs >= 0 and link
void psxBGTZ()   { RepZBranchi32(>) }       // Branch if Rs >  0
void psxBLEZ()   { RepZBranchi32(<=) }      // Branch if Rs <= 0
void psxBLTZ()   { RepZBranchi32(<) }       // Branch if Rs <  0
void psxBLTZAL() { RepZBranchLinki32(<) }   // Branch if Rs <  0 and link

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
void psxSLL() { if (!_Rd_) return; _u32(_rRd_) = _u32(_rRt_) << _Sa_; } // Rd = Rt << sa
void psxSRA() { if (!_Rd_) return; _i32(_rRd_) = _i32(_rRt_) >> _Sa_; } // Rd = Rt >> sa (arithmetic)
void psxSRL() { if (!_Rd_) return; _u32(_rRd_) = _u32(_rRt_) >> _Sa_; } // Rd = Rt >> sa (logical)

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/
void psxSLLV() { if (!_Rd_) return; _u32(_rRd_) = _u32(_rRt_) << (_u32(_rRs_) & 0x1F); } // Rd = Rt << rs
void psxSRAV() { if (!_Rd_) return; _i32(_rRd_) = _i32(_rRt_) >> (_u32(_rRs_) & 0x1F); } // Rd = Rt >> rs (arithmetic)
void psxSRLV() { if (!_Rd_) return; _u32(_rRd_) = _u32(_rRt_) >> (_u32(_rRs_) & 0x1F); } // Rd = Rt >> rs (logical)

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/
void psxLUI() { if (!_Rt_) return; _u32(_rRt_) = psxRegs.code << 16; } // Upper halfword of Rt = Im

/*********************************************************
* Move from HI/LO to GPR                                 *
* Format:  OP rd                                         *
*********************************************************/
void psxMFHI() { if (!_Rd_) return; _rRd_ = _rHi_; } // Rd = Hi
void psxMFLO() { if (!_Rd_) return; _rRd_ = _rLo_; } // Rd = Lo

static void mflohiCheckStall(void)
{
	u32 left = psxRegs.muldivBusyCycle - psxRegs.cycle;
	if (left <= 37) {
		//printf("muldiv stall %u\n", left);
		psxRegs.cycle = psxRegs.muldivBusyCycle;
	}
}

void psxMFHI_stall() { mflohiCheckStall(); psxMFHI(); }
void psxMFLO_stall() { mflohiCheckStall(); psxMFLO(); }

/*********************************************************
* Move to GPR to HI/LO & Register jump                   *
* Format:  OP rs                                         *
*********************************************************/
void psxMTHI() { _rHi_ = _rRs_; } // Hi = Rs
void psxMTLO() { _rLo_ = _rRs_; } // Lo = Rs

/*********************************************************
* Special purpose instructions                           *
* Format:  OP                                            *
*********************************************************/
void psxBREAK() {
	psxRegs.pc -= 4;
	psxException(0x24, branch);
}

void psxSYSCALL() {
	psxRegs.pc -= 4;
	psxException(0x20, branch);
}

void psxRFE() {
//	SysPrintf("psxRFE\n");
	psxRegs.CP0.n.Status = (psxRegs.CP0.n.Status & 0xfffffff0) |
						  ((psxRegs.CP0.n.Status & 0x3c) >> 2);
	psxTestSWInts();
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
#define RepBranchi32(op)      if(_i32(_rRs_) op _i32(_rRt_)) doBranch(_BranchTarget_);

void psxBEQ() {	RepBranchi32(==) }  // Branch if Rs == Rt
void psxBNE() {	RepBranchi32(!=) }  // Branch if Rs != Rt

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
void psxJ()   {               doBranch(_JumpTarget_); }
void psxJAL() {	_SetLink(31); doBranch(_JumpTarget_); }

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
void psxJR()   {
	doBranch(_rRs_ & ~3);
	psxJumpTest();
}

void psxJALR() {
	u32 temp = _u32(_rRs_);
	if (_Rd_) { _SetLink(_Rd_); }
	doBranch(temp & ~3);
}

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/

#define _oB_ (_u32(_rRs_) + _Imm_)

void psxLB() {
	if (_Rt_) {
		_i32(_rRt_) = (signed char)psxMemRead8(_oB_); 
	} else {
		psxMemRead8(_oB_); 
	}
}

void psxLBU() {
	if (_Rt_) {
		_u32(_rRt_) = psxMemRead8(_oB_);
	} else {
		psxMemRead8(_oB_); 
	}
}

void psxLH() {
	if (_Rt_) {
		_i32(_rRt_) = (short)psxMemRead16(_oB_);
	} else {
		psxMemRead16(_oB_);
	}
}

void psxLHU() {
	if (_Rt_) {
		_u32(_rRt_) = psxMemRead16(_oB_);
	} else {
		psxMemRead16(_oB_);
	}
}

void psxLW() {
	if (_Rt_) {
		_u32(_rRt_) = psxMemRead32(_oB_);
	} else {
		psxMemRead32(_oB_);
	}
}

u32 LWL_MASK[4] = { 0xffffff, 0xffff, 0xff, 0 };
u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };

void psxLWL() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	if (!_Rt_) return;
	_u32(_rRt_) =	( _u32(_rRt_) & LWL_MASK[shift]) | 
					( mem << LWL_SHIFT[shift]);

	/*
	Mem = 1234.  Reg = abcd

	0   4bcd   (mem << 24) | (reg & 0x00ffffff)
	1   34cd   (mem << 16) | (reg & 0x0000ffff)
	2   234d   (mem <<  8) | (reg & 0x000000ff)
	3   1234   (mem      ) | (reg & 0x00000000)
	*/
}

u32 LWR_MASK[4] = { 0, 0xff000000, 0xffff0000, 0xffffff00 };
u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };

void psxLWR() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	if (!_Rt_) return;
	_u32(_rRt_) =	( _u32(_rRt_) & LWR_MASK[shift]) | 
					( mem >> LWR_SHIFT[shift]);

	/*
	Mem = 1234.  Reg = abcd

	0   1234   (mem      ) | (reg & 0x00000000)
	1   a123   (mem >>  8) | (reg & 0xff000000)
	2   ab12   (mem >> 16) | (reg & 0xffff0000)
	3   abc1   (mem >> 24) | (reg & 0xffffff00)
	*/
}

void psxSB() { psxMemWrite8 (_oB_, _rRt_ &   0xff); }
void psxSH() { psxMemWrite16(_oB_, _rRt_ & 0xffff); }
void psxSW() { psxMemWrite32(_oB_, _rRt_); }

u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0 };
u32 SWL_SHIFT[4] = { 24, 16, 8, 0 };

void psxSWL() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	psxMemWrite32(addr & ~3,  (_u32(_rRt_) >> SWL_SHIFT[shift]) |
			     (  mem & SWL_MASK[shift]) );
	/*
	Mem = 1234.  Reg = abcd

	0   123a   (reg >> 24) | (mem & 0xffffff00)
	1   12ab   (reg >> 16) | (mem & 0xffff0000)
	2   1abc   (reg >>  8) | (mem & 0xff000000)
	3   abcd   (reg      ) | (mem & 0x00000000)
	*/
}

u32 SWR_MASK[4] = { 0, 0xff, 0xffff, 0xffffff };
u32 SWR_SHIFT[4] = { 0, 8, 16, 24 };

void psxSWR() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	psxMemWrite32(addr & ~3,  (_u32(_rRt_) << SWR_SHIFT[shift]) |
			     (  mem & SWR_MASK[shift]) );

	/*
	Mem = 1234.  Reg = abcd

	0   abcd   (reg      ) | (mem & 0x00000000)
	1   bcd4   (reg <<  8) | (mem & 0x000000ff)
	2   cd34   (reg << 16) | (mem & 0x0000ffff)
	3   d234   (reg << 24) | (mem & 0x00ffffff)
	*/
}

/*********************************************************
* Moves between GPR and COPx                             *
* Format:  OP rt, fs                                     *
*********************************************************/
void psxMFC0() { if (!_Rt_) return; _i32(_rRt_) = (int)_rFs_; }
void psxCFC0() { if (!_Rt_) return; _i32(_rRt_) = (int)_rFs_; }

void psxTestSWInts() {
	if (psxRegs.CP0.n.Cause & psxRegs.CP0.n.Status & 0x0300 &&
	   psxRegs.CP0.n.Status & 0x1) {
		psxRegs.CP0.n.Cause &= ~0x7c;
		psxException(psxRegs.CP0.n.Cause, branch);
	}
}

void MTC0(int reg, u32 val) {
//	SysPrintf("MTC0 %d: %x\n", reg, val);
	switch (reg) {
		case 12: // Status
			psxRegs.CP0.r[12] = val;
			psxTestSWInts();
			break;

		case 13: // Cause
			psxRegs.CP0.n.Cause &= ~0x0300;
			psxRegs.CP0.n.Cause |= val & 0x0300;
			psxTestSWInts();
			break;

		default:
			psxRegs.CP0.r[reg] = val;
			break;
	}
}

void psxMTC0() { MTC0(_Rd_, _u32(_rRt_)); }
void psxCTC0() { MTC0(_Rd_, _u32(_rRt_)); }

/*********************************************************
* Unknow instruction (would generate an exception)       *
* Format:  ?                                             *
*********************************************************/
void psxNULL() { 
#ifdef PSXCPU_LOG
	PSXCPU_LOG("psx: Unimplemented op %x\n", psxRegs.code);
#endif
}

void psxSPECIAL() {
	psxSPC[_Funct_]();
}

void psxREGIMM() {
	psxREG[_Rt_]();
}

void psxCOP0() {
	psxCP0[_Rs_]();
}

void psxCOP2() {
	psxCP2[_Funct_]((struct psxCP2Regs *)&psxRegs.CP2D);
}

void psxCOP2_stall() {
	u32 f = _Funct_;
	gteCheckStall(f);
	psxCP2[f]((struct psxCP2Regs *)&psxRegs.CP2D);
}

void psxBASIC(struct psxCP2Regs *regs) {
	psxCP2BSC[_Rs_]();
}

void psxHLE() {
//	psxHLEt[psxRegs.code & 0xffff]();
//	psxHLEt[psxRegs.code & 0x07]();		// HDHOSHY experimental patch
    uint32_t hleCode = psxRegs.code & 0x03ffffff;
    if (hleCode >= (sizeof(psxHLEt) / sizeof(psxHLEt[0]))) {
        psxNULL();
    } else {
        psxHLEt[hleCode]();
    }
}

void (*psxBSC[64])() = {
	psxSPECIAL, psxREGIMM, psxJ   , psxJAL  , psxBEQ , psxBNE , psxBLEZ, psxBGTZ,
	psxADDI   , psxADDIU , psxSLTI, psxSLTIU, psxANDI, psxORI , psxXORI, psxLUI ,
	psxCOP0   , psxNULL  , psxCOP2, psxNULL , psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL   , psxNULL  , psxNULL, psxNULL , psxNULL, psxNULL, psxNULL, psxNULL,
	psxLB     , psxLH    , psxLWL , psxLW   , psxLBU , psxLHU , psxLWR , psxNULL,
	psxSB     , psxSH    , psxSWL , psxSW   , psxNULL, psxNULL, psxSWR , psxNULL, 
	psxNULL   , psxNULL  , gteLWC2, psxNULL , psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL   , psxNULL  , gteSWC2, psxHLE  , psxNULL, psxNULL, psxNULL, psxNULL 
};


void (*psxSPC[64])() = {
	psxSLL , psxNULL , psxSRL , psxSRA , psxSLLV   , psxNULL , psxSRLV, psxSRAV,
	psxJR  , psxJALR , psxNULL, psxNULL, psxSYSCALL, psxBREAK, psxNULL, psxNULL,
	psxMFHI, psxMTHI , psxMFLO, psxMTLO, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxMULT, psxMULTU, psxDIV , psxDIVU, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxADD , psxADDU , psxSUB , psxSUBU, psxAND    , psxOR   , psxXOR , psxNOR ,
	psxNULL, psxNULL , psxSLT , psxSLTU, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxNULL, psxNULL , psxNULL, psxNULL, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxNULL, psxNULL , psxNULL, psxNULL, psxNULL   , psxNULL , psxNULL, psxNULL
};

void (*psxREG[32])() = {
	psxBLTZ  , psxBGEZ  , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL  , psxNULL  , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxBLTZAL, psxBGEZAL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL  , psxNULL  , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL
};

void (*psxCP0[32])() = {
	psxMFC0, psxNULL, psxCFC0, psxNULL, psxMTC0, psxNULL, psxCTC0, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxRFE , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL
};

void (*psxCP2[64])(struct psxCP2Regs *regs) = {
	psxBASIC, gteRTPS , psxNULL , psxNULL, psxNULL, psxNULL , gteNCLIP, psxNULL, // 00
	psxNULL , psxNULL , psxNULL , psxNULL, gteOP  , psxNULL , psxNULL , psxNULL, // 08
	gteDPCS , gteINTPL, gteMVMVA, gteNCDS, gteCDP , psxNULL , gteNCDT , psxNULL, // 10
	psxNULL , psxNULL , psxNULL , gteNCCS, gteCC  , psxNULL , gteNCS  , psxNULL, // 18
	gteNCT  , psxNULL , psxNULL , psxNULL, psxNULL, psxNULL , psxNULL , psxNULL, // 20
	gteSQR  , gteDCPL , gteDPCT , psxNULL, psxNULL, gteAVSZ3, gteAVSZ4, psxNULL, // 28 
	gteRTPT , psxNULL , psxNULL , psxNULL, psxNULL, psxNULL , psxNULL , psxNULL, // 30
	psxNULL , psxNULL , psxNULL , psxNULL, psxNULL, gteGPF  , gteGPL  , gteNCCT  // 38
};

void (*psxCP2BSC[32])() = {
	gteMFC2, psxNULL, gteCFC2, psxNULL, gteMTC2, psxNULL, gteCTC2, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL
};


///////////////////////////////////////////

static int intInit() {
	return 0;
}

static void intReset() {
	memset(&ICache, 0xff, sizeof(ICache));
}

void intExecute() {
	extern int stop;
	for (;!stop;) 
		execI();
}

void intExecuteBlock() {
	branch2 = 0;
	while (!branch2) execI();
}

static void intClear(u32 Addr, u32 Size) {
}

void intNotify (int note, void *data) {
	/* Gameblabla - Only clear the icache if it's isolated */
	if (note == R3000ACPU_NOTIFY_CACHE_ISOLATED)
	{
		memset(&ICache, 0xff, sizeof(ICache));
	}
}

void intApplyConfig() {
	assert(psxBSC[18] == psxCOP2  || psxBSC[18] == psxCOP2_stall);
	assert(psxBSC[50] == gteLWC2  || psxBSC[50] == gteLWC2_stall);
	assert(psxBSC[58] == gteSWC2  || psxBSC[58] == gteSWC2_stall);
	assert(psxSPC[16] == psxMFHI  || psxSPC[16] == psxMFHI_stall);
	assert(psxSPC[18] == psxMFLO  || psxSPC[18] == psxMFLO_stall);
	assert(psxSPC[24] == psxMULT  || psxSPC[24] == psxMULT_stall);
	assert(psxSPC[25] == psxMULTU || psxSPC[25] == psxMULTU_stall);
	assert(psxSPC[26] == psxDIV   || psxSPC[26] == psxDIV_stall);
	assert(psxSPC[27] == psxDIVU  || psxSPC[27] == psxDIVU_stall);

	if (Config.DisableStalls) {
		psxBSC[18] = psxCOP2;
		psxBSC[50] = gteLWC2;
		psxBSC[58] = gteSWC2;
		psxSPC[16] = psxMFHI;
		psxSPC[18] = psxMFLO;
		psxSPC[24] = psxMULT;
		psxSPC[25] = psxMULTU;
		psxSPC[26] = psxDIV;
		psxSPC[27] = psxDIVU;
	} else {
		psxBSC[18] = psxCOP2_stall;
		psxBSC[50] = gteLWC2_stall;
		psxBSC[58] = gteSWC2_stall;
		psxSPC[16] = psxMFHI_stall;
		psxSPC[18] = psxMFLO_stall;
		psxSPC[24] = psxMULT_stall;
		psxSPC[25] = psxMULTU_stall;
		psxSPC[26] = psxDIV_stall;
		psxSPC[27] = psxDIVU_stall;
	}

	// dynarec may occasionally call the interpreter, in such a case the
	// cache won't work (cache only works right if all fetches go through it)
	if (!Config.icache_emulation || psxCpu != &psxInt)
		fetch = fetchNoCache;
	else
		fetch = fetchICache;
}

static void intShutdown() {
}

// interpreter execution
void execI() {
	psxRegs.code = fetch(psxRegs.pc);

	debugI();

	if (Config.Debug) ProcessDebug();

	psxRegs.pc += 4;
	psxRegs.cycle += BIAS;

	psxBSC[psxRegs.code >> 26]();
}

R3000Acpu psxInt = {
	intInit,
	intReset,
	intExecute,
	intExecuteBlock,
	intClear,
	intNotify,
	intApplyConfig,
	intShutdown
};
