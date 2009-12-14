/* Display width functions.
   Copyright (C) 2001-2002, 2005, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _UNIWIDTH_H
#define _UNIWIDTH_H

#include "unitypes.h"

/* Get size_t.  */
#include <stddef.h>

/* Get locale_charset() declaration.  */
#include "localcharset.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Display width.  */

/* These functions are locale dependent.  The encoding argument identifies
   the encoding (e.g. "ISO-8859-2" for Polish).  */

/* Determine number of column positions required for UC.  */
extern int
       uc_width (ucs4_t uc, int is_cjk_encoding);

#ifdef __cplusplus
}
#endif

#endif /* _UNIWIDTH_H */
