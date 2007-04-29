/***************************************************************************
  Name:         conf.c
  Description:  Handling of configuration files.
  Created:      Fri Aug 26 03:29:08 EEST 2005
  Copyright:    (C) 2003 by Kalle "Gobol" Kankare
  Email:        kalle.kankare@tut.fi
 ***************************************************************************/

/***************************************************************************

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 ***************************************************************************/

/***************************************************************************
  Second try on configuration file parser

  Format of files:
  #comment
  [header]
  textidentifier = "textvalue"
  intidentifier = "00012"
  floatidentifier = "0.00123"

  multilineident = " piip 
   text ends here ->"

  identident = " thisident = \"valueinvalue\""

  Headers and identifiers can have the following characters [a-zA-Z0-9_]*

  Values can have any characters except unescaped "'s:  (\"[^\"]*)

  Values cannot have comments inside them (they are read as values)

  Comments begin with # and end with \n  

  ***************************************************************************/

#include "defines.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "llist.h"
#include "iolet.h"
#include "conf.h"

#include "useful.h"

/*
  Miten tehdään:

  tarvitaan sisäinen funktio joka palauttaa kokonaisuuksia itse parserille.
  eli sille annetaan alku ja loppumerkit ja se palauttaa niiden välisen 
  tekstin, vaikka se olisi pilkkoutuneena monelle eri "riville" (siis jos 
  getlinen bufferin pituus ei riitä).

  Toinen ongelma oli grammarin kanssa, eli noi valueiden numeroinnit oli
  globaaleja (kato sequconfig.c)
  -tarvitaan uusi rakenne noihin syöte ja tulosterakenteisiin:

  syöterakenteet:

  header
  -nimi
  -lista identifiereitä 

  identifier
  -nimi
  -tyyppi


  tulosterakenteet:

  outheader
  -osoitin headeriin
  -lista valueita

  value
  -samanlainen kuin PR_Value eli
  -identifierin numero headerissa (ushort)
  -tulos merkkijonona/inttinä/floattina

  Syötteet ja tulosteet ovat NULL päätteisiä listoja jos muuta ei mainita.

  
  Notaatio:
  
  Globaalit funktiot ja rakenteet: conf_ etuliite 

  EI isoja kirjaimia funktioissa

  ulospäin näkyvät funktiot

  outheader *conf_parse_iolet(header *,iolet)
  tvalue conf_free_output(output)
  tvalue conf_print_output(iolet,header *,outheader *)

*/



/***************************************/

/* dfa tables */

/* this has the new state when inputted old state and character */
/* here a = a-zA-Z0-9_ and . = ascii 32-126 */
static const char lex_dfa_table[6][9] = 
{
  /* States         Characters                 */
  /*                 #  w  n  [  ]  "  a  .  = */
  /* whitespace */ { 1, 0, 0, 3,-1, 4, 2,-1, 5},
  /* commment   */ { 1, 1, 0, 1, 1, 1, 1, 1, 1},
  /* identifier */ { 1, 0, 0, 3,-1, 4, 2,-1, 5},
  /* header     */ {-1,-1,-1,-1, 0,-1, 3,-1,-1},
  /* value      */ { 4, 4, 4, 4, 4, 0, 4, 4, 4},
  /* equal      */ { 1, 0, 0, 3,-1, 4, 2,-1, 5}
};

static const char syn_dfa_table [4][4] = 
{
  /* States       Input      */
  /*              i  h  v  e*/
  /* empty   */ {-1, 1,-1,-1},
  /* headin  */ { 2, 1,-1,-1},
  /* identin */ {-1,-1,-1, 3},
  /* equin   */ {-1,-1, 1,-1}
};

/* proper return values of conf_lexer */
#define CONF_LEX_IDENT  1
#define CONF_LEX_HEADER 2
#define CONF_LEX_VALUE  3
#define CONF_LEX_EQUAL  4


/* functions */

/* acceptable character */
static inline tvalue accept_char(unsigned char khar)
{
  if(    (unsigned char) (khar - 'A') <= 26 
      || (unsigned char) (khar - 'a') <= 26
      || (unsigned char) (khar - '0') <= 9
      || khar == '_')
    return TRUE;
  else
    return FALSE;
}

/* constructs the outputstring of conf_lexer */
static tvalue record_char(char c, char **finalbuf, unsigned int *flen,
  unsigned int pos)
{
  if(*finalbuf == NULL || strlen(*finalbuf)+1 >= *flen)
  {
    char *tmp;

    if(*finalbuf != NULL)
      *flen*=2;

    tmp=realloc(*finalbuf,*(flen)+1);
    if(!tmp)
    {
      nullify(*finalbuf);
      print_err("Error: Out of memory in the lexer.\n");
      return FALSE;
    }

    if(!(*finalbuf))
      memset(tmp,0,*(flen)+1);

    *finalbuf=tmp;
  }

  (*finalbuf)[pos]=c;
  (*finalbuf)[pos+1]=0;

  return TRUE;
}

/* a lexer
   return values: -2 error, -1 eof.
   the old_oldstate is for subsequent calls of this.
   Assumes: *ret and *retlen are NULL and 0, respectively, in the beginning.
    */
static int conf_lexer(iolet *input, char **ret, unsigned int *retlen,
  unsigned int *lineno, int *old_state)
{
  unsigned int oldstate=0,symchar=5;
  char in,last=0;
  unsigned int pos=0;
  int state=*old_state;

  if(*retlen == 0)
    *retlen=128;
  else
    **ret=0; /* invalidize the previous string */

  while((in=iolet_in_char(input)) != -1)
  {
    /* interpret the character */
    switch(in)
    {
    case '#':
      symchar=0;
      break;
    case '[':
      symchar=3;
      break;
    case ']':
      symchar=4;
      break;
    case '\n':
      (*lineno)++;
      symchar=2;
      break;
    case ' ':
    case '\t':
      symchar=1;
      break;
    case '\"':
      symchar=5;
      break;
    case '=':
      symchar=8;
      break;
    default:
      /* characters  a-zA-Z0-9_ */
      if(accept_char(in) == TRUE)
        symchar=6;
      /* other characters */
      else
        symchar=7;
      break;
    }
    /* get the new state */
    oldstate=state;
    state=lex_dfa_table[state][symchar];

    /* parse error */
    if(state == -1)
      return -2;

    /* accept the \"-escapes in value */
    if(in == '\"' && oldstate == 4 && last == '\\')
      state=4;

    //    print_debug("merkki: [%c] uusi state %d\n",in,state);

    /* record the character */
    if(state >= 2 && state <=4)
      /* dont record if is the first character of header or value */
      if(state == 2 || oldstate == (unsigned)state)
      {
        if(record_char(in,ret,retlen,pos))
          pos++;
        else
          return -2;
      }

    /* handle a state change (from an important state) */
    if(oldstate >= 2 && oldstate <= 5 && oldstate != (unsigned)state)
    {
      *old_state=state;
      return oldstate-1;
    }
    
    last=in;
  }

  if(iolet_in_eof(input) == FALSE)
  {
    print_err("Error: Could not read a character from IOlet: \"%s\"",
      strerror(errno));
    
    return -2;
  }
  else
    return -1;
}

/* the parser */
/* this ignores undefined identifiers or headers. */
conf_out_value *conf_parse_iolet(conf_header *headers, iolet *file)
{
  register int beta=0,gamma=0;
  unsigned int idcount=0,curvalue;

  unsigned int line=0,slen=0;
  char *str=NULL;
  int lexout;

  const char *syms[] = {"identifier", "header", "value", "equal-sign"};

  unsigned int synstate=0,oldsynstate=0;
  int state=0;

  conf_out_value *ret=NULL;

  ARG_ASSERT(!file || !headers,NULL);

  /* calculate the number of identifiers in headers */
  for(beta=0;headers[beta].idents != NULL;beta++)
    for(gamma=0;headers[beta].idents[gamma].name != NULL;gamma++)
      idcount++;

  /* allocate the memory for output */
  /* this allocates the maximum possible amount of memory (for all defined 
     identifiers) */
  ret=malloc(sizeof(conf_out_value)*(idcount+1));
  if(!ret)
    return NULL;

  /* invalidize the headers*/
  for(beta=0;(unsigned)beta<idcount+1;beta++)
    ret[beta].header=-1;

  //print_debug("identifiereitä oli %d kpl\n",idcount);

  curvalue=0;

  /* lex the input */
  while((lexout=conf_lexer(file,&str,&slen,&line,&state)) >= 0)
  {
    oldsynstate=synstate;
    synstate=syn_dfa_table[synstate][lexout-1];
    
    /* interpret the statechange */
    switch(synstate)
    {

    /* header/value received */
    case 1: 
      if(lexout == CONF_LEX_HEADER)
      {
        /* search for the proper header */
        for(beta=0;headers[beta].name != NULL; beta++)
          if(strcmp(headers[beta].name,str) == 0)
            break;

        /* the values following this header will be ignored */
        if(headers[beta].name == NULL)
          beta=-1;

        //print_debug("Saatiin headeri [%s] paikka %d\n",str,beta);
      }
      else if(lexout == CONF_LEX_VALUE)
      {
        register unsigned int delta;

        /* if this is preceded by an invalid header or identifier */
        if(beta == -1 || gamma == -1)
          break;

        /* check for duplicates */
        for(delta=0;ret[delta].header != -1;delta++)
          if(ret[delta].header == beta && ret[delta].ident == (unsigned)gamma)
            break;

        if(ret[delta].header == -1)
        {
          delta=curvalue;
          curvalue++;
        }
        else if(headers[beta].idents[gamma].type == CONF_STRING)
          nullify(ret[delta].value.string);

        //print_debug("Saatiin arvo [%s] ja taalla delta %d\n",str,delta);

        /* write the data */
        ret[delta].header = beta;
        ret[delta].ident  = (unsigned)gamma;        
        ret[delta].type = headers[beta].idents[gamma].type;

        switch(headers[beta].idents[gamma].type)
        {
        case CONF_STRING:
          ret[delta].value.string = strdup(str);
          break;
        case CONF_INTEGER:
          ret[delta].value.integer = strtol(str,NULL,10);
          break;
        case CONF_FLOAT:
          ret[delta].value.floatp = strtod(str,NULL);
          break;
        case CONF_INVALID:
        default:
          print_err("Error: identifier \"%s\" from header \"%s\" has invalid"
                      " identifier: %d",
            headers[beta].idents[gamma].name,headers[beta].name,
            headers[beta].idents[gamma].type);
          conf_free_output(ret);
          nullify(str);
          return NULL;
        }
      }
      break;

    /* identifier received */
    case 2: 
      gamma=-1;
      if(beta != -1)
      {
        /* search for the identifier */
        for(gamma=0;headers[beta].idents[gamma].name != NULL; gamma++)
          if(strcmp(headers[beta].idents[gamma].name,str) == 0)
            break;

        if(headers[beta].idents[gamma].name == NULL)
          gamma=-1;
      }
      //print_debug("Saatiin identifier [%s] paikka %d\n",str,gamma);
      break;

    /* equal-sign received */
    case 3: 
      //print_debug("Saatiin yhtäsuuruus.\n");
      break;

    /* syntax errors */
    case 0:
    case -1:
    default:
      print_err("Syntax error at line %d: \"%s\" not expected.\n",
        line,syms[lexout-1]);
      conf_free_output(ret);
      return NULL;
    }
  }

  nullify(str);

  /* lexer error */
  if(lexout == -2)
  {
    nullify(ret);
    return NULL;
  }

  //print_debug("idcount %d ja curvalue %d\n",idcount,curvalue);

  /* if there is too much memory allocated */
  if(idcount > curvalue)
  {
    conf_out_value *tmp;

    tmp=realloc(ret,sizeof(conf_out_value)*(curvalue+1));
    if(!tmp)
      conf_free_output(ret);

    tmp[curvalue].header=-1;

    ret=tmp;
  }
    
  return ret;
}

/* free the values */
tvalue conf_free_output(conf_out_value *out)
{
  register unsigned int beta;

  if(!out)
    return TRUE;

  /* nullify the string-values */
  for(beta=0;out[beta].header != -1;beta++)
    if(out[beta].type == CONF_STRING)
      nullify(out[beta].value.string);

  nullify(out);
  
  return TRUE;
}

/* prints the headers */
tvalue conf_print_output(iolet *file,conf_header *headers,
  conf_out_value *out)
{
  register unsigned int beta;
  int last=-2,type;

  ARG_ASSERT(!file || !headers,FALSE);

  for(beta=0;out[beta].header != -1;beta++)
  {
    type=headers[out[beta].header].idents[out[beta].ident].type;

    if(type == 0 || type > 3)
      continue;

    /* print the header */
    if(last != out[beta].header)
      iolet_out_format(file,"\n[%s]\n",headers[out[beta].header].name);

    iolet_out_format(file,"%s = \"",
      headers[out[beta].header].idents[out[beta].ident].name);

    if(type == CONF_STRING)
      iolet_out_format(file,"%s",out[beta].value.string);
    else if(type == CONF_INTEGER)
      iolet_out_format(file,"%d",out[beta].value.integer);
    else if(type == CONF_FLOAT)
      iolet_out_format(file,"%f",out[beta].value.floatp);

    iolet_out_format(file,"\"\n");

    last=out[beta].header;
  }

  return TRUE;
}


#ifdef CONF_TESTING

#include "file.h"

/* testing */

conf_header test[] =
{
  {"eka", (conf_identifier [])
   {
     {"arvotassa",CONF_STRING},
     {"tokaarvo" ,CONF_INTEGER},
     {"kolmas" ,CONF_FLOAT},
     {NULL,CONF_INVALID}
   }
  },
  {"hei", (conf_identifier [])
   {
     {"arvo", CONF_STRING},
     {NULL,CONF_INVALID}     
   }
  },
  {NULL,NULL}
};  

int main()
{
  iolet *file;
  conf_out_value *out;

  print_debug("Aloitetaan!\n");

  file=iolet_file_create("testimassa",IOLET_FILE_MODE_READ);

  out=conf_parse_iolet(test,file);
  print_debug("eka arvo on headerissa %d ident %d ja arvo %f\n",
    out[0].header,out[0].ident,out[0].value.floatp);

  conf_print_output(IL_IOStd,test,out);

  conf_free_output(out);
  //  conf_parse_sections(file,sects,scount);

  iolet_del(file);

  return 0;
}

#endif
