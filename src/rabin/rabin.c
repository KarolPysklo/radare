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

#include "../main.h"
#include "rabin.h"
#include <stdio.h>
#if __UNIX__
#include <fcntl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "dietelf.h"
#include "dietelf64.h"

#define ELF_CALL(func, bin, args...) elf64?Elf64_##func(&bin.e64,##args):Elf32_##func(&bin.e32,##args)

typedef union {
    Elf32_dietelf_bin_t e32;
    Elf64_dietelf_bin_t e64;
} dietelf_bin_t;

// TODO : move into rabin_t
char *file = NULL;
int filetype = FILETYPE_UNK;
int action   = ACTION_UNK;
int verbose  = 0;
int xrefs    = 0;
int rad      = 0; //radare output format
int fd       = -1;
static int pebase = 0;
static int elf64 = 0;

/* fun */

int rabin_show_help()
{
	printf(
"rabin [options] [bin-file]\n"
" -e        shows entrypoints one per line\n"
" -i        imports (symbols imported from libraries)\n"
" -s        symbols (exports)\n"
//" -o        others (other symbols)\n"
" -c        header checksum\n"
//" -t        type of binary\n"
" -S        show sections\n"
" -l        linked libraries\n"
" -L [lib]  dlopen library and show address\n"
" -z        search for strings in elf non-executable sections\n"
" -x        show xrefs of symbols (-s/-i required)\n"
" -I        show binary info\n"
" -r        output in radare commands\n"
" -v        be verbose\n");
	return 1;
}

void rabin_show_info(const char *file)
{
	dietelf_bin_t bin;

	switch(filetype) {
	case FILETYPE_ELF:
		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}

		u64 baddr=ELF_CALL(dietelf_get_base_addr,bin);
		
		if (rad) {
			printf("e file.type = elf\n");
			printf("e file.baddr = 0x%08llx\n", baddr);
			if (ELF_CALL(dietelf_is_big_endian,bin))
				printf("e cfg.endian = true\n");
			else
				printf("e cfg.endian = false\n");
			switch (ELF_CALL(dietelf_get_arch,bin)) {
				case EM_MIPS:
				case EM_MIPS_RS3_LE:
				case EM_MIPS_X:
					printf("e asm.arch = mips\n"); break;
				case EM_ARM:
					printf("e asm.arch = arm\n"); break;
				case EM_SPARC:
				case EM_SPARC32PLUS:
				case EM_SPARCV9:
					printf("e asm.arch = sparc\n"); break;
				case EM_PPC:
				case EM_PPC64:
					printf("e asm.arch = ppc\n"); break;
				case EM_68K:
					printf("e asm.arch = m68k\n"); break;
				default: printf("e asm.arch = intel\n");
			}
		} else {
			printf("ELF class:       %s\n"
			       "Data enconding:  %s\n"
			       "OS/ABI name:     %s\n"
			       "Machine name:    %s\n"
			       "File type:       %s\n",
			       ELF_CALL(dietelf_get_elf_class,bin),
			       ELF_CALL(dietelf_get_data_encoding,bin),
			       ELF_CALL(dietelf_get_osabi_name,bin),
			       ELF_CALL(dietelf_get_machine_name,bin),
			       ELF_CALL(dietelf_get_file_type,bin));

			printf("Stripped:        ");
			if (ELF_CALL(dietelf_get_stripped,bin))
			    printf("Yes\n");
			else
			    printf("No\n");

			printf("Static:          ");
			if (ELF_CALL(dietelf_get_static,bin))
			    printf("Yes\n");
			else
			    printf("No\n");
		}

		close(fd);
		break;
	case FILETYPE_CLASS:
		if (rad) {
			printf("e asm.arch = java\n");
			printf("e cfg.endian = true\n");
		}else {
			printf("File type: JAVA CLASS\n");
		}
		break;
	case FILETYPE_PE:
		if (rad)
			printf("e file.type = pe\n");
		else
			printf("File type: PE\n");
		break;
	case FILETYPE_MZ:
		if (rad)
			printf("e file.type = mz\n");
		else
			printf("File type: DOS COM\n");
		break;
	case FILETYPE_DEX:
		if (!rad)
			printf("File type: DEX (google android)\n");
		break;
	case FILETYPE_MACHO:
		if (rad) {
			printf("e file.type = macho\n");
			printf("e asm.arch = ppc\n");
			printf("e cfg.endian = false\n");
		} else
			printf("File type: MACH-O\n");
		break;
	case FILETYPE_CSRFW:
		if (rad) {
			printf("e asm.arch = csr\n");
		} else
			printf("File type: CSR FW\n");
		break;
	case FILETYPE_UNK:
		if (rad)
			printf("e file.type = unk\n");
		else
			printf("File type: UNKNOWN\n");
	}
}

void rabin_show_strings(const char *file)
{
	dietelf_bin_t bin;
	char buf[1024];


	switch(filetype) {
	case FILETYPE_ELF:
		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}
		ELF_CALL(dietelf_list_strings,bin,fd);
		close(fd);
		break;
	case FILETYPE_PE:
		// TODO: native version and support for non -r
		snprintf(buf, 1022, "rsc strings-pe-flag %s",file);
		system(buf);
		break;
	}
	
	//sprintf(buf, "echo /s | radare -e file.id=true -nv %s", file);
	//system(buf);
}

void rabin_show_checksum(const char *file)
{
	unsigned char buf[32];
	unsigned long addr = 0;
	int i;

	switch(filetype) {
	case FILETYPE_DEX:
		lseek(fd, 8, SEEK_SET);
		read(fd, &addr, 4);
		printf("Checksum: 0x%08lx\n", addr);
		read(fd, &buf, 20);
		printf("SHA-1 Signature: ");
		for(i=0;i<20;i++)
			printf("%08x ", buf[i]);
		break;
	case FILETYPE_ELF:
		break;
	case FILETYPE_MZ:
		break;
	case FILETYPE_PE:
		lseek(fd, pebase+0x18, SEEK_SET);
		read(fd, &addr, 4);
		printf("0x%x checksum file offset\n", pebase+0x18);
		printf("0x%04x checksum\n",
			(unsigned int)
			(unsigned short)addr);
		break;
	}
}

void rabin_show_entrypoint()
{
	u64 addr = 0;
	u64 base = 0;
	dietelf_bin_t bin;

	switch(filetype) {
	case FILETYPE_ELF:
#if 0
		lseek(fd, 0x18, SEEK_SET);
		read(fd, &addr, 4);
#endif

		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}
		addr=ELF_CALL(dietelf_get_entry_addr,bin);
		base=ELF_CALL(dietelf_get_base_addr,bin);

		if (rad) {
			printf("fs symbols\n");
			printf("f entrypoint @ 0x%08llx\n", addr - base);
			printf("s entrypoint\n");
		} else {
			if (verbose) {
				printf("0x%08llx memory\n", addr);
				printf("0x%08llx disk\n", addr - base);
			} else {
				printf("0x%08llx\n", addr);
			}
		}

		close(fd);
		break;
	case FILETYPE_MZ:
		break;
	case FILETYPE_PE:
		lseek(fd, pebase+0x28, SEEK_SET);
		read(fd, &addr, 4);
	//	printf("0x%08x disk offset for ep\n", pebase+0x28);
		lseek(fd, pebase+0x45, SEEK_SET);
		read(fd, &base, 4);
		if (rad) {
			printf("f entrypoint @ 0x%08llx\n", addr);
		} else {
			if (verbose) {
				printf("0x%08llx memory\n", base+addr);
				printf("0x%08llx disk\n", addr-0xc00);
			} else	printf("0x%08llx\n", base+addr);
		}
		break;
	}
}

unsigned long addr_for_lib(char *name)
{
#if __UNIX__
	unsigned long *addr = dlopen(name, RTLD_LAZY);
	if (addr) {
		dlclose(addr);
		return (unsigned long)((addr!=NULL)?(*addr):0);
	} else {
		printf("cannot open '%s' library\n", name);
		return 0;
	}
#endif
	return 0;
}

void rabin_show_arch()
{
	u32 dw;
	u16 w;

	switch(filetype) {
	case FILETYPE_ELF:
		lseek(fd, 16+2, SEEK_SET);
		read(fd, &w, 2);
		switch(w) {
		case 3:
			printf("arch: x86-32\n");
			break;
		case 0x28:
			printf("arch: ARM\n");
			break;
		default:
			printf("arch: 0x%x (unknown)\n", w);
			break;
		}
		break;
	case FILETYPE_PE:
		// [[0x3c]+4]
		lseek(fd, 0x3c, SEEK_SET);
		read(fd, &dw, 4);
		lseek(fd, dw+4, SEEK_SET);
		read(fd, &w, 2);
		switch(w) {
		case 0x1c0:
			printf("arch: ARM\n");
			break;
		case 0x14c:
			printf("arch: x86-32\n");
			break;
		default:
			printf("arch: 0x%x (unknown)\n", w);
		}
		break;
	}
}

void rabin_show_imports(const char *file)
{
	//char buf[1024];
	dietelf_bin_t bin;


	switch(filetype) {
	case FILETYPE_ELF:
		//sprintf(buf, "readelf -sA '%s'|grep GLOBAL | awk ' {print $8}'", file);
#if 0
		sprintf(buf, "readelf -s '%s' | grep FUNC | grep GLOBAL | grep DEFAULT  | grep ' UND ' | awk '{ print \"0x\"$2\" \"$8 }' | sort | uniq" , file);
		system(buf);
#else
		
		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}
		ELF_CALL(dietelf_list_imports,bin,fd);
		close(fd);
#endif
		break;
	}
}

void rabin_show_symbols(char *file)
{
	char buf[1024];
	dietelf_bin_t bin;

	switch(filetype) {
	case FILETYPE_ELF:
#if 0		
		sprintf(buf, "readelf -s '%s' | grep FUNC | grep GLOBAL | grep DEFAULT  | grep ' 12 ' | awk '{ print \"0x\"$2\" \"$8 }' | sort | uniq" , file);
		system(buf);
#endif		
		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}
		ELF_CALL(dietelf_list_symbols,bin,fd);
		close(fd);
		break;
	case FILETYPE_MACHO:
		if (rad) {
			system("otool -tv $target|grep -C 1 -e : |grep -v / | awk '{if (/:/){label=$1;gsub(\":\",\"\",label);next}if (label!=\"\"){print \"f sym\"label\" @ 0x\"$1;label=\"\"}}");
		} else {
		   #if __DARWIN_BYTE_ORDER
			sprintf(buf, "nm '%s' | grep ' T ' | sed 's/ T / /' | awk '{print \"0x\"$1\" \"$2}'", file);
			system(buf);
		   #else
			sprintf(buf, "arm-apple-darwin-nm '%s' | grep ' T ' | sed 's/ T / /' | awk '{print \"0x\"$1\" \"$2}'", file);
			system(buf);
		   #endif
		}
		break;
	case FILETYPE_CLASS:
		// TODO: native version and support for non -r
		if (rad)
			snprintf(buf, 1022, "javasm -rc %s",file);
		else
			snprintf(buf, 1022, "javasm -c '%s'", file);
		system(buf);
		break;
	case FILETYPE_PE:
		// TODO: native version and support for non -r
		snprintf(buf, 1022, "rsc syms-pe-flag %s",file);
		system(buf);
		break;
	}
}

#if 0
void rabin_show_others(char *file)
{
	dietelf_bin_t bin;

	switch(filetype) {
	case FILETYPE_ELF:
		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}
		ELF_CALL(dietelf_list_others,bin,fd);
		close(fd);
		break;
	}
}
#endif

void rabin_show_sections(const char *file)
{
	char buf[1024];
	int fd;
	dietelf_bin_t bin;

	switch(filetype) {
	case FILETYPE_ELF:
		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}
		ELF_CALL(dietelf_list_sections,bin,fd);
		close(fd);
		break;
	default:
		// TODO: use the way that rsc flag-sections does
		sprintf(buf, "readelf -S '%s'|grep '\\[' | grep -v '\\[Nr\\]' | cut -c 4- | awk '{ print \"0x\"$4\" \"$2 }'", file);
		system(buf);
	}
}

void rabin_show_libs(const char *file)
{
	char buf[1024];
	int fd;
	dietelf_bin_t bin;

	switch(filetype) {
	case FILETYPE_ELF:
		fd = ELF_CALL(dietelf_new,bin,file);
		if (fd == -1) {
			fprintf(stderr, "cannot open file\n");
			return;
		}
		ELF_CALL(dietelf_list_libs,bin,fd);
		close(fd);
		break;
	case FILETYPE_MACHO:
		sprintf(buf, "otool -L '%s'", file);
		system(buf);
		break;
	default:
		sprintf(buf, "strings '%s' | grep -e '^lib'", file);
		system(buf);
		break;
	}
}

int rabin_identify_header()
{
	unsigned char buf[1024];

	lseek(fd, 0, SEEK_SET);
	read(fd, buf, 1024);

        if (!memcmp(buf, "\xCA\xFE\xBA\xBE", 4))
		if (buf[9])
                	filetype = FILETYPE_CLASS;
		else	filetype = FILETYPE_MACHO;
	else
        if (!memcmp(buf, "\xFE\xED\xFA\xCE", 4)) {
		filetype = FILETYPE_MACHO;
		printf("endian = big\n");
	} else	
	if (!memcmp(buf, "CSR-", 4)) {
		filetype = FILETYPE_CSRFW;
	//	config_set("asm.arch", "csr");
	} else
	if (!memcmp(buf, "dex\n009\0", 8))
		filetype = FILETYPE_DEX;
	else
	if (!memcmp(buf, "\x7F\x45\x4c\x46", 4)) {
		filetype = FILETYPE_ELF;
		
		if (buf[EI_CLASS] == ELFCLASS64)
		{
		    elf64 = 1;
		}
	} else
	if (!memcmp(buf, "\x4d\x5a", 2)) {
		int pe = buf[0x3c];
		filetype = FILETYPE_MZ;
		if (buf[pe]=='P' && buf[pe+1]=='E') {
			filetype = FILETYPE_PE;
			pebase = pe;
		}
	} else {
		printf("Unknown filetype\n");
	}

	return filetype;
}


int main(int argc, char **argv, char **envp)
{
	int c;

	while ((c = getopt(argc, argv, "acerlishL:SIvxz")) != -1)
	{
		switch( c ) {
		case 'a':
			action |= ACTION_ARCH;
			break;
		case 'b':
			action |= ACTION_BASE;
			break;
		case 'i':
			action |= ACTION_IMPORTS;
			break;
		case 'c':
			action |= ACTION_CHECKSUM;
			break;
		case 's':
			action |= ACTION_SYMBOLS;
			//action |= ACTION_EXPORTS;
			break;
#if 0
		case 't':
			action |= ACTION_FILETYPE;
			break;
		case 'o':
			action |= ACTION_OTHERS;
			break;
#endif
		case 'S':
			action |= ACTION_SECTIONS;
			break;
		case 'I':
			action |= ACTION_INFO;
			break;
		case 'e':
			action |= ACTION_ENTRY;
			break;
		case 'l':
			action |= ACTION_LIBS;
			break;
		case 'L':
			printf("0x%08lx %s\n", addr_for_lib(optarg), optarg);
			action |= ACTION_NOP;
			break;
		case 'r':
			rad = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'x':
			xrefs = 1;
			break;
		case 'z':
			action |= ACTION_STRINGS;
			break;
		case 'h':
		default:
			return rabin_show_help();
		}
	}

	file = argv[optind];

	if (action == ACTION_UNK)
		return rabin_show_help();
	if (action != ACTION_NOP) {
		if (file == NULL)
			return rabin_show_help();
		fd = open(file, O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "Cannot open file\n");
			return 0;
		}
	} else return 0;

	rabin_identify_header();

	if (action&ACTION_ARCH)
		rabin_show_arch(file);
	if (action&ACTION_ENTRY)
		rabin_show_entrypoint(file);
#if 0
	if (action&ACTION_FILETYPE)
		rabin_show_filetype();
	if (action&ACTION_EXPORTS)
		rabin_show_exports(file);
	if (action&ACTION_OTHERS)
		rabin_show_others(file);
#endif
	if (action&ACTION_IMPORTS)
		rabin_show_imports(file);
	if (action&ACTION_SYMBOLS)
		rabin_show_symbols(file);
	if (action&ACTION_SECTIONS)
		rabin_show_sections(file);
	if (action&ACTION_INFO)
		rabin_show_info(file);
	if (action&ACTION_LIBS)
		rabin_show_libs(file);
	if (action&ACTION_CHECKSUM)
		rabin_show_checksum(file);
	if (action&ACTION_STRINGS)
		rabin_show_strings(file);

	close(fd);

	return 0;
}
