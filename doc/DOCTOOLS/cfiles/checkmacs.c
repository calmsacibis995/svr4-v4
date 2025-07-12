/*
 *	ident	"@(#)doctools:cfiles/checkmacs.c	1.2"
 *      Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
 *      Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
 *        All Rights Reserved
 *
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
 *      UNIX System Laboratories, Inc.
 *      The copyright notice above does not evidence any
 *      actual or intended publication of such source code.
 *
 *      checkmacs - a slimmed down, adaptation of checkdoc
 *
 *    Modified by: M. Shapiro
 *                UNIX System Laboratories
 *                attunix!mxs   201-522-5181
 *                  
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXLINE 512
int maxline=MAXLINE;
char line[MAXLINE+2];
char xline[MAXLINE+2];
int line_nr, MP, MACSTART, eof; /* Added MP flag for TH/SS conflict */

main(argc,argv)
char **argv; 
int argc;
{
int c;
	char *p, *q;
	int file_nr;
	FILE *fp;
	printf("checkmacs version 1.0 diagnostics:\n");
	file_nr = 1;
	do{
		if(argc == 1) fp = stdin;
		else{
			fp = fopen(argv[file_nr],"r");
			printf("%s:\n",argv[file_nr]);
		}
		if(fp == NULL){
			printf("File %s could not be opened\n\n",argv[file_nr]);
			continue;
		}
		line_nr = 0;
		eof = 0;
		MP = 0;
		MACSTART = 0;
		do{
			if(fgets(line,MAXLINE+2,fp) == NULL) eof++;
			line_nr++;
			/* check for excessive line length */
			if(!eof && strlen(line) > MAXLINE){
				line[MAXLINE] = '\n';
				printf("Line %d: Line should not exceed %d characters\n",line_nr,maxline);
				/* eat up remainder of the line */
				if(line[MAXLINE+1]!='\n' && line[MAXLINE+1]!='\0')
					while((c=getc(fp)) != '\n');
			}
			/* change ' control character to . */
			if(line[0]=='\'') line[0] = '.';
			/* check for break line at beginning */
			if((line[0]=='\n' || line[0]==' ') && (MACSTART != 1))
				printf("Line %d: empty line or begins with a space; may cause incorrect page break\n",line_nr);
			/* squeeze out white space after a control character */
			if(line[0]=='.'){
				/* Now set flag for first macro hit */
				if(MACSTART != 1) {
					strcpy(xline,line);
					if((p=strpbrk(xline+1,"ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789")) != NULL)
						MACSTART = 1;
				}
				p = q = &line[1];
				while(*q==' ' || *q=='\t') q++;
				if(p!=q){
					do *p++ = *q; while(*q++!='\0');
				}
			}
			/* check for additional source files and .sy command line */
			if(!eof){
				if(!strncmp(line,".so",3)){
					printf("Line %d: .so file not examined - see man page\n",line_nr);
					continue;
				}
				if(!strncmp(line,".SO",3)){
					printf("Line %d: .SO file not examined - see man page\n",line_nr);
					continue;
				}
				if(!strncmp(line,".nx",3)){
					printf("Line %d: .nx file not examined - see man page\n",line_nr);
					continue;
				}
				if(!strncmp(line,".sy",3)){
					printf("Line %d: CAUTION! Before formatting make sure that\n",line_nr);
					printf("            you know what this .sy command does!\n");
					continue;
				}
			}
			
     /* calls of the checking functions follow */
     /* each function is called once for each line of input and
        once at end-of-file.
        The global variables line, line_nr, and eof are to be used
        by those routines.
	The line will be terminated by a newline character.
	At end-of-file each function must reinitialize itself so
	as to be ready for processing additional files.
     */
			pic_ck();
			nest_ck();
			tbl_ck();
			eqn_ck();
			tl_ck();
			lst_ck();
			font_ck();
			quote_ck();
		}  while(!eof);
		printf("%d lines done\n\n",line_nr-1);
		fclose(fp);
	} while(++file_nr < argc);
	/* if this program was invoked with the name ckmm assume that
	   the shell script is being used which will invoke the
	   original checkmm next
	 */
	if((p=strrchr(argv[0],'/'))!=NULL && !strcmp(p+1,"ckmm"))
		printf("original checkmm diagnostics:\n");
}

/* Special function which searches for string s2 (null terminated)
   in string s1 (newline terminated) and returns a pointer to the
   character following the occurrance, or NULL if not found.
 */
 /* D. Muir 4/2/88 */
char *index(s1,s2) char *s1, *s2;
{
	char *p, *pp, *q;
	for(p=s1; *p!='\n'; p++){
		pp = p; q = s2;
		while(*pp == *q){pp++; q++;}
		if(*q == '\0') return pp;
	}
	return NULL;
}

/* Check PIC drawings */
/* D. Muir 6/5/88 */
pic_ck(){
	static int state1 = 0, state2 = 0;
	/* State1 Table:
     0 = initial(base) state
     1 = .tr ~ in effect
     State2 Table:
     0 = initial(base) state
     1 = .PS found
     2 = in a display
     3 = .PS found in a display
     4 = PIC drawing ended in a display
   */
	char *p;

	/* EOF processing */
	if(eof) {
		/* the following deleted because nest_ck does the function
		if(state2 == 1)
			printf("Missing .PE or .PF detected at EOF\n");
		*/
		state1 = 0; 
		state2 = 0;
		return;
	}

	/* .tr processing */
	if(!strncmp(line,".tr",3)){
		if((p=strchr(line,(int)'~')) != NULL){
			if(*(p+1) != '~'){
				 state1 = 1;
				if(state2==3 || state2==4)
					printf("Line %d: .tr ~ may cause an error in the preceding PIC drawing\n",line_nr);
			}
			else state1 = 0;
		}
		return;
	}

	/* Display processing */
	if(!strncmp(line,".DS",3)){
		if(state2 == 0) state2 = 2;
		return;
	}
	if(!strncmp(line,".DE",3)){
		state2 = 0;
		return;
	}

	/* .PS processing */
	if(!strncmp(line,".PS",3)){
		if(state1 == 1)
			printf("Line %d: .tr ~ may cause an error in the following PIC drawing\n",line_nr);
		p = &line[3];
		while(*p == ' ') p++;
		if(*p == '<') return;
		switch(state2){
		case 0:
		case 2:
			state2++;
			break;
		case 1:
		case 3:
			printf("Line %d: Missing .PE\n",line_nr);
			break;
		case 4:
			state2 = 3;
			break;
		}
		return;
	}

	/* .PE and .PF processing */
	if(!strncmp(line,".PE",3) || !strncmp(line,".PF",3)){
		switch(state2){
		case 0:
		case 2:
		case 4:
			/* the following deleted because nest_ck does the function
			printf("Line %d: .PE or .PF not preceded by a .PS\n",line_nr);
			*/
			break;
		case 1:
			state2 = 0;
			break;
		case 3:
			state2 = 4;
			break;
		}
		return;
	}

	return;
}

/* Check for illegal nesting and bad pairing of certain macros */
#define PUSH(x) state_stack[i++]=state; state=x
#define POP  state=state_stack[--i]
/* Following two just check for correct pairing, like VS/VE */
#define PUSHCNT(x) state_cnt[x]++;
#define POPCNT(x)  state_cnt[x]--;
#define SETCNT(x)  state_cnt[x]=0;
nest_ck(){
	static int state=0, state_stack[20], i=0, state_cnt[20];
	/* State Table:
     0 = initial(base) state
     1 = in a .AB/.AC pair
     2 = in a .DS/.DE pair
     3 = in a .2C/.1C pair
     4 = in a .2S/.2E pair
     5 = in a .TS/.TE pair
     6 = in a .RS/.RE pair
     7 = in a .SS/.SE pair
     8 = in a .BB/.BC pair
     9 = in a .EQ/.EN pair
    10 = in a .G1/.G2 pair
    11 = in a .PS/.PE or .PF pair
    12 = in a .GS/.GE pair
    13 = in a .VS/.VE pair
    14 = in a .CO/.SF pair
    15 = in a .UI/.SF pair
    16 = in a .PC/.SF pair
   */
	char pname[8], *index(), *p;
	int error;
	error = 0;

set_pname:
	switch(state){
	case 0:
		break;
	case 1:
		strcpy(pname,".AB/.AC");
		break;
	case 2:
		strcpy(pname,".DS/.DE");
		break;
	case 3:
		strcpy(pname,".2C/.1C");
		break;
	case 4:
		strcpy(pname,".2S/.2E");
		break;
	case 5:
		strcpy(pname,".TS/.TE");
		break;
	case 6:
		strcpy(pname,".RS/.RE");
		break;
	case 7:
		strcpy(pname,".SS/.SE");
		break;
	case 8:
		strcpy(pname,".BB/.BC");
		break;
	case 9:
		strcpy(pname,".EQ/.EN");
		break;
	case 10:
		strcpy(pname,".G1/.G2");
		break;
	case 11:
		strcpy(pname,".PS/.PE");
		break;
	case 12:
		strcpy(pname,".GS/.GE");
		break;
	case 13:
		strcpy(pname,".VS/.VE");
		break;
	case 14:
		strcpy(pname,".CO/.SF");
		break;
	case 15:
		strcpy(pname,".UI/.SF");
		break;
	case 16:
		strcpy(pname,".PC/.SF");
		break;
	}

	if(eof){
		if(state != 0){
			printf("Missing %.3s detected at EOF\n",&pname[4]);
			POP;
			goto set_pname;
		}
		i =0; /* should already be zero but just in case */
		return;
	}

	/* See if we have a .. line */
	if(!strncmp(line,"..",2) && (line[2] == ' ' || line[2] == '\n')){
		if(state != 0){

			printf("Line %d: .. not allowed within a %s pair\n",line_nr,pname);
			return;
		}
	}

	/* Check other illegal nestings */
	if(!strncmp(line,".AB",3)){
		if(state!=0 && state!=14)error++;
		PUSH(1);
		goto end;
	}
	if(!strncmp(line,".AC",3)){
		if(state != 1) printf("Line %d: .AC not preceded by a .AB\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".DS",3)){
		if(state!=0)error++;
		PUSH(2);
		goto end;
	}
	if(!strncmp(line,".DE",3)){
		if(state!=2) printf("Line %d: .DE not preceded by a .DS\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".2C",3)){
		if(state!=0)error++;
		PUSH(3);
		goto end;
	}
	if(!strncmp(line,".1C",3)){
		if(state!=3) printf("Line %d: .1C not preceded by a .2C\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".2S",3)){
		if(state!=0)error++;
		PUSH(4);
		goto end;
	}
	if(!strncmp(line,".2E",3)){
		if(state != 4) printf("Line %d: .2E not preceded by a .2S\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".TS",3)){
		if(state!=0 && state!=2 && state!=3 && state!=6 && state!=7 && state!=13 && state!=14)error++;
		PUSH(5);
		goto end;
	}
	if(!strncmp(line,".TE",3)){
		if(state != 5) printf("Line %d: .TE not preceded by a .TS\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".RS",3)){
		PUSH(6);
		goto end;
	}
	if(!strncmp(line,".RE",3)){
		if(state != 6) printf("Line %d: .RE not preceded by a .RS\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if((!strncmp(line,".SS",3)) && (MP != 1)){
		if(state!=0)error++;
		PUSH(7);
		goto end;
	}
	if(!strncmp(line,".SE",3)){
		if(state != 7) printf("Line %d: .SE not preceded by a .SS\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".BB",3)){
		/* Don't do PUSH if BB has argument */
		strcpy(xline,line);
		if((p=strtok(xline+4," \t\n")) == NULL) {
			PUSH(8);
		}
		goto end;			
	}
	if(!strncmp(line,".BC",3)){
		if(state != 8) printf("Line %d: .BC not preceded by a .BB\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".EQ",3)){
		if(state!=0 && state!=2 && state!=3 && state!=5 && state!=13 && state!=14)error++;
		PUSH(9);
		goto end;
	}
	if(!strncmp(line,".EN",3)){
		if(state != 9) printf("Line %d: .EN not preceded by a .EQ\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".G1",3)){
		if(state!=0 && state!=2 && state!=3)error++;
		PUSH(10);
		goto end;
	}
	if(!strncmp(line,".G2",3)){
		if(state != 10) printf("Line %d: .G2 not preceded by a .G1\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".PS",3)){
		if(strchr(line,(int)'<')==NULL){
			PUSH(11);
		}
		goto end;
	}
	if(!strncmp(line,".PE",3)){
		if(state != 11) printf("Line %d: .PE not preceded by a .PS\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".PF",3)){
		if(state==0) return; /* page footer macro must be assumed */
		if(state!=11){
			error++; 
			goto end;
		}
		POP;
		return;
	}
	if(!strncmp(line,".GS",3)){
		if(state!=0 && state!=2 && state!=3)error++;
		PUSH(12);
		goto end;
	}
	if(!strncmp(line,".GE",3)){
		if(state != 12) printf("Line %d: .GE not preceded by a .GS\n",line_nr);
		else{
			POP;
		}
		return;
	}
	if(!strncmp(line,".SG",3)){
		if(state!=0) error++;
		goto end;
	}
	if(!strncmp(line,".VS",3)){
		if(state_cnt[13]!=0) printf("Line %d: .VS Cannot nest .VS/.VE pairs\n",line_nr);
		/* Don't do PUSH if VS has argument   vsresolve enhancement (mxs) */
		for (p = line + strlen(".VS"); isspace(*p); p++);
		while ( !isspace(*p) && *p != '\0' ) p++;
		while ( isspace(*p) )  p++;
		if ( !p == '\n' || *p == '\0' )
			PUSHCNT(13);
		goto end;
	}
	if(!strncmp(line,".VE",3)){
		if(state_cnt[13]!=1) printf("Line %d: .VE: Matching .VS Not Found\n",line_nr);
		else {
			POPCNT(13);
		}
		return;
	}
	if(!strncmp(line,".CO",3)){
		/* Don't do PUSH if CO has argument */
		strcpy(xline,line);
		if(((p=strtok(xline+4," \t\n")) == NULL) && (state!=15 && state!=16)){
			PUSHCNT(14);
		}
		goto end;
	}
	if(!strncmp(line,".UI",3)){
		/* Don't do PUSH if UI has argument */
		strcpy(xline,line);
		if(((p=strtok(xline+4," \t\n")) == NULL) && (state!=14 && state!=16)){
			PUSHCNT(15);
		}
		goto end;
	}
	if(!strncmp(line,".PC",3)){
		/* Don't do PUSH if PC has argument */
		strcpy(xline,line);
		if(((p=strtok(xline+4," \t\n")) == NULL) && (state!=14 && state!=15)){
			PUSHCNT(16);
		}
		goto end;
	}
	if(!strncmp(line,".SF",3)){
		if(state_cnt[14]!=1 && state_cnt[15]!=1 && state_cnt[16]!=1){
		    printf("Line %d: .SF: Matching .UI, .CO, or .PC Not Found\n",line_nr);
		}
		else {
			SETCNT(14); SETCNT(15); SETCNT(16);
		}
		return;
	}
	/* Pretend we're at end of file if unended pairs hit here */
	if(!strncmp(line,".H ",3)){
		if(state != 0){
			printf("Line %d: Missing %.3s detected before heading: %s\n",line_nr,&pname[4],line);
			POP;
			goto set_pname;
		}
		if(state_cnt[14]!=0){
			printf("Line %d: Missing .SF: Unended .CO/.SF display detected before heading: %s\n",line_nr,line);
			SETCNT(14);
		}
		if(state_cnt[15]!=0){
			printf("Line %d: Missing .SF: Unended .UI/.SF display detected before heading: %s\n",line_nr,line);
			SETCNT(15);
		}
		if(state_cnt[16]!=0){
			printf("Line %d: Missing .SF: Unended .PC/.SF display detected before heading: %s\n",line_nr,line);
			SETCNT(16);
		}
		i =0; /* should already be zero but just in case */
		return;
	}
end:
	if(error) printf("Line %d: %.3s not allowed within a %s pair\n",line_nr,line,pname);
#undef PUSH
#undef POP
}

/* Check various problems with tables */
/* D. Muir 6/3/88 */
/* Added _ and = to list of allowable column format descriptors.
   D. Muir 11/22/88 */
/* Added check for characters after the ; on the options line.
   D. Muir 2/20/89 */
tbl_ck(){
	static int state = 0;
	/* state table:
	0 = base (initial) state
	1 = .TS found
	2 = end of global options
	3 = end of format section
	*/
	static int NR_COLS = 0; /* from first format line */
	static int nr_cols = 0; /* for subsequent format lines and
					   for columns of data */
	static int H_OPTION = 0;
	static int T_BLOCK = 0;
	static char TAB = '\t';
	char *p, *index();

	/* eof processing */
	if(eof){
		/* no need to check for missing .TE becuz nest_ck does it */
		if(T_BLOCK) printf("Uncompleted table text block found at EOF\n");
		if(state==2) printf("Missing table format section found at EOF\n");
		if(state==1) printf("Missing table global options found at EOF\n");
		TAB='\t'; 
		state=T_BLOCK=NR_COLS=nr_cols=H_OPTION=0;
		return;
	}

	/* .TS processing */
	if(!strncmp(line,".TS",3)){
		state = 1;
		p = &line[3];
		while(*p == ' ') p++;
		if(*p == 'H') H_OPTION = 1;
		return;
	}

	/* .TE processing */
	if(!strncmp(line,".TE",3)){
		if(H_OPTION) printf("Line %d: Missing .TH after .TS H\n",line_nr);
		if(T_BLOCK) printf("Line %d: Missing end of text block (T}) detected\n",line_nr);
		if(state==2) printf("Line %d: Missing table format data detected\n",line_nr);
		if(state==1) printf("Line %d: Missing table options detected\n",line_nr);
		state=T_BLOCK=NR_COLS=H_OPTION=0;
		TAB = '\t';
		return;
	}

	/* .TH processing */
	if(!strncmp(line,".TH",3)){
		strcpy(xline,line);
		/* Got to protect against .TH in manpage */
		if((p=strtok(xline+4," \t\nH")) == NULL) {
			if(!H_OPTION) printf("Line %d: Missing .TS H before .TH\n",line_nr);
			else H_OPTION = 0;
		}
		else
			MP = 1; /* Have to tell SS/SE this is manpage */
		return;
	}

	/* global options processing */
	if(state == 1){
		state = 2;
		if((p=strrchr(line,(int)';'))!=NULL){ /* we have an options line */
			if(*(p+1)!='\n') printf("Line %d: Semicolon not last character\n",line_nr);
			if((p=index(line,"tab")) != NULL){
				while(*p == ' ') p++;
				if(*p=='(' && *(p+2)==')') TAB = *(p+1);
			}
			return;
		}
	}

	/* format section processing */
	if(state == 2){
		p = line;
		if(NR_COLS == 0){
			while((p=strpbrk(p,"LlRrCcNnAaSs^_=.")) != NULL){
				if(*p=='.'){
					while(*(p+1)==' ') p++;
					if(*(p+1)=='\n'){
						state=3;
						return;
					}
					else NR_COLS--;
				}
				p++; 
				NR_COLS++;
			}
			if(NR_COLS>36) printf("Line %d: Too many columns\n",line_nr);
			return;
		}
		else{
			nr_cols = 0;
			while((p=strpbrk(p,"LlRrCcNnAaSs^_=.")) != NULL){
				if(*p=='.'){
					while(*(p+1)==' ') p++;
					if(*(p+1)=='\n'){
						state=3;
						break;
					}
					else nr_cols--;
				}
				p++; 
				nr_cols++;
			}
			if(nr_cols != NR_COLS)
				printf("Line %d: format lines must have same number of columns\n",line_nr);
			nr_cols = 0;
			return;
		}
	}

	/* format respecification processing */
	if(state==3 && !strncmp(line,".T&",3)){ 
		state = 2; 
		return;
	}

	/* processing of data lines including text blocks */
	if(state==3 && !T_BLOCK){
		p = line;
cnt_data:	
		while(*p != '\n'){
			while(*p!=TAB && *p!='\n') p++;
			if(++nr_cols > NR_COLS){
				printf("Line %d: Too many columns of data\n",line_nr);
				nr_cols = 0;
				return;
			}
			if(*p == TAB) p++;
		}
	}

	if(state==3 && !T_BLOCK){
		if((p = index(line,"T{"))!=NULL){
			if(*p != '\n') printf("Line %d: T{ must be at the end of the input line\n",line_nr);
			T_BLOCK = 1;
			return;
		}
		else nr_cols = 0;
	}

	if(T_BLOCK && (p=index(line,"T}"))!=NULL){
		if(p!=&line[2]) printf("Line %d: T} must be at the beginning of the line\n",line_nr);
		if(*p!='\n' && *p!=TAB)
			printf("Line %d: T} must be followed by the TAB character or the end of the line\n",line_nr);
		T_BLOCK = 0;
		if(*p=='\n'){
			nr_cols=0; 
			return;
		}
		else{
			p++; 
			goto cnt_data;
		}
	}
}

/* Check various problems with equations */
/* D. Muir 2/17/88 */
/* Exempted control lines from delim, etc. scan - D. Muir 9/16/88 */
/* Removed check for mismatched vertical bars - D. Muir 11/16/88  */
/* Added check for bad equation delimiters - D. Muir 2/21/89 */
/* Added "delim off" statement processing - D. Muir 2/21/89 */
/* Added warning for delimiter alone on a line - D. Muir 3/7/89 */
/* Fixed bug that caused continued control lines (CONT=1) to not
   end when they should.
   Eliminated the search for in-line equation delimiters in an
   .EQ/.EN pair.
   Added EQ != 0 as a condition for exempting control lines from
   the delim, etc. scan - D. Muir 3/31/89 */
/* Fixed bug that caused .H lines not to be scanned for delimiters,
   etc. - D. Muir 4/6/89 */
eqn_ck(){
	static int EQ = 0, /* in an .EQ/.EN pair */
	DEQ = 0, /* in delim/delim pair */
	CONT = 0, /* continuation of a control line */
	ALONE = 0, /* left delim alone on the line */
	PAREN = 0,
	BRACE = 0,
	BRACKET = 0;
	static char l_delim = '\0',
	r_delim = '\0';
	char *p, *q;
	int i;
	static char bad_delim[]={'#','{','}','~','^','\"','\n'};

	/* eof processing */
	if(eof){
		/* deleted because nest_ck does the function
		if(EQ != 0) printf("Missing .EN detected at EOF\n");
		*/
		if(DEQ != 0) printf("Missing right equation delimiter detected at EOF\n");
		EQ=DEQ=PAREN=BRACE=BRACKET=CONT=ALONE=0;
		l_delim=r_delim='\0';
		return;
	}

	/* delim statement processing */
	if(EQ == 1 && !strncmp(line,"delim",5)){
		p = &line[5];
		while(*p == ' ') p++;
		if(*(p-1) != ' ' ||
		    !isgraph((int)*p) ||
		    !isgraph((int)*(p+1))){
			printf("Line %d: syntax error in delim statement\n",line_nr);
			return;
		}
		if(!strncmp(p,"off",3)){
			l_delim=r_delim='\0';
			return;
		}
		l_delim = *p;
		r_delim = *(p+1);
		for(i=0; bad_delim[i]!='\n'; i++){
			if(*p==bad_delim[i] || *(p+1)==bad_delim[i]){
				printf("Line %d: #, {, }, ~, ^ and \" should not be used as equation delimiters\n",line_nr);
				return;
			}
		}
		return;
	}

	/* .H processing */
	if(!strncmp(line,".H",2)){
		if(DEQ == 1)
			printf("Line %d: Missing right equation delimiter before .H\n",line_nr);
		DEQ = EQ = PAREN = BRACE = BRACKET = CONT = 0;
	}

	/* .EQ/.EN processing */
	if(!strncmp(line,".EQ",3)){
		if(EQ == 1) printf("Line %d: Missing .EN or redundant .EQ detected\n",line_nr);
		EQ = 1;
		if(DEQ == 1){
			DEQ = 0;
			printf("Line %d: .EQ not allowed within a delimited equation\n",line_nr);
		}
		return;
	}
	if(!strncmp(line,".EN",3)){
		/* nest_ck looks for redundant .EN */
		EQ = 0;
		if(DEQ == 1){
			DEQ = 0;
			printf("Line %d: .EN not allowed within a delimited equation\n",line_nr);
		}
		if(PAREN != 0) printf("Line %d: Mismatched parens in equation\n",line_nr);
		if(BRACE != 0) printf("Line %d: Mismatched braces in equation\n",line_nr);
		if(BRACKET != 0) printf("Line %d: Mismatched brackets in equation\n",line_nr);
		PAREN = BRACE = BRACKET = CONT = 0;
		return;
	}

	/* delim finding and paren, etc scan */
	/* don't scan formatting control lines or their continuations 
		in an EQ/EN pair */
	if(CONT || (EQ && line[0]=='.')){
		if(line[strlen(line)-2] == '\\') CONT = 1;
		else CONT = 0;
		return;
	}
	p = line;
	p--;
	while(*++p != '\n'){ /* scan the line */
		if(DEQ || EQ){
			/* \" does not start a string */
			if(*p=='\\' && *(p+1)=='\"'){
				p++;
				continue;
			}
			if(*p == '\"'){  /* ignore quoted strings */
				if((p=strchr(++p,'\"')) == NULL){
					printf("Line %d: Unfinished quoted string\n",line_nr);
					break;
				}
				continue;
			}
			if(!strncmp(p,"left",4)){
				q = p+4;
				while(*q == ' ') q++;
				if(*q=='[' || *q=='{' || *q=='('){
					p = q;
					continue;
				}
			}
			if(!strncmp(p,"right",5)){
				q = p+5;
				while(*q == ' ') q++;
				if(*q==']' || *q=='}' || *q==')'){
					p = q;
					continue;
				}
			}
		}
		if(!EQ){
			if(!DEQ && *p == l_delim){
				DEQ = 1;
				ALONE = 1;
				continue;
			}
			if(DEQ && *p == r_delim){
				DEQ = 0;
				ALONE = 0;
				if(PAREN != 0) printf("Line %d: Mismatched parens in equation\n",line_nr);
				if(BRACE != 0) printf("Line %d: Mismatched braces in equation\n",line_nr);
				if(BRACKET != 0) printf("Line %d: Mismatched brackets in equation\n",line_nr);
				PAREN = BRACE = BRACKET = 0;
				continue;
			}
		}
		if(DEQ || EQ){
			if(*p == '(' && *(p-1) != '\\'){
				PAREN++;
				continue;
			}
			if(*p == ')'){
				PAREN--;
				continue;
			}
			if(*p == '{'){
				BRACE++;
				continue;
			}
			if(*p == '}'){
				BRACE--;
				continue;
			}
			if(*p == '['){
				BRACKET++;
				continue;
			}
			if(*p == ']'){
				BRACKET--;
				continue;
			}
			/* \(xx is the special character xx and \" is allowed by eqn */
			if(*p=='\\' && *(p+1)!='(' && *(p+1)!='\"'){
				printf("Line %d: Unquoted troff command in equation\n",line_nr);
				continue;
			}
		}
	} /* end while(*++p != '\n') */
	if(ALONE){
		printf("Line %d: Warning - left equation delimiter alone on this line\n",line_nr);
		ALONE = 0;
	}
}

/* Make sure a break occurs before a .tl */
/* D. Muir 2/17/88 */
/* Bug fix: static added to char end[4]; D. Muir 7/25/88 */
/* Bug fix: end[4] changed to end[5] and limited the length of copies
   into and comparisons of that data to guard against trailing
   blanks - D Muir 11/22/88 */
tl_ck(){
	static int state = 0;
	/* State Table:
     0 = initial(base) state
     1 = .ig or .de in effect
     2 = break causing command detected
   */
	static char end[5]; /* ending string for .ig or .de blocks
			       Must be big enuf for .xx\n\0      */
	char *p;
	int i;
	static char *break_cmd[] ={
		".nf",".fi",".br",".bp",".ce",".sp",
		".in",".ti",".SK","END"		};

	if(eof){ 
		state = 0; 
		return;
	}

	switch(state){
	case 0:
		if(!strncmp(line,".de",3)){
			state = 1;
			strcpy(end,"..");
			return;
		}
		if(!strncmp(line,".ig",3)){
			state = 1;
			p = &line[3];
			while(*p == ' ') p++;
			if(*p == '\n') strcpy(end,"..");
			else {
				strcpy(end,".");
				strncat(end,p,3);
			}
			return;
		}
		if(!strncmp(line,".tl",3)){
			printf("Line %d: .br needed ahead of .tl\n",line_nr);
			return;
		}
		i = 0;
		while(strcmp(break_cmd[i],"END")){
			if(!strncmp(line,break_cmd[i++],3)){
				state = 2;
				return;
			}
		}
		return;
	case 1:
		if(!strncmp(line,end,3)) state = 0;
	case 2:
		return;
	}
}

/* Check for list pairings  */
/* V. G. Dave 4/28/88  */
/* Added check for text-indent arg on .VL - D. Muir 11/14/88 */
lst_ck()
{
	struct list
	{
		char type_lst[4];
		int line_num;
	};

	static struct list level[21];
	static int lev_num = 0;
	char *p;

	if(eof)
	{
		while (lev_num > 0)
		{
			if(lev_num < 22)
			{
				printf("Unfinished %s list detected at EOF which was begun on line %d\n", 
				    level[lev_num].type_lst,
				    level[lev_num].line_num);
			}
			lev_num--;
		}
		return;
	}
	if(!strncmp(line,".LI",3) && lev_num == 0)
	{
		lev_num++;
		strncpy(level[lev_num].type_lst,".??",3);
		level[lev_num].line_num = 0;
		printf("Line %d: Missing list begin before .LI\n",line_nr);
	}
	if(!strncmp(line,".H ",3))
	{
		while (lev_num > 0)
		{
			if(lev_num < 22)
			{
				printf("Line %d: Missing .LE for %s before .H\n",
			 	    line_nr,
				    level[lev_num].type_lst);
			}
			lev_num--;
		}
	}
	if(!strncmp(line, ".AL", 3))
	{
		lev_num++;
		if (lev_num < 22)
		{
			strncpy(level[lev_num].type_lst, ".AL", 3);
			(level[lev_num].line_num = line_nr);
			if ( (lev_num > 3) && (lev_num < 22) )
			{
				printf("Line %d: Lists nested %d deep. Maximum levels permitted is 3.\n", line_nr,
				    lev_num);
			}
		}
	}
	if (!strncmp(line, ".BL", 3))
	{
		lev_num++;
		if (lev_num < 22)
		{
			strncpy(level[lev_num].type_lst, ".BL", 3);
			(level[lev_num].line_num = line_nr);
			if ( (lev_num > 3) && (lev_num < 22) )
			{
				printf("Line %d: Lists nested %d deep. Maximum levels permitted is 3.\n", line_nr,
				    lev_num);
			}
		}
	}
	if (!strncmp(line, ".DL", 3))
	{
		lev_num++;
		if (lev_num < 22)
		{
			strncpy(level[lev_num].type_lst, ".DL", 3);
			(level[lev_num].line_num = line_nr);
			if ( (lev_num > 3) && (lev_num < 22) )
			{
				printf("Line %d: Lists nested %d deep. Maximum levels permitted is 3.\n", line_nr,
				    lev_num);
			}
		}
	}
	if (!strncmp(line, ".VL", 3))
	{
		p = &line[3];
		while(*p == ' ') p++;
		lev_num++;
		if (lev_num < 22)
		{
			strncpy(level[lev_num].type_lst, ".VL", 3);
			(level[lev_num].line_num = line_nr);
			if ( (lev_num > 3) && (lev_num < 22) )
			{
				printf("Line %d: Lists nested %d deep. Maximum levels permitted is 3.\n", line_nr,
				    lev_num);
			}
		}
	}
	if (!strncmp(line, ".LE", 3))
	{
		lev_num--;
		if (lev_num < 0)
		{
			printf("Line %d: Extra .LE\n", line_nr);
			lev_num = 0;
		}
	}
}

/* check for bad font selections */
/* D. Muir  9/12/88 */
/* Added ZD font - D. Muir 2/20/98 */
font_ck(){
	static int state = 0;
	/* state table:
		0 = initial(base) state
		1 = in an ignore block
	*/
	static char one_char_font[] = {
		'B','C','H','I','P','R','S',
		'1','2','3','4','5','6','7','8','9','\n'};
	static char *two_char_font[] = {
		"BI","CB","CE","CI","CO","CT","CW","CX",
		"GB","GR","GS","HB","HI","HK","HL","HX","LO",
		"MB","MI","MR","MX","PA","PB","PI","PX",
		"S1","SC","SM","TB","TX","ZD","10","\n\n"};
	int i;
	char *p, *index();
	static char end[4];

	/* eof processing */
	if(eof){
		state = 0;
		return;
	}

	switch(state){
	case 0:
		if(!strncmp(line,".ig",3)){
			state = 1;
			p = &line[3];
			while(*p == ' ') p++;
			if(*p == '\n') strcpy(end,"..");
			else{
				strcpy(end,".");
				strcat(end,p);
			}
			return;
		}
		if(!strncmp(line,".ft",3)){
			p = &line[3];
			while(*p == ' ') p++;
			i = 0;
			while(one_char_font[i] != '\n'){
				if(*p == one_char_font[i++]) return;
			}
			i = 0;
			while(strcmp(two_char_font[i],"\n\n")){
				if(!strncmp(p,two_char_font[i++],3)) return;
			}
			printf("Line %d: Illegal font requested\n",line_nr);
			return;
		}
		p = line;
		while(1==1){
			if((p=index(p,"\\f")) == NULL) return;
			if(*p == '('){
				i = 0;
				while(strcmp(two_char_font[i],"\n\n")){
					if(!strncmp(p+1,two_char_font[i++],2)) goto done2;
				}
				printf("Line %d: Illegal font %.2s selected\n",line_nr,(p+1));
done2:				p += 2; continue;
			}
			else{
				i = 0;
				while(one_char_font[i] != '\n'){
					if(*p == one_char_font[i++]) goto done1;
				}
				printf("Line %d: Illegal font %.1s selected\n",line_nr,p);
done1:				p++; continue;
			}
		}
	case 1:
		if(!strcmp(line,end)) state = 0;
		return;
	}
}

/* Check for proper closure of quoted values on escape sequences */
/* D. Muir 3/31/89 */
quote_ck(){
	static char letter[] = "bhlLovwxDHS" ;
	char *p, *q;
	int i;

	if(eof) return;

	p = line;
	while(1==1){
		if((p=index(p,"\\")) == NULL) return;
		for(i=0; letter[i] != '\0'; i++){
			if(*p == letter[i]){
				if(*(p+1) != '\'') break;
				p += 2;
repeat:				if((q=index(p,"\'")) == NULL){
					printf("Line %d: Missing quote mark on an escape sequence\n",line_nr);
					return;
				}
				/* an escaped quote doesn't count */
				if(*(q-2) == '\\'){
					p = q;
					goto repeat;
				}
			}
		}
	}
}
