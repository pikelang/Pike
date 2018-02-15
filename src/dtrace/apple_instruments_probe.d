#!/usr/sbin/dtrace -C -Z -s
/*
 To run this script, please type the following command while in the same 
 directory (or specify the full pather rather than ./):

   sudo ./apple_instruments_probe.d -o dtrace_output.txt 
 
 After the output file has been generated (here, it's dtrace_output.txt but you 
 may call it whatever you like), call up the trace document where
 this script was exported from and choose the "DTrace Data Import..." option
 from the "File" pulldown menu to import that data. 

*/


self int indent; /* thread-local */
char *indentstr;
#pragma D option switchrate=1msec
#pragma D option bufsize=25m
#pragma D option stackindent=0
#pragma D option quiet

syscall::getpriority:entry/execname=="Instruments"/ {}

BEGIN{printf("<dtrace_output_begin/>\n %-14d\n2BE2386B-9075-4AD6-B966-28DAC5FA6317\n", walltimestamp);

self->indent = 0;
indentstr = ".                                                                                                                                                                 ";}



pike*:::fn-done/((1 == 1))/{

self->indent = (self->indent > 0) ? (self->indent - 1) : 0; 

printf("<e> -1 0 %d %u %-14d\n%s\n%s\n%d\n%d \n</e>\n",cpu, tid, walltimestamp, probename, strjoin(substr(indentstr, 0, 2 * (self->indent + 1)), strjoin("< ", copyinstr(arg0))), tid, self->indent + 1);printf("<s>\n");ustack(128);printf("</s>\n");
}

pike*:::fn-start/((1 == 1))/{

self->indent++; 

printf("<e> -1 1 %d %u %-14d\n%s\n%s\n%s\n%d\n%d \n</e>\n",cpu, tid, walltimestamp, probename, stringof(copyinstr(arg1)), strjoin(substr(indentstr, 0, 2 * self->indent), strjoin("> ", copyinstr(arg0))), self->indent, tid);printf("<s>\n");ustack(128);printf("</s>\n");
}

pike*:::fn-popframe/((1 == 1))/{

self->indent--;
}


