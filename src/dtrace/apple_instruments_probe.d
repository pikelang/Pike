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

BEGIN{printf("<dtrace_output_begin/>\n %-14d\n928DA016-CC75-4D44-9C36-CE93CEE8FC38\n", walltimestamp);

self->indent = 0;
indentstr = ".                                                                                                                                                                 ";}



pike*:::fn-done/((1 == 1))/{

self->indent = (self->indent > 0) ? (self->indent - 2) : 0; 

printf("<e> -1 0 %d %u %-14d\n%s\n%s\n%d \n</e>\n",cpu, tid, walltimestamp, probename, strjoin(substr(indentstr, 0, self->indent + 2), strjoin("<- ", copyinstr(arg0))), self->indent);printf("<s>\n");ustack(128);printf("</s>\n");
}

pike*:::fn-start/((1 == 1))/{

self->indent += 2; 

printf("<e> -1 1 %d %u %-14d\n%s\n%d\n%s\n%s \n</e>\n",cpu, tid, walltimestamp, probename, self->indent, strjoin(substr(indentstr, 0, self->indent), strjoin("-> ", copyinstr(arg0))), stringof(copyinstr(arg1)));printf("<s>\n");ustack(128);printf("</s>\n");
}

pike*:::fn-popframe/((1 == 1))/{

self->indent -= 2;
}


