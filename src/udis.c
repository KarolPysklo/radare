/*
 * Copyright (C) 2007, 2008
 *       pancake <youterm.com>
 *
 * radare is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * radare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with radare; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "main.h"
#include "code.h"
#include "flags.h"
#include "arch/arm/disarm.h"
/* http://devnull.owl.de/~frank/Disassembler_e.html */
#include "arch/ppc/ppc_disasm.h"
#include "arch/m68k/m68k_disasm.h"
#include "list.h"

enum {
	ARCH_X86 = 0,
	ARCH_ARM = 1,
	ARCH_PPC = 2,
	ARCH_M68K = 3,
	ARCH_JAVA = 4
};

static int lines = 0;

static struct reflines_t *reflines = NULL;
struct list_head comments;

/* -- metadata -- */

void metadata_comment_add(u64 offset, const char *str)
{
	struct comment_t *cmt;

	/* no null comments */
	if (strnull(str))
		return;

	cmt = (struct comment_t *) malloc(sizeof(struct comment_t));
	cmt->offset = offset;
	cmt->comment = strdup(str);
	if (cmt->comment[strlen(cmt->comment)-1]=='\n')
		cmt->comment[strlen(cmt->comment)-1]='\0';
	list_add_tail(&(cmt->list), &(comments));
}

void metadata_comment_del(u64 offset, const char *str)
{
	struct list_head *pos;
	u64 off = get_math(str);

	list_for_each(pos, &comments) {
		struct comment_t *cmt = list_entry(pos, struct comment_t, list);
		if (!pos)
			return;
		if (off) {
			if (off == cmt->offset) {
				list_del(pos);
				if (str[0]=='*')
					metadata_comment_del(offset, str);
				return;
			}
		} else {
			if (str[0]=='*') {
				list_del(pos);
				if (str[0]=='*')
					metadata_comment_del(offset, str);
			} else
			if (cmt->offset == offset) {
				list_del(pos);
				return;
			}
		}
	}
}

char *metadata_comment_list()
{
	struct list_head *pos;
	list_for_each(pos, &comments) {
		struct comment_t *cmt = list_entry(pos, struct comment_t, list);
		cons_printf("0x"OFF_FMTx" %s\n", cmt->offset, cmt->comment);
	}
}

char *metadata_comment_get(u64 offset)
{
	struct list_head *pos;
	char *str = NULL;
	int cmtmargin = (int)config_get_i("asm.cmtmargin");
	int cmtlines = config_get_i("asm.cmtlines");
	char null[128];

	memset(null,' ',126);
	null[126]='\0';
	if (cmtmargin<0) cmtmargin=0; else
		// TODO: use screen width here
		if (cmtmargin>80) cmtmargin=80;
	null[cmtmargin] = '\0';
	if (cmtlines<0)
		cmtlines=0;

	if (cmtlines) {
		int i = 0;
		list_for_each(pos, &comments) {
			struct comment_t *cmt = list_entry(pos, struct comment_t, list);
			if (cmt->offset == offset) {
				i++;
			}
		}
		if (i>cmtlines) {
			cmtlines = i-cmtlines;
		}
	}

	list_for_each(pos, &comments) {
		struct comment_t *cmt = list_entry(pos, struct comment_t, list);
		if (cmt->offset == offset) {
			if (cmtlines) {
				cmtlines--;
				continue; // skip comment lines
			}
			if (str == NULL) {
				str = malloc(1024);
				str[0]='\0';
			} else {
				str = realloc(str, cmtmargin+strlen(str)+strlen(cmt->comment)+128);
			}
			strcat(str, null);
			strcat(str, "; ");
			strcat(str, cmt->comment);
			strcat(str, "\n");
			lines++;
		}
	}
	return str;
}

void metadata_comment_init(int new)
{
	INIT_LIST_HEAD(&(comments));
}

static int metadata_print(int delta)
{
	FILE *fd;
	int lines = 0;
	u64 off = 0;
	char *ptr,*ptr2;
	char buf[4096];
	char *rdbfile;
	int show_lines = config_get("asm.lines");
	int show_flagsline = config_get("asm.flagsline");
	int i;
	u64 offset = (u64)config.seek + (u64)delta; //(u64)ud_insn_off(&ud_obj);
	//	u64 seek = config.baddr + (u64)delta; //ud_insn_off(&ud_obj);

	// config.baddr everywhere???
	D {} else return 0;
	if (show_flagsline) {
		ptr = flag_name_by_offset( offset );
		if (ptr[0]) {
			if (show_lines&&reflines)
				code_lines_print2(reflines, config.baddr+config.seek +i);
			C	cons_printf(C_RESET C_BWHITE""OFF_FMT" %s:"C_RESET"\n",
					config.baddr+offset, ptr);
			else	cons_printf(OFF_FMTs" %s:\n",
					config.baddr+offset, ptr);
			lines++;
		}
	}

	ptr = metadata_comment_get(offset);
	if (ptr && ptr[0]) {
		if (show_lines&&reflines)
			code_lines_print2(reflines, config.baddr+config.seek +i);
		C 	cons_printf(C_MAGENTA"%s"C_RESET, ptr);
		else 	cons_printf("%s", ptr);
		free(ptr);
	}

	return lines;
}

#include "arch/x86/udis86/types.h"
#include "arch/x86/udis86/extern.h"

static ud_t ud_obj;
static unsigned char o_do_off = 1;
static int ud_idx = 0;
static int length = 0;

static int input_hook_x(ud_t* u)
{
	if (ud_idx>length)
		return -1;
	if (ud_idx>=config.block_size)
		return -1;
	return config.block[ud_idx++];
}

int udis86_color = 0;

void udis_init()
{
	char *syn = config_get("asm.syntax");
	char *ptr = config_get("asm.arch");

	ud_init(&ud_obj);

	if (!strcmp(ptr, "intel16")) {
		ud_set_mode(&ud_obj, 16);
	} else {
		if((!strcmp(ptr, "intel"))
				|| (!strcmp(ptr, "intel32"))) {
			ud_set_mode(&ud_obj, 32);
		} else {
			if (!strcmp(ptr, "intel64")) {
				ud_set_mode(&ud_obj, 64);
			} else
				ud_set_mode(&ud_obj, 32);
		}
	}

	udis86_color = config.color;

	/* set syntax */
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);
	if (syn) {
		if (!strcmp(syn,"pseudo")) 
			ud_set_syntax(&ud_obj, UD_SYN_PSEUDO);
		else
			if (!strcmp(syn,"att")) 
				ud_set_syntax(&ud_obj, UD_SYN_ATT);
	}

#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
#endif  
	ud_set_input_hook(&ud_obj, input_hook_x);
}

static int jump_n = 0;
static u64 jumps[10]; // only 10 jumps allowed

void udis_jump(int n)
{
	if (n<jump_n) {
		radare_seek(jumps[n], SEEK_SET);
		config.seek = jumps [n];
		radare_read(0);
		undo_push();
	}
}

/* -- disassemble -- */

extern int color;
void udis_arch(int arch, int len, int rows)
{
	struct aop_t aop;
	char* hex1, *hex2;
	char c;
	int i,delta;
	u64 seek = 0;
	int lines = 0;
	int bytes = 0;
	u64 myinc = 0;
	unsigned char b[32];
	char *follow, *cmd_asm;
	int endian;
	int show_size, show_bytes, show_offset,show_splits,show_comments,show_lines,show_traces,show_nbytes, show_flags;

	cmd_asm = config_get("cmd.asm");
	show_size = config_get("asm.size");
	show_bytes = config_get("asm.bytes");
	show_offset = config_get("asm.offset");
	show_splits = config_get("asm.split");
	show_flags = config_get("asm.flags");
	show_lines = config_get("asm.lines");
	show_traces = config_get("asm.trace");
	show_comments = config_get("asm.comments");
	show_nbytes = (int)config_get_i("asm.nbytes");
	endian = config_get("cfg.endian");
	color = config_get("scr.color");

	len*=2; // uh?!
	jump_n = 0;
	length = len;
	ud_idx = 0;
	delta = 0;
	inc = 0;

	if (rows<0)
		rows = 1;

	if (arch == ARCH_X86) {
		udis_init();
		/* disassembly loop */
		ud_obj.pc = config.seek;
	}

	if (show_nbytes>16 ||show_nbytes<0)
		show_nbytes = 16;

	reflines = NULL;
	if (show_lines)
		reflines = code_lines_init();
	radare_controlc();

	while (!config.interrupted) {
		seek = config.baddr +config.seek+bytes;
		if ( (config.visual && len!=config.block_size) && (++lines>=(config.height-2)))
			break;
		// XXX code analyssi doesnt works with endian here!!!1
		// TAKE CARE TAKE CARE TAKE CARE TAKE CARE TAKE CARE
		if (arch != ARCH_X86 && endian) {
			endian_memcpy(b, config.block+bytes, 4);
		} else  memcpy(b, config.block+bytes, 32);

		if (cmd_asm&& cmd_asm[0]) {
			char buf[1024];
			sprintf(buf, "%lld", seek);
			setenv("HERE", buf, 1);
			radare_cmd(cmd_asm, 0);
		}

		switch(arch) {
			case ARCH_X86:
				if (ud_disassemble(&ud_obj)<1)
					return;
				arch_x86_aop((unsigned long)ud_insn_off(&ud_obj), (const unsigned char *)config.block+bytes, &aop);
				myinc = ud_insn_len(&ud_obj);
				break;
			case ARCH_ARM:
				arch_arm_aop(seek, (const unsigned char *)b, &aop); //config.block+bytes, &aop);
				myinc = 4;
				break;
			case ARCH_JAVA:
				arch_java_aop(seek, (const unsigned char *)config.block+bytes, &aop);
				myinc = aop.length;
				break;
			case ARCH_PPC:
				arch_ppc_aop(seek, (const unsigned char *)b, &aop);
				myinc = aop.length;
				break;
			default:
				// Uh?
				myinc = 4;
				break;
				// TODO
		}
		if (myinc<1)
			myinc =1;
		if (config.cursor_mode) {
			if (config.cursor == bytes)
				inc = myinc;
		} else
			if (inc == 0)
				inc = myinc;
		length-=myinc;
		if (length<=0)
			break;

		INILINE;
		D { 
			if (show_comments)
				lines+=metadata_print(bytes);
			if (rows && rows == lines)
				return;

			// TODO autodetect stack frames here !! push ebp and so... and wirte a comment
			if (show_lines)
				code_lines_print(reflines, seek); //config.baddr+ud_insn_off(&ud_obj));

			if (show_offset) {
				C cons_printf(C_GREEN"0x%08llX "C_RESET, (unsigned long long)(seek));//config.baddr + ud_insn_off(&ud_obj)));
				else cons_printf("0x%08llX ", (unsigned long long)(seek)); //config.baddr + ud_insn_off(&ud_obj)));
			}
			/* size */
			if (show_size)
				cons_printf("%d ", aop.length); //dislen(config.block+seek));
			/* trac information */
			if (show_traces)
				cons_printf("%02d %02d ", trace_count(seek), trace_times(seek));

			if (show_flags) {
				char buf[1024];
				char *flag = flag_name_by_offset(seek);
				sprintf(buf, "%%%ds", show_nbytes);
				if (flag) {
					if (strlen(flag)>show_nbytes) {
						cons_printf(buf, flag);
						NEWLINE;
						lines--;
						if (rows && rows == lines)
							return;
						//cons_printf("    ");
						if (show_lines)
							code_lines_print(reflines, seek); //config.baddr+ud_insn_off(&ud_obj));
						if (show_offset) {
							C cons_printf(C_GREEN"0x%08llX "C_RESET, (unsigned long long)(seek));//config.baddr + ud_insn_off(&ud_obj)));
							else cons_printf("0x%08llX ", (unsigned long long)(seek)); //config.baddr + ud_insn_off(&ud_obj)));
						}
						
						cons_printf("        ");
					} else {
						cons_printf(buf, flag);
					}
				} else cons_printf(buf,"");
			}
			/* cursor and bytes */
			if (is_cursor(bytes, myinc)) {
				cons_printf("*");
			} else  cons_printf(" ");
			if (show_bytes) {
				int max = show_nbytes;
				int cur = myinc;
				if (cur + 1> show_nbytes)
					cur = show_nbytes - 1;

				for(i=0;i<cur; i++)
					print_color_byte_i(bytes+i, "%02x", b[i]); //config.block[bytes+i]); //ud_obj.insn_hexcode[i]);
				if (cur !=myinc)
					max--;
				for(i=(max-cur)*2;i>0;i--)
					cons_printf(" ");
				if (cur != myinc)
					cons_printf(". ");
			}
			switch(arch) {
				case ARCH_X86:
					hex1 = ud_insn_hex(&ud_obj);
					hex2 = hex1 + 16;
					c = hex1[16];
					hex1[16] = 0;
					cons_printf("%-24s", ud_insn_asm(&ud_obj));
					hex1[16] = c;
					if (strlen(hex1) > 24) {
						C cons_printf(C_RED);
						cons_printf("\n");
						if (o_do_off)
							cons_printf("%15s .. ", "");
						cons_printf("%-16s", hex2);
					}
					break;
				case ARCH_ARM: {
						       unsigned long ins = (b[0]<<24)+(b[1]<<16)+(b[2]<<8)+(b[3]);
						       cons_printf("  %s", disarm(ins, (unsigned int)seek));
					       } break;
				case ARCH_PPC: {
						       char opcode[128];
						       char operands[128];
						       struct DisasmPara_PPC dp;
						       /* initialize DisasmPara */
						       dp.opcode = opcode;
						       dp.operands = operands;
						       dp.iaddr = seek; //config.baddr + config.seek + i;
						       dp.instr = b; //config.block + i;
						       PPC_Disassemble(&dp, endian);
						       cons_printf("  %s %s", opcode, operands);
					       } break;
				case ARCH_JAVA: {
							char output[128];
							if (java_disasm(b, output)!=-1)
								cons_printf(" %s", output);
							else cons_strcat(" ???");
						} break;
				case ARCH_M68K: {
							char opcode[128];
							char operands[128];
							struct DisasmPara_PPC dp;
							/* initialize DisasmPara */
							dp.opcode = opcode;
							dp.operands = operands;
							dp.iaddr = seek; //config.baddr + config.seek + i;
							dp.instr = b; //config.block + i;
							// XXX read vda68k: this fun returns something... size of opcode?
							M68k_Disassemble(&dp);
							cons_printf("  %s %s", opcode, operands);
						} break;
			}
			C cons_printf(C_RESET);

			if (aop.jump) {
				if (++jump_n<10) {
					jumps[jump_n-1] = aop.jump;
					cons_printf("\e[0m   [%d]", jump_n,jumps[jump_n-1]);
				}
			}

			if (show_splits) {
				if (aop.jump||aop.eob) {
					NEWLINE;
					if (show_lines)
						code_lines_print2(reflines, seek); //config.baddr+ud_insn_off(&ud_obj));
					cons_printf("; ------------------------------------ ");
					lines++;
				}
			}
		} else {
			switch(arch) {
				case ARCH_X86:
					cons_printf("%s", ud_insn_asm(&ud_obj));
					break;
				case ARCH_ARM:
					break;
			}
		}
		seek+=myinc;
		NEWLINE;
		if (rows && rows == lines)
			break;
		bytes+=myinc;
	}
	radare_controlc_end();

}

void disassemble(int len, int rows)
{
	char *ptr = config_get("asm.arch");

	if (ptr == NULL) {
		eprintf("No ARCH defined.\n");
		return;
	}

	radare_controlc();

	/* handles intel16, intel32, intel64 */
	if (!memcmp(ptr, "intel", 5))
		udis_arch(ARCH_X86, len,rows);
	else
	if (!strcmp(ptr, "arm"))
		udis_arch(ARCH_ARM, len,rows);
	else
	if (!strcmp(ptr, "java"))
		udis_arch(ARCH_JAVA, len, rows);
	else
	if (!strcmp(ptr, "ppc"))
		udis_arch(ARCH_PPC, len, rows);
	else
	if (!strcmp(ptr, "m68k"))
		udis_arch(ARCH_M68K, len, rows);

	radare_controlc_end();
	fflush(stdout);
}
