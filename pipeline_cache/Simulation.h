#include<iostream>
#include<stdio.h>
#include<math.h>
#include <unistd.h>

#include<time.h>
#include<stdlib.h>


//----------------------//
#define OP_R 51

#define F3_ADD 0
#define F7_ADD 0

#define F3_MUL 0
#define F7_MUL 1

#define F3_SUB 0
#define F7_SUB 32

#define F3_SLL 1
#define F7_SLL 0

#define F3_MULH 1
#define F7_MULH 1

#define F3_SLT 2
#define F7_SLT 0

#define F3_XOR 4
#define F7_XOR 0

#define F3_DIV 4
#define F7_DIV 1

#define F3_SRL 5
#define F7_SRL 0

#define F3_SRA 5
#define F7_SRA 32

#define F3_OR 6
#define F7_OR 0

#define F3_REM 6
#define F7_REM 1

#define F3_AND 7
#define F7_AND 0
//--------------------//
#define OP_RW 59

#define F3_ADDW 0
#define F7_ADDW 0

#define F3_SUBW 0
#define F7_SUBW 32

#define F3_MULW 0
#define F7_MULW 1

#define F3_DIVW 4
#define F7_DIVW 1

#define F3_REMW 6
#define F7_REMW 1
//---------------------//
#define OP_LW 3

#define F3_LB 0
#define F3_LH 1
#define F3_LW 2
#define F3_LD 3

//---------------------//
#define OP_I 19

#define F3_ADDI 0

#define F3_SLLI 1
#define F7_SLLI 0

#define F3_SLTI 2

#define F3_XORI 4

#define F3_SRLI 5
#define F7_SRLI 0

#define F3_SRAI 5
#define F7_SRAI 16

#define F3_ORI 6

#define F3_ANDI 7
//----------------------//
#define OP_IW 27

#define F3_ADDIW 0

#define F3_SLLIW 1
#define F7_SLLIW 0

#define F3_SRLIW 5
#define F7_SRLIW 0

#define F3_SRAIW 5
#define F7_SRAIW 32
//------------------------//
#define OP_JALR 103

#define F3_JALR 0
//-------------------------//
#define OP_SCALL 115

#define F3_SCALL 0
#define F7_SCALL 0
//--------------------//
#define OP_SW 35

#define F3_SB 0
#define F3_SH 1
#define F3_SW 2
#define F3_SD 3
//------------------------//
#define OP_BRANCH 99

#define F3_BEQ 0
#define F3_BNE 1
#define F3_BLT 4
#define F3_BGE 5
#define F3_BLTU 6
#define F3_BGEU 7
//------------------------//
#define OP_AUIPC 23
//----------------------//
#define OP_LUI 55
//--------------------//
#define OP_JAL 111
//--------------------//


#define MAX 100000000
#define INST_LEN 32

typedef unsigned long long REG;

//主存
//extern unsigned char memory[MAX]={0};
//寄存器堆
REG reg[32]={0};
//PC
int PC=0;

//加载内存
void load_memory();

void IF();
void ID();
void EX();
void MEM();
void WB();


//符号扩展
int ext_signed(unsigned int src, int EXTop, int len);
long long ext_signed_64(unsigned long long src, int EXTop, int len);

//获取指定位
unsigned int getbit(unsigned inst, int s,int e);//???

unsigned int getbit(unsigned inst, int s,int e)
{
	if(s>=0 && e<INST_LEN && s<=e){
		unsigned mask = 0xffffffff;
		return (inst>>s) & (mask>>(INST_LEN-e+s-1));
	}
	return 0;
}

//??????
int ext_signed(unsigned int src, int EXTop, int len)
{
	if(EXTop==0){
		return (int)src;
	}
	else{
		int sign = src>>(len-1);
		int mask = 0xffffffff<<len;
		if(sign){
			return mask|((int)src);
		}
		else
			return (int)src;
	}
}

long long ext_signed_64(unsigned long long src, int EXTop, int len){
	if(EXTop==0){
		return (long long)src;
	}
	else{
		long long sign = src>>(len-1);
		long long mask = ((unsigned long long)(-1))<<len;
		if(sign){
			return mask|((long long)src);
		}
		else
			return (long long)src;
	}
}


enum ALUOP {NONE_ALUOP, ADD, MUL, SUB, SLL, MULH, SLT, XOR, DIV, SRL, SRA, OR, REM, AND};
enum INST_TYPE {NONE_TYPE, R, RW, LW, I, IW, JALR, SCALL, SW, BRANCH, AUIPC, LUI, JAL};
enum BRAN {NONE_BRANCH, BEQ, BNE, BLT, BGE, BLTU, BGEU};
enum MEMOP {NONE_MEMOP, LOAD, STORE, SCALL_print_str, SCALL_scan_int, SCALL_time};
enum STEP {NONE_STEP, IIEW, IIEMW, IIEM, IIE};

struct IFID_REG{
	bool val;
	bool exit;
	unsigned int inst;
	int pc;
}IFID;

struct IDEX_REG{
	bool val;
	bool exit;
	int pc;
	ALUOP aluop;
	INST_TYPE inst_type;
	MEMOP memop;
	STEP step;
	BRAN bran;
	int memsize;
	int rs1, rs2, rd;
	unsigned int imm;
}IDEX;

struct EXMEM_REG{
	//int pc;
	bool val;
	bool exit;
	STEP step;
	unsigned long long int EXout;
	int rd;
	int rs2;
	MEMOP memop;
	int memsize;
}EXMEM;

struct MEMWB_REG{
	bool val;
	bool exit;
	STEP step;
	int rd;
	unsigned long long int MEMout;
}MEMWB;

int readable[32];
int stop;
int jump;
int write_done_reg;