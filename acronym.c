/* acronym.c - interpret acronym commands

Copyright (C) 1995-2002 The Free Software Foundation

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

This file is available from http://sourceforge.net/projects/latex2rtf/
 
Authors:
    1995      Fernando Dorner, Andreas Granzer, Freidrich Polzer, Gerhard Trisko
    1995-1997 Ralf Schlatterbeck
    1998-2000 Georg Lehner
    2001-2002 Scott Prahl
    
    2009-2010 Pedro A. Aranda
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "main.h"
#include "convert.h"
#include "funct1.h"
#include "commands.h"
#include "stack.h"
#include "fonts.h"
#include "cfg.h"
#include "ignore.h"
#include "utils.h"
#include "encodings.h"
#include "parser.h"
#include "counters.h"
#include "lengths.h"
#include "definitions.h"
#include "preamble.h"
#include "xrefs.h"
#include "equations.h"
#include "direct.h"
#include "styles.h"
#include "graphics.h"
#include "vertical.h"

#include "aux.h"
#include "acronym.h"
#include "biblio.h"
#include "labels.h"

/* *****************************************************
   Acronyms:
   this version is quite aligned with acronyms.sty v1.35
   it supports irregular plurals (...plural macros)
   

   \begin{acronym}[HBCI]

   In standard mode, the acronym-list will consist of all defined
   acronyms, regardless if the the acronym was used in the text
   before or not.

   \usepackage{acronym} accepts following parameters:

   * printonlyused
   * withpage
   
   TODO:
   implement taking the bracket parameter into account.

   Date:   29-Dec-2009
   Author: Pedro A. Aranda Gutiérrez
   ***************************************** */

void UsePackageAcronym(char *options)
{
    diagnostics(WARNING,"acronym package support: almost v1.35");
    /*  we need an .aux file to get the acronyms from */
    LoadAuxFile();

    if (options != NULL) {
	char *acroOptions = strdup(options);
	char *opt;
	diagnostics(WARNING,"acronym package with option%c %s",
		    strHasChar(options,',') ? 's' : ' ',
		    options);
	for (opt=strtok(acroOptions,",");
	     NULL != opt;
	     opt=strtok(NULL,",")) {
	    if (streq(opt,"printonlyused")) {
		acroPrintOnlyUsed = TRUE;
	    } else if (streq(opt,"withpage")) {
		acroPrintWithPage = TRUE;
	    } else {
		diagnostics(WARNING,"acronym package: option %s unknown",opt);
	    }
	}
	free(acroOptions);
    }
}

static int inAcroEnvironment = FALSE;

void CmdBeginAcronym(int code)
{
    /*  diagnostics(5,"CmdBeginAcronym(0x%04x) ON = 0x%04x",code,ON); */
    
    if (ON == (code & ON)) {
	char *longest = getBracketParam();
	if (NULL != longest) {
	    diagnostics(WARNING,"Ignoring longest acronym hint '[%s]'",longest);
	    free(longest);
	}
	inAcroEnvironment = TRUE;
	PushEnvironment(ACRONYM_MODE);
    } else {
	inAcroEnvironment = FALSE;
	PopEnvironment();
    }
}

/*
 * This table holds all the acronym information
 * With irregular plurals it is extremely difficult
 * to get it on the fly from the .aux file
 */
typedef struct _acroEntry {
    char *acDef, *acShort, *acLong;
    char *acShortPlural, *acLongPlural;
    int used;
    int printable;
} acroEntry;

static acroEntry *acroTable = NULL;
static int        acroNum = 0;
int               acroPrintOnlyUsed = FALSE;
int               acroPrintWithPage = FALSE;

/*
 * acronym table handling routines:
 *
 * search an entry in the table...
 * linear search, OK not completely efficient, but portable
 *
 */
acroEntry *searchEntry(char *acDef)
{
    int i;
 
    if (acDef != NULL) {
	for (i = 0; i<acroNum; i++) {
	    if (0 == strcmp(acDef,acroTable[i].acDef)) {
		return &acroTable[i];
	    }
	}
	/* 	diagnostics(WARNING,"Undefined acronym '%s'",acDef); */
    }
    return NULL;
}


/* auxiliary functions */
static void ConvertFmtString(char *fmt,char *str)
{
    char *buffer = malloc(strlen(fmt)+strlen(str)+1);
    sprintf(buffer,fmt,str);
    ConvertString(buffer);
}

static void ConvertFmt2String(char *fmt,char *s1,char *s2)
{
    char *buffer = malloc(strlen(fmt)+strlen(s1)+strlen(s2)+1);
    sprintf(buffer,fmt,s1,s2);
    ConvertString(buffer);
}

/* ********************************
   print the acronym long expansion
   ******************************** */

static void printLong(acroEntry *entry,int plural)
{
    if (FALSE == plural) {
	ConvertString(entry->acLong);
    } else {
	if (NULL != entry->acLongPlural) {
	    ConvertString(entry->acLongPlural);
	} else {
	    ConvertString(entry->acLong);
	    ConvertString("s");
	}
    }
}

/* *********************************
   print the acronym short expansion
   ********************************* */

static void printShort(acroEntry *entry,int plural)
{
    if (FALSE == plural) {
	ConvertString(entry->acShort);
    } else {
	if (NULL != entry->acShortPlural) {
	    ConvertString(entry->acShortPlural);
	} else {
	    ConvertString(entry->acShort);
	    ConvertString("s");
	}
    }
}

/* ***************************************************
   Create a regular plural
   Now I only need it once when defining a long plural
   *************************************************** */
static char* regularPlural(char *singular)
{
    int singularLength = strlen(singular);
    char *result = malloc(singularLength+2);
    if (NULL != result) {
	strcpy(result,singular);
	result[singularLength]='s';
	result[singularLength+1]='\0';
    }
    return result;
}

/* *******************************
   create a new entry in the table
   or use one if it exists
   ******************************* */
static acroEntry *createEntry(char *acDef) {
    acroEntry *result = searchEntry(acDef);
    if (NULL == result) {
	acroTable = realloc(acroTable,(++acroNum)*sizeof(acroEntry));
	if (NULL != acroTable) {
	    result = &acroTable[acroNum-1];
	    result->acDef         = acDef;
	    result->acShort       = NULL;
	    result->acLong        = NULL;
	    result->acShortPlural = NULL;
	    result->acLongPlural  = NULL;
	    result->used          = FALSE;
	    result->printable     = FALSE;
	}
    }
    return result;
}

/* ********************************
   Common acro creation diagnostics
   ******************************** */

static void acroDiag(acroEntry *thisEntry)
{
    register int diagLevel = 5;
    int entry = thisEntry - acroTable;
 
    diagnostics(diagLevel,"defining acronym");
    diagnostics(diagLevel," acroTable[%d].acDef         = %s",entry,thisEntry->acDef);
    if (NULL != thisEntry->acShort)
	diagnostics(diagLevel," acroTable[%d].acShort       = %s",entry,thisEntry->acShort);
    if (NULL != thisEntry->acLong)
	diagnostics(diagLevel," acroTable[%d].acLong        = %s",entry,thisEntry->acLong);
    if (NULL != thisEntry->acShortPlural)
	diagnostics(diagLevel," acroTable[%d].acShortPlural = %s",entry,thisEntry->acShortPlural);
    if (NULL != thisEntry->acLongPlural)
	diagnostics(diagLevel," acroTable[%d].acLongPlural  = %s",entry,thisEntry->acLongPlural);
}

/* **********************************************************
   Ignore \acroplural,\acrodef and \acrodefplural
   Handle \acro (in the acronym environment) and
          \newacro and \newacroplural (in the .aux file) only
	  *************************************************** */

void CmdAcrodef(int code)
{
    char *acDef = NULL, *acShort = NULL, *acLong = NULL;
    acDef   = getBraceParam();
    acShort = getBracketParam();
    acLong  = getBraceParam();

    /*  diagnostics(5,"void CmdAcrodef(%d)",code); */
    
    if (code == ACRONYM_NEWACRO) {
	acroEntry *thisEntry = createEntry(acDef);
	
	if (acShort == NULL)
	    acShort = acDef;

	if (NULL != thisEntry) {
	    thisEntry->acShort    = acShort;
	    thisEntry->acLong     = acLong;

	    acroDiag(thisEntry);
	} else {
	    diagnostics(ERROR,"Out of memory!");
	}
    } else if (code == ACRONYM_NEWACROPLURAL) {
	acroEntry *thisEntry = createEntry(acDef);
 
	if (NULL != thisEntry) {
	    thisEntry->acShortPlural = (acShort == NULL) ? regularPlural(acDef) : acShort;
	    thisEntry->acLongPlural  = acLong;
	    acroDiag(thisEntry);
	} else {
	    diagnostics(ERROR,"Out of memory!");
	}
    } else if (code == ACRONYM_ACRO) {
	/*
	 * just to know whether we have to print it or not!
	 * use acShort and acLong from the definition
	 */
	acroEntry *thisEntry = searchEntry(acDef);
	if (NULL != thisEntry) {
    
	    int doPrint =
		/*  (inAcroEnvironment == TRUE) &&  */
		((thisEntry->used == TRUE) ||
		 (acroPrintOnlyUsed == FALSE));
 
	    /*  diagnostics(5,"\\acro('%s','%s')",(NULL != acShort) ? acShort:acDef,acLong); */
 
	    if (doPrint) {
		int vspace = getLength("itemsep") + getLength("parsep");
		CmdEndParagraph(0);
		setVspace(vspace);
		
		CmdIndent(INDENT_USUAL);
		startParagraph("acronym", FIRST_PARAGRAPH);
		fprintRTF("{\\b ");
		ConvertString((NULL != acShort) ? acShort : acDef); 
		fprintRTF("}\\tab\n");
		ConvertString(acLong);
		if (TRUE == acroPrintWithPage) {
		    /*
		     * use alloca to dispose automatically when exiting
		     * block
		     */
		    char *acroLabel = alloca(strlen(acDef)+8);
		    char *acroPage;
		    sprintf(acroLabel,"acro:%s",acDef);
		    acroPage = getLabelPage(acroLabel);
		    if (NULL != acroPage) {
			fprintRTF("\\~\\tab\\~");
			ConvertString(acroPage);
		    }
		}
	    }
	} else {
	    diagnostics(WARNING,"Undefined acronym '%s'",acDef);
	}
    }
}

/*  ignore the first, push the second */
/*  \AC@hyperlink{isis}{IS-IS} */
   
void CmdAC(int code)
{
    char *shortAc;

    shortAc = getTexUntil("hyperlink",FALSE);
    if (NULL != shortAc) free(shortAc);

    ignoreBraceParam();

    shortAc = getBraceParam();
    if (shortAc != NULL) {
	ConvertString(shortAc);
	diagnostics(5,"in CmdAC(), pushing '%s'",shortAc);
    }
}
/*
  \ac{acronym}  	Expand and identify the acronym the first time; use only the acronym thereafter
  \acf{acronym} 	Use the full name of the acronym.
  \acs{acronym} 	Use the acronym, even before the first corresponding \ac command
  \acl{acronym} 	Expand the acronym without using the acronym itself.

  \acf{SW}  	Scientific Word (SW) documents are beautifully typeset.
  \acs{SW} 	SW documents are beautifully typeset.
  \acl{SW} 	Scientific Word documents are beautifully typeset.

  \acp{label}
  plural form of acronym by adding an s. \acfp. \acsp, \aclp work as well.

  \acfi{label}  Now like \acf{label}    
*/
void CmdAc(int code)
{
    char *ac;
    acroEntry *found = NULL;
    int  basecode = (code & 0x0ff);
    int plural    = ((code & 0xf00) == ACRONYM_PLURAL);
    int star      = ((code & 0xf00) == ACRONYM_STAR);
    int markUsed  = ((code & 0xf00) == ACRONYM_USED);
 
    ac   = getBraceParam();
    found = searchEntry(ac);
 
    if (found == NULL) {
	if (basecode == ACRONYM_AC)
	    ConvertFmt2String("\\textsf{\\textbf{%s! (%s!)}}",ac,ac);
	else
	    ConvertFmtString("\\textsf{\\textbf{%s!}}",ac);
	diagnostics(WARNING,"Undefined acronym '%s'",ac);
	return;
    }
 
    if (basecode == ACRONYM_AC) {
	if (found->used == TRUE) {
	    basecode = ACRONYM_ACS;
	} else {
	    found->used = (star == FALSE);
	    basecode = ACRONYM_ACF;
	}
    }
 
    if (markUsed == TRUE && star == FALSE)
	found->used = TRUE;
 
    switch(basecode) {
    case ACRONYM_ACF:
	printLong(found,plural);
	ConvertString(" (");
	printShort(found,plural);
	ConvertString(")");
	break;
    case ACRONYM_ACS:
	printShort(found,plural);
	break;
    case ACRONYM_ACL:
	printLong(found,plural);
	break;
    case ACRONYM_ACFI:
	ConvertFmt2String("\\textit{%s} (%s)",
 			  found->acLong,
 			  found->acShort);
	break;
    default:
	diagnostics(WARNING,"Fell through CmdAc()!");
	break;
    }
} 

/*  \acused{ac} */
/*  mark an acronym as used */

void CmdAcUsed(int code)
{
    acroEntry *acronym = NULL;
    char      *ac      = getBraceParam();
    acronym = searchEntry(ac);
    if (acronym != NULL) {
	acronym->used = TRUE;
    }
}

/*  \acresetall */
/*     resets all acronyms to not used. */
/*     Useful after the abstract to redefine */
/*     all acronyms in the introduction.  */

void CmdAcResetAll(int code)
{
    int i;
    for (i=0;i<acroNum;i++)
	acroTable[i].used = FALSE;
}

/*  \acroextra */
/*    push the contents for further conversion */

void CmdAcroExtra(int code)
{
    char      *ac      = getBraceParam();
    if (ac != NULL) {
	ConvertString(ac);
    }
}
