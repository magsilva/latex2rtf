/* $Id: funct1.c,v 1.19 2001/09/10 05:40:26 prahl Exp $ 
 
This file contains routines that interpret various LaTeX commands and produce RTF

Authors:  Dorner, Granzer, Polzer, Trisko, Schlatterbeck, Lehner, Prahl
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "main.h"
#include "convert.h"
#include "funct1.h"
#include "commands.h"
#include "stack.h"
#include "l2r_fonts.h"
#include "cfg.h"
#include "ignore.h"
#include "util.h"
#include "encode.h"
#include "parser.h"
#include "counters.h"
#include "lengths.h"
#include "preamble.h"

extern bool     bInDocument;	/* true if File-Pointer is in the document */
extern bool     g_processing_equation;	/* true at a formula-convertion */
extern bool     twocolumn;	/* true if twocolumn-mode is enabled */
extern bool     article;	/* true if article-mode is set */
extern int      indent;		/* includes the left margin e.g. for itemize-commands */
static bool     NoNewLine;
extern bool     bNewPar;
extern bool     mbox;
extern char    *language;
extern enum     TexCharSetKind TexCharSet;
extern int      curr_fontbold[MAXENVIRONS];
extern int      curr_fontital[MAXENVIRONS];
extern int      curr_fontscap[MAXENVIRONS];
extern int      curr_fontnumb[MAXENVIRONS];

static void     CmdLabel1_4(int code, char *text);
static void     CmdLabelOld(int code, char *text);
void            CmdPagestyle( /* @unused@ */ int code);
void            CmdHeader(int code);
void            putRtfChar(char cThis);

void 
CmdBeginEnd(int code)
/***************************************************************************
 purpose: reads the parameter after the \begin or \end-command; ( see also getParam )
	      after reading the parameter the CallParamFunc-function calls the
	      handling-routine for that special environment
 parameter: code: CMD_BEGIN: start of environment
		          CMD_END:   end of environment
 ***************************************************************************/
{
	char            c;
	char           *s = getParam();

	switch (code) {
	case CMD_BEGIN:
		(void) CallParamFunc(s, ON);
		break;
	case CMD_END:
		(void) CallParamFunc(s, OFF);
		break;
	default:
		assert(0);
	}
	free(s);
	c = getNonBlank();
	ungetTexChar(c);
}

void 
Paragraph(int code)
/*****************************************************************************
    purpose : sets the alignment for a paragraph
  parameter : code: alignment centered, justified, left or right
     globals: fRtf: Rtf-file-pointer
              alignment: alignment of paragraphs
              bNewPar
 ********************************************************************************/
{
	static char     old_alignment_before_center = JUSTIFIED;
	static char     old_alignment_before_right = JUSTIFIED;
	static char     old_alignment_before_left = JUSTIFIED;
	static char     old_alignment_before_centerline = JUSTIFIED;

	switch (code) {
	case (PAR_CENTERLINE):
		old_alignment_before_centerline = alignment;
		alignment = CENTERED;
		fprintf(fRtf, "\\par\n{\\pard\\q%c ", alignment);
		diagnostics(4,"Entering Convert from Paragraph");
		Convert();
		diagnostics(4,"Exiting Convert from Paragraph");
		alignment = old_alignment_before_centerline;
		fprintf(fRtf, "\\par}\n\\pard\\q%c ", alignment);
		bNewPar = TRUE;
		break;

	case (PAR_CENTER | ON):
		old_alignment_before_center = alignment;
		alignment = CENTERED;
		fprintf(fRtf, "{\\pard\\q%c ", alignment);
		break;
	case (PAR_CENTER | OFF):
		alignment = old_alignment_before_center;
		fprintf(fRtf, "\\par}\n\\pard\\q%c ", alignment);
		bNewPar = TRUE;
		break;

	case (PAR_RIGHT | ON):
		old_alignment_before_right = alignment;
		alignment = RIGHT;
		fprintf(fRtf, "{\\pard\\q%c ", alignment);
		break;
	case (PAR_RIGHT | OFF):
		alignment = old_alignment_before_right;
		fprintf(fRtf, "\\par}\n\\pard\\q%c ", alignment);
		bNewPar = TRUE;
		break;

	case (PAR_LEFT | ON):
		old_alignment_before_left = alignment;
		alignment = LEFT;
		fprintf(fRtf, "{\\pard\\q%c ", alignment);
		break;
	case (PAR_LEFT | OFF):
		alignment = old_alignment_before_left;
		fprintf(fRtf, "\\par}\n\\pard\\q%c ", alignment);
		bNewPar = TRUE;
		break;
	}
}

void 
CmdToday( /* @unused@ */ int code)
/******************************************************************************
    purpose: converts the LaTeX-date-command into a Rtf-chdate-command which
	     prints the current date into an document
 ******************************************************************************/
{
	fprintf(fRtf, "\\chdate ");
}


void 
CmdIgnore( /* @unused@ */ int code)
/******************************************************************************
 purpose: LaTeX-commands which can't be converted in Rtf-commands are overread
	  as such
 ******************************************************************************/
{
}

void 
CmdLdots( /* @unused@ */ int code)
/******************************************************************************
 purpose: converts the LaTeX-\ldots-command into "..." in Rtf
 globals : fRtf
 ******************************************************************************/
{
	fprintf(fRtf, "...");
}

void 
Environment(int code)
/******************************************************************************
  purpose: pushes/pops the new environment-commands on/from the stack
parameter: code includes the type of the environment
globals  : bIndocument
 ******************************************************************************/
{
	if (code & ON) {	/* on switch */
		code &= ~(ON);	/* mask MSB */
		diagnostics(4,"Entering Environment");
		if (code == DOCUMENT) {
			/* LEG Meanwhile commented out    ClearEnvironment(); */
			bInDocument = TRUE;
/*			if (!pagestyledefined) {
				diagnostics(WARNING, "rtf 1.4 codes generated, funct1.c (Environment)");
				PlainPagestyle();
			}
*/			ReadLg(language);
		}
		PushEnvironment(code);
	} else {		/* off switch */
		diagnostics(4,"Exiting Environment");
		PopEnvironment();
	}
}


void 
CmdSection(int code)
/******************************************************************************
  purpose: converts the LaTeX-section-commands into similar Rtf-styles
parameter: code: type of section-recursion-level
 globals : bNewPar
           bBlankLine
 ******************************************************************************/
{
	char            optparam[100] = "";
	char            *heading;
	char			c;
	int				DefFont = DefaultFontFamily();
	
	getBracketParam(optparam, 99);
	heading = getParam();
	diagnostics(4,"entering CmdSection [%s]{%s}",optparam,heading);
	switch (code) {
	case SECT_PART:
		incrementCounter("part");
		fprintf(fRtf, "\\par\\pard\\page");
		fprintf(fRtf, "{\\qc\\b\\fs40 ");
		ConvertBabelName("PARTNAME");
		fprintf(fRtf, "%d\\par ", getCounter("part"));
		ConvertString(heading);
		fprintf(fRtf, "\\par}\n\\page");
		break;

	case SECT_CHAPTER:
		incrementCounter("chapter");
		setCounter("section",0);
		setCounter("subsection",0);
		setCounter("subsubsection",0);
		fprintf(fRtf, "\\par\\pard\\page\\pard{\\pntext\\pard\\plain\\b\\fs32\\kerning28 ");
		fprintf(fRtf, "%d\\par \\par }\\pard\\plain\n", getCounter("chapter"));
		fprintf(fRtf, "%s\\f%d%s{", HEADER11, DefFont, HEADER12);
		ConvertString(heading);
		fprintf(fRtf, "}\n");
		bNewPar = FALSE;
		bBlankLine = TRUE;
		break;

	case SECT_NORM:
		incrementCounter("section");
		setCounter("subsection",0);
		setCounter("subsubsection",0);
		fprintf(fRtf, "{\\par\\pard\\pntext\\pard\\plain\\b");
		if (article) {
			fprintf(fRtf, "\\fs32\\kerning28 %d\\tab}\\pard\\plain\n", getCounter("section"));
			fprintf(fRtf, "%s\\f%d%s{", HEADER11, DefFont, HEADER12);
		} else {
			fprintf(fRtf, "\\fs24 %d.%d\\tab}\\pard\\plain\n", getCounter("chapter"),getCounter("section"));
			fprintf(fRtf, "%s\\f%d%s{", HEADER21, DefFont, HEADER22);
		}

		ConvertString(heading);
		fprintf(fRtf, "}\n");
		bNewPar = FALSE;
		bBlankLine = TRUE;
		break;

	case SECT_SUB:
		incrementCounter("subsection");
		setCounter("subsubsection",0);
		fprintf(fRtf, "{\\par\\pard\\pntext\\pard\\plain\\b");
		if (article) {
			fprintf(fRtf, "\\fs24 %d.%d\\tab}\\pard\\plain\n", getCounter("section"), getCounter("subsection"));
			fprintf(fRtf, "%s\\f%d%s{", HEADER21, DefFont, HEADER22);
		} else {
			fprintf(fRtf, "\\fs24 %d.%d.%d\\tab}\\pard\\plain\n", getCounter("chapter"), getCounter("section"), getCounter("subsection"));
			fprintf(fRtf, "%s\\f%d%s{", HEADER31, DefFont, HEADER32);
		}

		ConvertString(heading);
		fprintf(fRtf, "}\n");
		bNewPar = FALSE;
		bBlankLine = TRUE;
		break;

	case SECT_SUBSUB:
		incrementCounter("subsubsection");
		fprintf(fRtf, "{\\par\\pard\\pntext\\pard\\plain\\b");
		if (article) {
			fprintf(fRtf, "\\fs24 %d.%d.%d\\tab}\\pard\\plain\n", getCounter("section"), getCounter("subsection"), getCounter("subsubsection"));
			fprintf(fRtf, "%s\\f%d%s{", HEADER31, DefFont, HEADER32);
		} else {
			fprintf(fRtf, "\\fs24 %d.%d.%d.%d\\tab}\\pard\\plain\n", getCounter("chapter"), getCounter("section"), getCounter("subsection"), getCounter("subsubsection"));
			fprintf(fRtf, "%s\\f%d%s{", HEADER41, DefFont, HEADER42);
		}

		ConvertString(heading);
		fprintf(fRtf, "}\n");
		bNewPar = FALSE;
		bBlankLine = TRUE;
		break;
	}
	
	fprintf(fRtf, "\\par\\pard");
	CmdTextNormal(F_TEXT_NORMAL);
	fprintf(fRtf, "\\f%d\\q%c\n", DefFont, alignment);
	if (heading) free(heading);
	c = getNonBlank();
	ungetTexChar(c);

}


void
CmdCaption(int code)
/******************************************************************************
 purpose: converts \caption from LaTeX to Rtf
 ******************************************************************************/
{
	char           *thecaption;
	char            lst_entry[300];
	
	getBracketParam(lst_entry,299);  /* discard entry for list of tables or figures */
	
	diagnostics(4, "entering CmdCaption [%s]", lst_entry);

	if (g_processing_figure) {
		incrementCounter("figure");
		fprintf(fRtf, "\\par\n{\\pard\\qc ");
		ConvertBabelName("FIGURENAME");
		fprintf(fRtf, " %d: ",getCounter("figure"));
	} else {
		incrementCounter("table");
		fprintf(fRtf, "\\par\n{\\pard\\qc ");
		ConvertBabelName("TABLENAME");
		fprintf(fRtf, " %d: ",getCounter("table"));
	}

	thecaption = getParam();
	diagnostics(4, "in CmdCaption [%s]", thecaption);
	ConvertString(thecaption);
	free(thecaption);
	fprintf(fRtf, "\\par}\n\\pard\\q%c\n", alignment);
	diagnostics(4, "exiting CmdCaption");
}

void
CmdCounter(int code)
/******************************************************************************
 purpose: handles \newcounter, \setcounter, \addtocounter, \value
 ******************************************************************************/
{
	char            *s, *s2, *s3, *s4;
	int              num;
	
	s = getParam();
		
	diagnostics(4,"Entering CmdCounter(), <%s>", s);
	
	if (code == COUNTER_ADD || code == COUNTER_SET) {
	
		s2 = getParam();

		if ((s3 = strchr(s2,'{')) && (s4 = strchr(s2,'}')) ) {
			s3++;
			*s4 = '\0';
			diagnostics(4,"CmdCounter() processing \\value{%s}", s3);
			num = getCounter(s3);

		} else if (sscanf(s2, "%d", &num) != 1) {

			fprintf(stderr, "\nBad parameter in set/addcounter{%s}{%s}\n", s,s2);
			free(s2);
			free(s);
			return;
		}
		
		free(s2);
		if (code == COUNTER_ADD)
			setCounter(s, getCounter(s)+num);
		else
			setCounter(s, num);
			
	} else if (code == COUNTER_NEW)
		setCounter(s,0);
		
	free(s);
}

void
CmdLength(int code)
/******************************************************************************
 purpose: handles \newlength, \setlength, \addtolength
 purpose: handle \textwidth=0.8in or \moveright0.1\textwidth
 ******************************************************************************/
{
	char            *s, *s1;
	char             cThis;
	int              num;
	

	if (code == LENGTH_ADD || code == LENGTH_SET || code == LENGTH_NEW) {
		s = getParam();
		if (strlen(s)<=1) {
			free(s);
			fprintf(stderr,"missing argument in \\newlength \\addtolength or \\setlength");
			return;
		}
		s1 = s + 1; /* skip initial '//' */
		diagnostics(4,"Entering CmdLength(), <%s>", s1);
		if (code == LENGTH_ADD || code == LENGTH_SET) {
		
			cThis = getNonSpace();
			
			if (cThis=='{') {
				num = getDimension();
				while ((cThis=getTexChar()) != '}');
						
				if (code == LENGTH_ADD)
					setLength(s1, getLength(s1)+num);
				else
					setLength(s1, num);
			} else 
				fprintf(stderr,"bad parameter to \\addtolength or \\setlength");
				
		} else
			setLength(s1,0);
			
		free(s);
	
	} else {
		skipSpaces();
		cThis = getTexChar();
		
		if (cThis == '=')			/* optional '=' */
			skipSpaces();
		else
			ungetTexChar(cThis);
			
		num = getDimension();      /* discard for now */
	}
}

void 
CmdFootNote(int code)
/******************************************************************************
 purpose: converts footnotes from LaTeX to Rtf
 params : code specifies whether it is a footnote or a thanks-mark
 ******************************************************************************/
{
	char            number[255];
	static int      thankno = 1;
	int             text_ref_upsize, foot_ref_upsize;
	char            *footnote;
	int				DefFont = DefaultFontFamily();
	
	diagnostics(4,"Entering ConvertFootNote");
	getBracketParam(number, 254);	/* is ignored because of the
					 * automatic footnumber-generation */

	text_ref_upsize = (6 * CurrentFontSize()) / 20;
	foot_ref_upsize = (6 * CurrentFontSize()) / 20;

	if (code == THANKS) {
		thankno++;
		fprintf(fRtf, "{\\up%d %d}\n", text_ref_upsize, thankno);
		fprintf(fRtf, "{\\*\\footnote \\pard\\plain\\s246\\f%d",DefFont);
		fprintf(fRtf, "\\fs%d {\\up%d %d}", CurrentFontSize(), foot_ref_upsize, thankno);
	} else {
		fprintf(fRtf, "{\\up%d\\chftn}\n", text_ref_upsize);
		fprintf(fRtf, "{\\*\\footnote \\pard\\plain\\s246\\f%d",DefFont);
		fprintf(fRtf, "\\fs%d {\\up%d\\chftn}", CurrentFontSize(), foot_ref_upsize);
	}

	footnote = getParam();
	if (footnote) {
		ConvertString(footnote);
		free(footnote);
	}
	diagnostics(4,"Exiting CmdFootNote");
	fprintf(fRtf, "}\n ");
}

void 
CmdQuote(int code)
/******************************************************************************
  purpose: converts the LaTeX-Quote-commands into similar Rtf-commands
parameter: code: QUOTE and QUOTATION On/Off
		 specifies the recursion-level of these commands
 globals : NoNewLine: true if no newline should be printed into the Rtf-File
	   indent : includes the left-indent-position
 ******************************************************************************/
{
	switch (code) {
	case (QUOTATION | ON):
		case (QUOTE | ON):
		diagnostics(4,"Entering CmdQuote");
		PushBrace();
		indent += 512;
		NoNewLine = TRUE;
		fprintf(fRtf, "\n{\\par\\li%d ", indent);
		break;
		
	case (QUOTATION | OFF):
	case (QUOTE | OFF):
		diagnostics(4,"Exiting CmdQuote");
		(void) PopBrace();
		indent -= 512;
		NoNewLine = FALSE;
		if (indent>0)
			fprintf(fRtf, "\\par}\n\\li%d ", indent);
		else
	    	fprintf(fRtf, "\\par}\n\\pard\\q%c ", alignment);
		break;
	}
}


void 
CmdList(int code)
/******************************************************************************
  purpose : converts the LaTeX-environments itemize, description and enumerate
	    to similar Rtf-styles
	    (only the begin/end-commands and not the commands inside the environment
	     see also function CmdItem)
parameter : code : type of environment and on/off-state
 globals  : nonewline, indent: look at funtction CmdQuote
 ******************************************************************************/
{
	switch (code) {
		case (ITEMIZE | ON):
		PushEnvironment(ITEMIZE);
		indent += 512;
		NoNewLine = FALSE;
		bNewPar = FALSE;
		break;
	case (ITEMIZE | OFF):
		PopEnvironment();
		indent -= 512;
		NoNewLine = FALSE;
		fprintf(fRtf, "\n\\par\\fi0\\li%d ", indent);
		bNewPar = TRUE;
		break;
	case (ENUMERATE | ON):
		PushEnvironment(ENUMERATE);
		g_enumerate_depth++;
		CmdItem(RESET_ITEM_COUNTER);
		indent += 512;
		NoNewLine = TRUE;
		bNewPar = FALSE;
		break;
	case (ENUMERATE | OFF):
		PopEnvironment();
		g_enumerate_depth--;
		indent -= 512;
		NoNewLine = FALSE;
		fprintf(fRtf, "\n\\par\\fi0\\li%d ", indent);
		bNewPar = TRUE;
		break;
	case (DESCRIPTION | ON):
		PushEnvironment(DESCRIPTION);
		indent += 512;
		NoNewLine = FALSE;
		bNewPar = FALSE;
		break;
	case (DESCRIPTION | OFF):
		PopEnvironment();
		indent -= 512;
		NoNewLine = FALSE;
		fprintf(fRtf, "\n\\par\\fi0\\li%d ", indent);
		bNewPar = TRUE;
		break;
	}
}

void 
CmdItem(int code)
/******************************************************************************
 purpose : makes the same as the function CmdList except that
	   here are only \-commands are handled and in the function
	   CmdList only the \begin or \end{environment}-command is handled
parameter : code : type of environment and on/off-state
 globals  : nonewline, indent: look at function CmdQuote
            bNewPar:
            fRtf: Rtf-file-pointer
 ******************************************************************************/
{
	char            itemlabel[100];
	static int      item_number[4];

	if (code == RESET_ITEM_COUNTER) {
		item_number[g_enumerate_depth] = 1;
		return;
	}

	diagnostics(4, "Entering CmdItem depth=%d item=%d",g_enumerate_depth,item_number[g_enumerate_depth]);
	fprintf(fRtf, "\n\\par\\fi0\\li%d ", indent);

	if (getBracketParam(itemlabel, 99)) {	/* \item[label] */

		fprintf(fRtf, "{\\b ");	/* bold on */
		diagnostics(5,"Entering ConvertString from CmdItem");
		ConvertString(itemlabel);
		diagnostics(5,"Exiting ConvertString from CmdItem");
		fprintf(fRtf, "}\\tab ");	/* bold off */
		
	}
	
	switch (code) {
	case ITEMIZE:
		fprintf(fRtf, "\\bullet\\tab ");
		break;

	case ENUMERATE:
		switch (g_enumerate_depth) {
		case 1:
			fprintf(fRtf, "%d.", item_number[g_enumerate_depth]);
			break;

		case 2:
			fprintf(fRtf, "(%c)", 'a' + item_number[g_enumerate_depth] - 1);
			break;

		case 3:
			roman_item(item_number[g_enumerate_depth], itemlabel);
			fprintf(fRtf, "%s.", itemlabel);
			break;

		case 4:
			fprintf(fRtf, "%c.", 'A' + item_number[g_enumerate_depth] - 1);
			break;
		}
		fprintf(fRtf, "\\tab ");
		item_number[g_enumerate_depth]++;
		break;

	case DESCRIPTION:
		fprintf(fRtf, "\\tab ");	/* indent */
		break;
	}
	
	Convert();
	diagnostics(4, "Exiting Convert() from CmdItem");
	bNewPar = FALSE;
}

void 
CmdBox( /* @unused@ */ int code)
/******************************************************************************
  purpose: converts the LaTeX \box-commands into  an similar Rtf-style
  globals: mbox
 ******************************************************************************/
{
	char           *s;
	
	if (g_processing_equation) {
		g_processing_equation = FALSE;
		fprintf(fRtf, "}");	/* close math italics */

		mbox = TRUE;
		s = getParam();
		diagnostics(4, "Entering ConvertString(%s) from CmdBox",s);
		ConvertString(s);
		diagnostics(4, "Exiting Convert() from CmdBox");
		free(s);
		
		mbox = FALSE;
		g_processing_equation = TRUE;
		fprintf(fRtf, "{\\i ");	/* reopen math italics */
	} else {
		mbox = TRUE;
		Convert();
		mbox = FALSE;
	}
}

void
CmdInclude(int code)
/******************************************************************************
 purpose: reads an extern-LaTeX-File from the into the actual document and converts it to
	  an similar Rtf-style
 globals: GermanMode: is set if germanstyles are included
          fTex
          progname
 ******************************************************************************/
{
	char            *fname;
	FILE *fp;
	FILE *LatexFile;
	char           *olatexname;
	long            oldlinenumber;

	strcpy(fname, "");

	fname = getParam();
	if (strstr(fname, "german.sty") != NULL) {
		GermanMode = TRUE;
		PushEnvironment(GERMANMODE);
		return;
	}
	assert(fname != NULL);

	if (strcmp(fname, "") == 0) {
		diagnostics(WARNING, "Empty or invalid filename in \\include{filename}");
		return;
	}
	/* extension .tex is appended automatically */
	if (strchr(fname, '.') == NULL)
		strcat(fname, ".tex");

/* 
   this is needed in classic MacOS because of the weird way that directories
   are handled under DropUnix
*/
#ifdef __MWERKS__
	{
	char            fullpath[1024];
		char           *dp;
		strcpy(fullpath, latexname);
		dp = strrchr(fullpath, ':');
		if (dp != NULL) {
			dp++;
			*dp = '\0';
		} else
			strcpy(fullpath, "");
		strcat(fullpath, fname);
		strcpy(fname, fullpath);
	}
#endif

	if (g_processing_include) {
		diagnostics(WARNING, "Cannot process nested \\include{%s}", latexname);
		return;
	}
	g_processing_include = TRUE;

	if ((fp = (fopen(fname, "rb"))) == NULL) {
		diagnostics(WARNING, "Cannot open \\include file: %s", latexname);
		return;
	}
	fprintf(stderr, "\nProcessing include file: %s\n", latexname);

	LatexFile = fTex;	/* Save current file pointer */
	diagnostics(5, "changing fTex file pointer in CmdInclude");
	fTex = fp;
	oldlinenumber = linenumber;
	linenumber = 1;
	olatexname = latexname;
	latexname = fname;

	diagnostics(4, "Entering Convert() from CmdInclude");
	Convert();
	diagnostics(4, "Exiting Convert() from CmdInclude");

	if (fclose(fp) != 0)
		diagnostics(ERROR, "Could not close include file.");

	diagnostics(5, "resetting fTex file pointer in CmdInclude");
	fTex = LatexFile;
	latexname = olatexname;
	linenumber = oldlinenumber;
	g_processing_include = FALSE;
	free(fname);
}

void
putRtfChar(char cThis)
/****************************************************************************
purpose: filter for characters being written to RTF file
 ****************************************************************************/
{
	if (cThis == '\\')
		fprintf(fRtf, "\\\\");
	else if (cThis == '{')
		fprintf(fRtf, "\\{");
	else if (cThis == '}')
		fprintf(fRtf, "\\}");
	else if (cThis == '\n') 
		fprintf(fRtf, "\n\\par ");
	else
		fprintf(fRtf, "%c", cThis);
}

void 
CmdVerb(int code)
/******************************************************************************
 purpose: converts the LaTeX-verb-environment to a similar Rtf-style
 ******************************************************************************/
{
	char            cThis;
	char            markingchar = '\177';
	int             num;

	while ((cThis = getRawTexChar())) {
		if ((cThis != ' ') && (cThis != '*') && !isalpha((int)cThis)) {
			markingchar = cThis;
			break;
		}
	}

	num = TexFontNumber("Typewriter");
	fprintf(fRtf, "{\\plain\\f%d ", num);

	while ((cThis = getRawTexChar()) && cThis != markingchar) 
		putRtfChar(cThis);
	
	fprintf(fRtf, "}");
}

void 
CmdVerbatim(int code)
/******************************************************************************
	convert characters 1:1 until \end{verbatim} or \end{Verbatim} is reached
	There has got to be a better way of doing this
******************************************************************************/
{
	char            endstring[] = "\\end{verbatim}";
	int             num, i = 0, j = 0;
	char            cThis;

	if (code & ON) {
		diagnostics(4, "Entering CmdVerbatim");
		num = TexFontNumber("Typewriter");
		fprintf(fRtf, "{\\plain\\f%d ", num);
	
		for (;;) {
			cThis = getRawTexChar();
			if ((cThis != endstring[i]) || ((i > 0) && (cThis == ' '))) {
				if (i > 0) {
					for (j = 0; j < i; j++) {
						if (j == 0)
							putRtfChar('\\');
						else
							putRtfChar(endstring[j]);
					}
					i = 0;
				}
				putRtfChar(cThis);
			} else {
				if (cThis != ' ')
					++i;
				if (i >=  strlen(endstring)) {               /* put \end{verbatim} back */
					for (i=strlen(endstring)-1; i>=0; i--)
						ungetTexChar(endstring[i]);
					return;
				}
			}
		}
	} else {
		diagnostics(4, "Exiting CmdVerbatim");
		fprintf(fRtf, "}");
	}
		
}

void 
CmdVerse(int code)
/******************************************************************************
  purpose: converts the LaTeX-Verse-environment to a similar Rtf-style
parameter: code: turns on/off handling routine
  globals: NoNewLine
           bNewPar
 ******************************************************************************/
{
	switch (code) {
		case ON:
		fprintf(fRtf, "\\par\n\\pard\\q%c\\fi-567\\li1134\\ri1134\\keep ", alignment);
		NoNewLine = FALSE;
		bNewPar = TRUE;
		break;
	case OFF:
		fprintf(fRtf, "\\par\n\\pard\\q%c ", alignment);
		bNewPar = TRUE;
		break;
	}
}


void 
CmdIgnoreDef( /* @unused@ */ int code)
/*****************************************************************************
 purpose: newenvironments or newcommands which are defined by the user aren't
	      converted into Rtf and so they must be ignored
 ******************************************************************************/
{
	char            cThis, *temp;

	while ((cThis = getTexChar()) && cThis != '{');
    ungetTexChar(cThis);
	temp = getParam();
	free(temp);
}

void 
TranslateGerman(void)
/***************************************************************************
purpose: called on active german-mode and " character in input file to
	 handle " as an active (meta-)character.
globals: reads from fTex and writes to fRtf
 ***************************************************************************/
{
	char            cThis;

	cThis = getTexChar();

	switch (cThis) {
	case 'a':
		fprintf(fRtf, "{\\'e4}");
		break;
	case 'o':
		fprintf(fRtf, "{\\'f6}");
		break;
	case 'u':
		fprintf(fRtf, "{\\'fc}");
		break;
	case 's':
		fprintf(fRtf, "{\\'df}");
		break;
	case '|':
		break;		/* ignore */
	case '-':
		break;		/* ignore */
	case '"':
		break;		/* ignore */
	case '\'':
		fprintf(fRtf, "\\ldblquote ");
		break;
	case '`':
		fprintf(fRtf, "\\rdblquote ");
		break;
	case '<':
		break;
	case '>':
		break;
	default:
		fprintf(fRtf, "%c", cThis);
	}
}

void 
CmdPrintRtf(int code)
/***************************************************************************
purpose: writes string to RTF file
globals: writes to fRtf
 ***************************************************************************/
{
	fprintf(fRtf, (char *) code);
}

void 
GermanPrint(int code)
{
	switch (code) {
		case GP_CK:fprintf(fRtf, "ck");
		break;
	case GP_LDBL:
		fprintf(fRtf, "\\ldblquote");
		break;
	case GP_L:
		fprintf(fRtf, "\\lquote");
		break;
	case GP_R:
		fprintf(fRtf, "\\rquote");
		break;
	case GP_RDBL:
		fprintf(fRtf, "\\rdblquote");
	}
}


void 
CmdIgnoreLet( /* @unused@ */ int code)
/******************************************************************************
     purpose : ignore \let 
	   Format: \let\XXXXX = \YYYYYY or \let\XXXXX\YYYYYY
 ******************************************************************************/
{
	char            cThis;

	while ((cThis = getTexChar()) && cThis != ' '  && cThis != '\\');

	if (cThis == ' ')
	{
		skipSpaces();
		while ((cThis = getTexChar()) && cThis != ' ');
		skipSpaces();
	}
}


/*************************************************************************
 *LEG250498
 * purpose: opens existing aux-file and scans for token, with value
 * reference.  Then outputs the corresponding ref-information to the
 * rtf-file parameters: code 0 = get the first parameter ( {refnumber}
 * -> refnumber)
 *                           1 = get the first par. of the first {
 * {{sectref}{line}} } -> sectref ) globals: input (name of
 * LaTeX-Inputfile) fRtf rtf-Outputfile
 * returns 1 if target is found otherwise 0
 ************************************************************************/

int 
ScanAux(char *token, char *reference, int code)
{
	static FILE    *fAux = NULL;
	static char     openbrace = '{';
	static char     closebrace = '}';
	char            tokenref[255];	/* should really be allocated
					 * dynmically */
	char            AuxLine[1024];
	char           *s;

	if (g_aux_file_missing || strlen(reference) == 0) {
		fprintf(fRtf, "?");
		return 0;
	}
	if (fAux == NULL && (fAux = fopen(AuxName, "r")) == NULL) {	/* open .aux file if not
									 * opened before */
		fprintf(stderr, "Error opening AUX-file: %s\n", AuxName);
		fprintf(stderr, "Try running LaTeX first\n");
		g_aux_file_missing = TRUE;
		fprintf(fRtf, "?");
		return 0;
	}
	rewind(fAux);
	sprintf(tokenref, "\\%s{%s}", token, reference);

	while (fgets(AuxLine, 1023, fAux) != NULL) {

		if ((s = strstr(AuxLine, tokenref)) != NULL) {
			s += strlen(tokenref);

			for (; code >= 0; code--) {	/* Do once or twice
							 * depending on code */
				s = strchr(s, openbrace);
				if (s == NULL) {	/* no parameter found,
							 * just print '?' and
							 * return */
					fprintf(fRtf, "?");
					return 0;
				}
				s++;
			}

			while (*s != closebrace && *s != '\0')	/* print the number and
								 * exit */
				fprintf(fRtf, "%c", *s++);

			return 1;
		}
	}

	diagnostics(WARNING, "\\%s{%s} not found in %s", token, reference, AuxName);
	fprintf(fRtf, "?");
	return 0;
}

void
CmdLabel(int code)
/******************************************************************************
  purpose : label, produce rtf-output in dependency of the rtfversion.
  parameter : code  kind of label, passed through  *** BROKEN by SAP ***
 ******************************************************************************/
{
	char           *text;
	char            cThis;

	text=getParam();
	free(text);
	return;
	
	if (code < HYPER) {
		text = getParam();
		ungetTexChar(text[strlen(text) - 1]);	/* somewhat screwy */
	} else {
		text = hyperref;
	}

	diagnostics(4, "Generating label/bookmark `%s'", text);

	if (rtf_restrict(1, 1))
		CmdLabelOld(code, text);
	if (rtf_restrict(1, 4))
		CmdLabel1_4(code, text);

	if (code >= HYPER)
		free(text);

	cThis = getTexChar();

	cThis = getNonSpace();

	cThis = getTexChar();

	if (cThis != '\n')
		ungetTexChar(cThis);

	/* LEG190498 */
}

void
CmdLabel1_4(int code, char *text)
/******************************************************************************
  purpose : label
  parameter : code  kind of label, text name of the label
 ******************************************************************************/
{
	switch (code) {
		case LABEL:
		/*
		 * Note that Hyperlabels do not exist, if they are
		 * encountered it's a severe bug
		 */

		fprintf(fRtf, "{\\*\\bkmkstart %s} {\\*\\bkmkend %s}", text, text);
		break;
	case REF:
	case HYPERREF:
		fprintf(fRtf, "{\\field\\fldlock{\\*\\fldinst REF %s  \\\\n}{\\fldrslt ", text);
		ScanAux("newlabel", text, 1);
		fprintf(fRtf, "}}");
		break;
	case PAGEREF:
	case HYPERPAGEREF:
		fprintf(fRtf, "{\\field{\\*\\fldinst PAGEREF %s}{\\fldrslt ?}}", text);
		break;
	default:
		diagnostics(ERROR, "Called CmdLabel with wrong Code %d", code);
	}
}


void
CmdLabelOld(int code, char *text)
/******************************************************************************
     purpose : label
   parameter : code  kind of label
 ******************************************************************************/
{
	switch (code) {
		case LABEL:
		fprintf(fRtf, "{\\v[LABEL: %s]}", text);
		break;
	case REF:
	case HYPERREF:
		fprintf(fRtf, "{\\v[REF: %s]}", text);
		break;
	case PAGEREF:
	case HYPERPAGEREF:
		fprintf(fRtf, "{\\v[PAGEREF: %s]}", text);
		break;
	default:
		diagnostics(ERROR, "Called CmdLabel with wrong Code %d", code);
	}
}

void 
IgnoreNewCmd( /* @unused@ */ int code)
/******************************************************************************
     purpose : ignore \newcmd
   parameter : code  not used
 ******************************************************************************/
{
	char            cThis;

	/* ignore first '{' */
	cThis = getTexChar();
	ungetTexChar(cThis);

	if (cThis == '\\')
		CmdIgnoreDef(0);
	else
		CmdIgnoreParameter(No_Opt_Two_NormParam);
}

void CmdQuad(int kk)
/******************************************************************************
 purpose: inserts kk quad spaces (D. Taupin)
 ******************************************************************************/
{
	int z;	
	fprintf(fRtf,"{\\emspace ");
	for (z=0; z<kk; z++) fprintf(fRtf," ");
	fprintf(fRtf,"}");
}

void CmdSpace(float kk)
/******************************************************************************
 purpose: inserts a space of width kk*space 
 ******************************************************************************/
{
	int size = CurrentFontSize()*kk;
	fprintf(fRtf,"{\\fs%d  }", size);
}

void 
CmdFigure(int code)
/******************************************************************************
  purpose: Process \begin{figure} ... \end{figure} environment
 ******************************************************************************/
{
	char            loc[5];

	if (code & ON) {
		getBracketParam(loc,4);
		diagnostics(4, "entering CmdFigure [%s]", loc);
		g_processing_figure = TRUE;
	} else {
		g_processing_figure = FALSE;
		diagnostics(4, "exiting CmdFigure");
	}
}

void 
CmdIgnoreFigure(int code)
/******************************************************************************
  purpose: function to read Picture,Bibliography and Minipage Environment
parameter: code: which environment to ignore
 ******************************************************************************/
{
	char            environ[30];
	char            thechar;

	switch (code & ~(ON)) {	/* mask MSB */
	
	case PICTURE:
			strcpy(environ, "picture");
			break;
	
	case MINIPAGE:
			strcpy(environ, "minipage");
			break;
	
	case THEBIBLIOGRAPHY:
			strcpy(environ, "thebibliography");
			break;
	
	default:
			fprintf(stderr, "CmdIgnoreFigure called with unknown environment\n");
			return;
	}

	while ((thechar = getTexChar())) {
		char *thisEnviron;
		if (thechar == '\\') {
			if (getTexChar() != 'e') break;
			if (getTexChar() != 'n') break;
			if (getTexChar() != 'd') break;
			skipSpaces();
			thisEnviron = getParam();
			if (strcmp(environ,thisEnviron) == 0) {
				free(thisEnviron);
				return;
			}
		}
	}
	return;
}

/******************************************************************************
CmdLink:

  purpose: hyperlatex support. function, which translates the first parameter
           to the rtf-file and ignores the second, the proposed optional
	   parameter is also (still) ignored.
  parameter: not (yet?) used.

  The second parameter should be remembered for the \Cite (\Ref \Pageref)
  command.
  globals: hyperref, set to second Parameter

The first parameter of a \link{anchor}[ltx]{label} is converted to the
rtf-output. Label is stored to hyperref for later use, the optional
parameter is ignored.
[ltx] should be processed as Otfried recommends it, to use for
exclusive latex output.e.g:

	\link{readhere}[~\Ref]{explaining:chapter}.

Since {explaining:chapter} is yet read by latex and hyperlatex when
[...] is evaluated it produces the correct reference. We are only
strolling from left to right through the text and can't remember what
we will see in the future.

 ******************************************************************************/
void
CmdLink(int code)
{
	char           *param2;
	char            optparam[255] = "";

	diagnostics(4, "Entering hyperlatex \\link command");
	Convert();		/* convert routine is called again for
				 * evaluating the contents of the first
				 * parameter */
	diagnostics(4, "  Converted first parameter");

	getBracketParam(optparam, 255);
	/* LEG190498 now should come processing of the optional parameter */
	diagnostics(4, "  Converted optional parameter");

	param2 = getParam();
	diagnostics(4, "  Converted second parameter");

	if (hyperref != NULL)
		free(hyperref);

	hyperref = (char *) malloc((strlen(param2) + 1));
	if (hyperref == NULL)
		error(" malloc error -> out of memory!\n");

	strcpy(hyperref, param2);
	free(param2);
	/* LEG210698*** better? hyperref = param2 */
}

void 
CmdColumn(int code)
/******************************************************************************
  purpose: chooses between one/two-columns
parameter: number of columns
 globals: twocolumn: true if twocolumn-mode is set
 ******************************************************************************/
{
	switch (code) {
		case One_Column:fprintf(fRtf, "\\page \\colsx709\\endnhere ");	/* new page & one column */
		twocolumn = FALSE;
		break;
	case Two_Column:
		fprintf(fRtf, "\\page \\cols2\\colsx709\\endnhere ");	/* new page & two
									 * columns */
		twocolumn = TRUE;
		break;
	}			/* switch */
}

void 
CmdNewPage(int code)
/******************************************************************************
  purpose: starts a new page
parameter: code: newpage or newcolumn-option
 globals: twocolumn: true if twocolumn-mode is set
 ******************************************************************************/
{
	switch (code) {
		case NewPage:fprintf(fRtf, "\\page ");	/* causes new page */
		break;
	case NewColumn:
		if (twocolumn)
			fprintf(fRtf, "\\column ");	/* new column */
		else
			fprintf(fRtf, "\\page ");	/* causes new page */
		break;
	}			/* switch */
}

void 
Cmd_OptParam_Without_braces( /* @unused@ */ int code)
/******************************************************************************
 purpose: gets an optional parameter which isn't surrounded by braces but by spaces
 ******************************************************************************/
{
	char            cNext = ' ';
	char            cLast = ' ';

	do {
		cLast = cNext;
		cNext = getTexChar();
	} while ((cNext != ' ') &&
		 (cNext != '\\') &&
		 (cNext != '{') &&
		 (cNext != '\n') &&
		 (cNext != ',') &&
		 ((cNext != '.') || (isdigit((unsigned char) cLast))) &&
	/*
	 * . doesn't mean the end of an command inside an number of the type
	 * real
	 */
		 (cNext != '}') &&
		 (cNext != '\"') &&
		 (cNext != '[') &&
		 (cNext != '$'));

	ungetTexChar(cNext);
}

void 
CmdBottom(int code)
/******************************************************************************
  purpose: ignore raggedbottom command
 ******************************************************************************/
{
}

/******************************************************************************
parameter: code: on/off-option
 globals : article and titlepage from the documentstyle
 ******************************************************************************/
void
CmdAbstract(int code)
{
	static char     oldalignment;

	fprintf(fRtf, "\n\\par\n\\par\\pard ");
	if (code == ON) {
		if (!article || !titlepage) 
			fprintf(fRtf, "\\page");

		fprintf(fRtf, "\\pard\\qj ");
		fprintf(fRtf, "{\\b\\fs%d ", CurrentFontSize());
		ConvertBabelName("ABSTRACTNAME");
		fprintf(fRtf, "}\\par ");
		oldalignment = alignment;
		alignment = JUSTIFIED;
	} else {
		fprintf(fRtf, "\\pard ");
		alignment = oldalignment;
		fprintf(fRtf, "\n\\par\\q%c ", alignment);
	}
}

void 
CmdTitlepage(int code)
/******************************************************************************
  purpose: \begin{titlepage} ... \end{titlepage}
           add pagebreaks before and after this environment
 ******************************************************************************/
{
	switch (code) {
		case ON:
		fprintf(fRtf, "\n\\par\\pard \\page ");	/* new page */
		fprintf(fRtf, "\n\\par\\q%c ", alignment);
		break;
	case OFF:
		fprintf(fRtf, "\\pard ");
		fprintf(fRtf, "\n\\par\\q%c \\page ", alignment);
		break;
	}			/* switch */
}

void 
CmdMultiCol( /* @unused@ */ int code)
/******************************************************************************
 purpose: converts the LaTex-Multicolumn to a similar Rtf-style
	  this converting is only partially
	  so the user has to convert some part of the Table-environment by hand
 ******************************************************************************/
{
	char            inchar[10];
	char            numColStr[100];
	long            numCol, i, toBeInserted;
	char            colFmtChar = 'u';
	char           *eptr;	/* for srtol   */
	static bool     bWarningDisplayed = FALSE;

	if (!bWarningDisplayed) {
		diagnostics(WARNING, "Multicolumn: Cells must be merged by hand!");
		bWarningDisplayed = TRUE;
	}
	i = 0;
	do {
		inchar[0] = getTexChar();
		if (isdigit((unsigned char) inchar[0]))
			numColStr[i++] = inchar[0];
	}
	while (inchar[0] != '}');
	numColStr[i] = '\0';
	numCol = strtol(numColStr, &eptr, 10);
	if (eptr == numColStr)
		error(" multicolumn: argument num invalid\n");


	do {
		inchar[0] = getTexChar();
		switch (inchar[0]) {
		case 'c':
		case 'r':
		case 'l':
			if (colFmtChar == 'u')
				colFmtChar = inchar[0];
			break;
		default:
			break;
		}
	}
	while (inchar[0] != '}');
	if (colFmtChar == 'u')
		colFmtChar = 'l';

	switch (colFmtChar) {
	case 'r':
		toBeInserted = numCol;
		break;
	case 'c':
		toBeInserted = (numCol + 1) / 2;
		break;
	default:
		toBeInserted = 1;
		break;
	}

	for (i = 1; i < toBeInserted; i++, actCol++) {
		fprintf(fRtf, " \\cell \\pard \\intbl ");
	}
	fprintf(fRtf, "\\q%c ", colFmtChar);

	diagnostics(4, "Entering Convert() from CmdMultiCol()");
	Convert();
	diagnostics(4, "Exiting Convert() from CmdMultiCol()");

	for (i = toBeInserted; (i < numCol) && (actCol < colCount); i++, actCol++) {
		fprintf(fRtf, " \\cell \\pard \\intbl ");
	}


}

void 
CmdColsep(int code)
/***************************************************************************
 * purpose: hyperlatex support, handles '&' as in Convert() in convert.c
 ***************************************************************************/
{
	if (!g_processing_tabular) {
		fprintf(fRtf, "{\\'a7}");
		return;
	}
	actCol++;

	if (g_processing_equation) {	/* means that we are in an eqnarray
					 * or array */
		fprintf(fRtf, "\\tab ");
	} else {
		fprintf(fRtf, " \\cell \\pard \\intbl ");
		if (colFmt == NULL)
			diagnostics(WARNING, "Fatal, Fatal! CmdColsep called whith colFmt == NULL.");
		else
			fprintf(fRtf, "\\q%c ", colFmt[actCol]);
	}
}

void 
CmdGraphics(int code)
{
	char            options[255];
	char            fullpath[1023];
	char           *filename;
	int             cc, i;
	short           top, left, bottom, right;
	FILE           *fp;

	/* could be \includegraphics*[0,0][5,5]{file.pict} */

	getBracketParam(options, 255);
	getBracketParam(options, 255);
	filename = getParam();

	if (strstr(filename, ".pict") || strstr(filename, ".PICT")) {
		/* SAP fixes for Mac Platform */
#ifdef __MWERKS__
		{
		char           *dp;
		strcpy(fullpath, latexname);
		dp = strrchr(fullpath, ':');
		if (dp != NULL) {
			dp++;
			*dp = '\0';
		} else
			strcpy(fullpath, "");
		strcat(fullpath, filename);
		}
#else
		strcpy(fullpath,filename);
#endif

		fprintf(stderr, "processing picture %s\n", fullpath);
		fp = fopen(fullpath, "rb");

		if (fseek(fp, 514L, SEEK_CUR) ||     /* skip 512 byte header + 2 bytes for version info */
		    (fread(&top, 2, 1, fp) < 1) ||    /* read the pict file dimensions in points */
		    (fread(&left, 2, 1, fp) < 1) || 
			(fread(&bottom, 2, 1, fp) < 1) || 
			(fread(&right, 2, 1, fp) < 1) || 
			fseek(fp, -10L, SEEK_CUR)) {    /* move back ten bytes so that entire file will be encoded */
				free(filename);
				fclose(fp);
				return;
			}
		fprintf(fRtf, "\n{\\pict\\macpict\\picw%d\\pich%d\n", right - left, bottom - top);

		i = 0;
		while ((cc = fgetc(fp)) != EOF) {
			fprintf(fRtf, "%.2x", cc);
			if (++i > 126) {
				i = 0;
				fprintf(fRtf, "\n");
			}	/* keep lines 254 chars long */
		}

		fprintf(fRtf, "}\n");
		fclose(fp);
		free(filename);
	}
}
