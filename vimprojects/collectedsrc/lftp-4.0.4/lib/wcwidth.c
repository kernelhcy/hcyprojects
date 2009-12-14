/* Determine the number of screen columns needed for a character.
   Copyright (C) 2006, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <wchar.h>

/* Get iswprint.  */
#include <wctype.h>

#if HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif

#include "localcharset.h"
#include "streq.h"
#include "uniwidth.h"

#undef wcwidth

#include "uniwidth/cjk.h"

static char cached_encoding[32];
static int cached_is_cjk_encoding;
static int cached_is_utf8_encoding;

static const char *locale_charset_simple ()
{
#if HAVE_LANGINFO_CODESET
  /* Most systems support nl_langinfo (CODESET) nowadays.  */
  return nl_langinfo (CODESET);
# else
  /* Do the complex case */
  return locale_charset ();
# endif
}

static void cache_encoding ()
{
  const char *encoding = locale_charset_simple ();
  if (!strncmp(encoding, cached_encoding, sizeof (cached_encoding)))
    return;
  strncpy (cached_encoding, encoding, sizeof (cached_encoding));
  encoding = locale_charset ();
  cached_is_utf8_encoding = STREQ (encoding, "UTF-8", 'U', 'T', 'F', '-', '8', 0, 0, 0 ,0);
  cached_is_cjk_encoding = is_cjk_encoding (encoding);
}

int
rpl_wcwidth (wchar_t wc)
{
  cache_encoding ();
  /* In UTF-8 locales, use a Unicode aware width function.  */
  if (cached_is_utf8_encoding || cached_is_cjk_encoding)
    {
      /* We assume that in a UTF-8 locale, a wide character is the same as a
	 Unicode character.  */
      return uc_width (wc, cached_is_cjk_encoding);
    }
  else
    {
      /* Otherwise, fall back to the system's wcwidth function.  */
#if HAVE_WCWIDTH
      return wcwidth (wc);
#else
      return wc == 0 ? 0 : iswprint (wc) ? 1 : -1;
#endif
    }
}
