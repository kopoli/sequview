/***************************************************************************
  Name:         conf.h
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


#ifndef COMMON_CONF_HEADER
#define COMMON_CONF_HEADER

/****************************************************************************
  Defines / Structures / data types
 ****************************************************************************/

/* identifier types */
#define CONF_INVALID 0
#define CONF_STRING  1
#define CONF_INTEGER 2
#define CONF_FLOAT   3

/* input structures */
typedef struct 
{
  char *name;
  unsigned char type;
} conf_identifier;

typedef struct
{
  char *name;
  conf_identifier *idents;
} conf_header;

/* possible values */
typedef union
{
  char     *string;
  long int  integer;
  double    floatp;
} conf_out_data;

/* output structure */
typedef struct
{
  /* the positions in conf_header and respective conf_identifier */
  int header;
  unsigned int ident;

  /* type just copied from conf_identifier, needed by conf_free_output */
  unsigned char type;

  /* the value */
  conf_out_data value;

} conf_out_value;


/****************************************************************************
  Prototypes
 ****************************************************************************/

conf_out_value *conf_parse_iolet(conf_header *headers, iolet *file);
tvalue conf_free_output(conf_out_value *out);
tvalue conf_print_output(iolet *file,conf_header *headers,
  conf_out_value *out);

#endif
