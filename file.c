/***************************************************************************
  Name:         file.c
  Description:  filesystem functionality of Infoleech
  Created:      Tue Feb  25 18:20:24 EET 2003
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

#include "commonconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>

#include "iolet.h"
#include "file.h"

/* 
  Converts iolet_file_mode to fopen modes. Assumes that the buffer is at 
  least 3 bytes. Assumes that the arguments are valid.
 */
static char *ILFM_to_fopen(iolet_file_mode moodi, char *buffer)
{
  moodi &= 0x0000FFFF;

  /* determine the mode */
  switch (moodi)
  {
  case IOLET_FILE_MODE_READ:
    strcpy(buffer, "r");
    break;
  case IOLET_FILE_MODE_READ | IOLET_FILE_MODE_WRITE:
    strcpy(buffer, "r+");
    break;
  case IOLET_FILE_MODE_WRITE | IOLET_FILE_MODE_TRUNCATE:
    strcpy(buffer, "w");
    break;
  case IOLET_FILE_MODE_READ | IOLET_FILE_MODE_WRITE | IOLET_FILE_MODE_TRUNCATE | IOLET_FILE_MODE_CREATE:
    strcpy(buffer, "w+");
    break;
  case IOLET_FILE_MODE_WRITE | IOLET_FILE_MODE_APPEND | IOLET_FILE_MODE_CREATE:
    strcpy(buffer, "a");
    break;
  case IOLET_FILE_MODE_READ | IOLET_FILE_MODE_WRITE | IOLET_FILE_MODE_APPEND | IOLET_FILE_MODE_CREATE:
    strcpy(buffer, "a+");
    break;

    /* This is not wise, most likely. */
  default:
    strcpy(buffer, "0");
    break;
  }

  return buffer;
}

/* Converts fopen's mode to iolet_file_mode */
iolet_file_mode iolet_file_mode_convert(const char *fopenmode)
{
  char khar;
  iolet_file_mode mode = 0;
  ARG_ASSERT(!fopenmode, 0);

  /* Parse the first two characters. */
  khar = fopenmode[0];

  /* Conversion */
  switch (khar)
  {
  case 'r':
    mode = IOLET_FILE_MODE_READ | ((fopenmode[1] == '+') ? IOLET_FILE_MODE_WRITE : 0);
    break;
  case 'w':
    mode =
     IOLET_FILE_MODE_WRITE | IOLET_FILE_MODE_TRUNCATE | ((fopenmode[1] ==
      '+') ? IOLET_FILE_MODE_READ | IOLET_FILE_MODE_CREATE : 0);
    break;
  case 'a':
    mode =
     IOLET_FILE_MODE_WRITE | IOLET_FILE_MODE_APPEND | IOLET_FILE_MODE_CREATE | ((fopenmode[1] ==
      '+') ? IOLET_FILE_MODE_READ : 0);
    break;

    /* Unacceptable */
  default:
    return 0;
  }

  return mode;
}


/* These functions assume that the IL_OutputInit has been executed */
static tvalue FileOut_Output_format(iolet * out, const char *format,
 va_list list)
{
  int returni;
  ARG_ASSERT(!out || !out->PrivData || !format, FALSE);

  returni = vfprintf(out->PrivData, format, list);

  if(returni < 0)
  {
    iolet_out_format((iolet *) IL_DefOutErr,
     "Error: could not write the data: %s\n", strerror(errno));
    return FALSE;
  }

  /* Flush the data as well */
  if(fflush(out->PrivData) == EOF)
  {
    /* Use the default because this might be the IL_StdErr */
    iolet_out_format((iolet *) IL_DefOutErr,
     "Error: could not flush the data: %s\n", strerror(errno));
    return FALSE;
  }

  return TRUE;
}

static tvalue FileOut_Output_stream(iolet * out, const char *data,
 size_t length)
{
  size_t written;
  ARG_ASSERT(!out || !out->PrivData || !data, FALSE);

  written = fwrite(data, length, 1, out->PrivData);

  if(written != 1)
  {
    iolet_out_format((iolet *) IL_DefOutErr,
     "Error: could not write the data: %s\n", strerror(errno));
    return FALSE;
  }

  /* Flush the data as well */
  if(fflush(out->PrivData) == EOF)
  {
    /* Use the default because this might be the IL_StdErr */
    iolet_out_format((iolet *) IL_DefOutErr,
     "Error: could not flush the " "data: %s\n", strerror(errno));
    return FALSE;
  }

  return TRUE;
}

static tvalue FileOut_End(iolet * out)
{
  ARG_ASSERT(!out || !out->PrivData, FALSE);

  if(fclose(out->PrivData) == EOF)
    print_err("Error: Could not close file handle: %s\n", strerror(errno));

  nullify(out);

  return TRUE;
}

/*
  File Input
*/

static int FileIn_InputChar(struct iolet *in)
{
  int khar;
  khar = fgetc(in->PrivData);

  if(khar == EOF)
    return -1;

  return khar;
}

static tvalue FileIn_InputLine(struct iolet *in, char *line, size_t length)
{
  if(!line)
    return FALSE;

  if(fgets(line, length, in->PrivData) == NULL)
    return FALSE;

  return TRUE;
}

static tvalue FileIn_InputStream(struct iolet *in, char *data, size_t length)
{
  size_t check = 0;

  if(!data)
    return FALSE;

  /* hmm.. is this wasteful? */
  check = fread(data, 1, length, in->PrivData);
  if(check != length)
    return FALSE;

  return TRUE;
}

static tvalue FileIn_IsEof(struct iolet *in)
{
  if(feof(in->PrivData) != 0)
    return TRUE;

  return FALSE;
}

static iolet *fileiolet_common_characteristics(FILE *fp)
{
  iolet *out;
  
  out = (iolet *) malloc(sizeof(iolet));
  if(!out)
  {
    print_err("Error: Could not allocate %d bytes!\n", sizeof(iolet));
    fclose(fp);
    return FALSE;
  }

  /* Set the opened FILE struct as the private data */
  out->PDataLen = sizeof(FILE *);
  out->PrivData = fp;

  /* output */
  out->OutputFormat = FileOut_Output_format;
  out->OutputStream = FileOut_Output_stream;

  /* input */
  out->InputChar = (*FileIn_InputChar);
  out->InputLine = (*FileIn_InputLine);
  out->InputStream = (*FileIn_InputStream);
  out->IsEof = (*FileIn_IsEof);

  out->Flags = IL_INLET_CHAR | IL_INLET_LINE | IL_INLET_STREAM | 
    IL_OUTLET_FORMAT | IL_OUTLET_STREAM;

  out->Type = IL_IOLET_FILE;
  out->End = FileOut_End;

  iolet_add(out);

  return out;
}

/* Creates a fileIOlet */
iolet *iolet_file_create(const char *Path, iolet_file_mode Mode)
{
  char buffer[4];
  FILE *fp;

  ARG_ASSERT(!iolet_init() || !Path || !Mode, NULL);

  /* Parse the mode */
  ILFM_to_fopen(Mode, buffer);
  if(buffer[0] == '0')
  {
    print_err("Error: The mode provided is invalid\n");
    return NULL;
  }

  if((fp = fopen(Path, buffer)) == NULL)
  {
    print_err("Error: Could not open file \"%s\": %s\n", Path,
     strerror(errno));
    return NULL;
  }

  return fileiolet_common_characteristics(fp);
}

iolet *iolet_file_create_fd(int filedesc, iolet_file_mode Mode)
{
  char buffer[4];
  FILE *fp;

  ARG_ASSERT(!iolet_init() || !Mode, NULL);

  /* Parse the mode */
  ILFM_to_fopen(Mode, buffer);
  if(buffer[0] == '0')
  {
    print_err("Error: The mode provided is invalid\n");
    return NULL;
  }

  if((fp=fdopen(filedesc,buffer)) == NULL)
  {
    print_err("Error: Could not open filedescriptor \"%d\": %s\n", filedesc,
     strerror(errno));
    return NULL;
  }

  return fileiolet_common_characteristics(fp);
}

//#define FILE_DEBUG
#ifdef FILE_DEBUG

int main()
{
  char muumi[4];
  iolet_file_mode kuppa = iolet_file_mode_convert("w+");
  iolet *outti;

  iolet_init();

  ILFM_to_fopen(kuppa, muumi);

  print_out("muumi on %d [%s]\n", kuppa, muumi);
  print_out("ja tämä on on [%d]\n", iolet_file_mode_convert(ILFM_to_fopen(kuppa,
     muumi)));


  outti = iolet_file_create("muumitiedosto.txt", iolet_file_mode_convert("w"));
  iolet_out_format(outti, "kirjoitetaan nyt jotain tännekin\n");

  // Bind a logfile into the IL_OutErr (not really)
  IL_OutErr = iolet_fork_output(IL_OutErr, outti);
  print_err("tämän pitäisi mennä myös tiedostoon %d\n", sizeof(FILE));

  return 0;
}

#endif
