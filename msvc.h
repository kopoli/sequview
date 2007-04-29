/***************************************************************************
  Name:         msvc.h
  Description:  Compatibility header
  Created:      Mon Oct 24 13:14:52 EET 2003
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

/* 
  As can be assumed from the acronym below this is for MSVisualC++. And the
  version of the compiler this file "patches" does not comply with C99.
 */
#ifndef IL_MSVC_COMP
#define IL_MSVC_COMP

/* long long integers */
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

/* Normal integers */
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;

typedef int mode_t;

/* hmm.. */
#define inline __inline

#endif /* IL_MSVC_COMP */
