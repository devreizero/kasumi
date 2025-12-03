/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/*
 *	ISO C99 Standard: 7.21 String handling	<string.h>
 */

#ifndef	_STRING_H
#define	_STRING_H	1

/* Get size_t and NULL from <stddef.h>.  */
#define	__need_size_t
#define	__need_NULL
#include <stddef.h>

/* Tell the caller that we provide correct C++ prototypes.  */
#if defined __cplusplus
# define __CORRECT_ISO_CPP_STRING_H_PROTO
#endif


/* Copy SRC to DEST.  */
extern char *strcpy (char *__restrict __dest, const char *__restrict __src);
/* Copy no more than N characters of SRC to DEST.  */
extern char *strncpy (char *__restrict __dest,
		      const char *__restrict __src, size_t __n);

/* Append SRC onto DEST.  */
extern char *strcat (char *__restrict __dest, const char *__restrict __src);
/* Append no more than N characters from SRC onto DEST.  */
extern char *strncat (char *__restrict __dest, const char *__restrict __src,
		      size_t __n);

/* Compare S1 and S2.  */
extern int strcmp (const char *__s1, const char *__s2);
/* Compare N characters of S1 and S2.  */
extern int strncmp (const char *__s1, const char *__s2, size_t __n);

/* Compare the collated forms of S1 and S2.  */
extern int strcoll (const char *__s1, const char *__s2);
/* Put a transformation of SRC into no more than N bytes of DEST.  */
extern size_t strxfrm (char *__restrict __dest,
		       const char *__restrict __src, size_t __n);

#ifdef __USE_XOPEN2K8
/* POSIX.1-2008 extended locale interface (see locale.h).  */
# include <bits/types/locale_t.h>

/* Compare the collated forms of S1 and S2, using sorting rules from L.  */
extern int strcoll_l (const char *__s1, const char *__s2, locale_t __l) __nonnull ((1, 2, 3));
/* Put a transformation of SRC into no more than N bytes of DEST,
   using sorting rules from L.  */
extern size_t strxfrm_l (char *__dest, const char *__src, size_t __n,
			 locale_t __l);
#endif

#if (defined __USE_XOPEN_EXTENDED || defined __USE_XOPEN2K8)
/* Duplicate S, returning an identical malloc'd string.  */
extern char *strdup (const char *__s);
#endif

/* Return a malloc'd copy of at most N bytes of STRING.  The
   resultant string is terminated even if no null terminator
   appears before STRING[N].  */
#if defined __USE_XOPEN2K8
extern char *strndup (const char *__string, size_t __n);
#endif

#if defined __USE_GNU && defined __GNUC__
/* Duplicate S, returning an identical alloca'd string.  */
# define strdupa(s)							      \
  (__extension__							      \
    ({									      \
      const char *__old = (s);						      \
      size_t __len = strlen (__old) + 1;				      \
      char *__new = (char *) __builtin_alloca (__len);			      \
      (char *) memcpy (__new, __old, __len);				      \
    }))

/* Return an alloca'd copy of at most N bytes of string.  */
# define strndupa(s, n)							      \
  (__extension__							      \
    ({									      \
      const char *__old = (s);						      \
      size_t __len = strnlen (__old, (n));				      \
      char *__new = (char *) __builtin_alloca (__len + 1);		      \
      __new[__len] = '\0';						      \
      (char *) memcpy (__new, __old, __len);				      \
    }))
#endif

/* Find the first occurrence of C in S.  */
#ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
extern "C++"
{
extern char *strchr (char *__s, int __c)
     __asm ("strchr");
extern const char *strchr (const char *__s, int __c)
     __asm ("strchr");

# ifdef __OPTIMIZE__
extern char *
strchr (char *__s, int __c)
{
  return __builtin_strchr (__s, __c);
}

extern const char *
strchr (const char *__s, int __c)
{
  return __builtin_strchr (__s, __c);
}
# endif
}
#else
extern char *strchr (const char *__s, int __c);
#endif
/* Find the last occurrence of C in S.  */
#ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
extern "C++"
{
extern char *strrchr (char *__s, int __c)
     __asm ("strrchr");
extern const char *strrchr (const char *__s, int __c)
     __asm ("strrchr");

# ifdef __OPTIMIZE__
extern char *
strrchr (char *__s, int __c)
{
  return __builtin_strrchr (__s, __c);
}

extern const char *
strrchr (const char *__s, int __c)
{
  return __builtin_strrchr (__s, __c);
}
# endif
}
#else
extern char *strrchr (const char *__s, int __c);
#endif

#ifdef __USE_MISC
/* This function is similar to `strchr'.  But it returns a pointer to
   the closing NUL byte in case C is not found in S.  */
# ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
extern "C++" char *strchrnul (char *__s, int __c)
     __asm ("strchrnul");
extern "C++" const char *strchrnul (const char *__s, int __c)
     __asm ("strchrnul");
# else
extern char *strchrnul (const char *__s, int __c);
# endif
#endif

/* Return the length of the initial segment of S which
   consists entirely of characters not in REJECT.  */
extern size_t strcspn (const char *__s, const char *__reject);
/* Return the length of the initial segment of S which
   consists entirely of characters in ACCEPT.  */
extern size_t strspn (const char *__s, const char *__accept);
/* Find the first occurrence in S of any character in ACCEPT.  */
#ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
extern "C++"
{
extern char *strpbrk (char *__s, const char *__accept)
     __asm ("strpbrk");
extern const char *strpbrk (const char *__s, const char *__accept)
     __asm ("strpbrk");

# ifdef __OPTIMIZE__
extern char *
strpbrk (char *__s, const char *__accept)
{
  return __builtin_strpbrk (__s, __accept);
}

extern const char *
strpbrk (const char *__s, const char *__accept)
{
  return __builtin_strpbrk (__s, __accept);
}
# endif
}
#else
extern char *strpbrk (const char *__s, const char *__accept);
#endif
/* Find the first occurrence of NEEDLE in HAYSTACK.  */
#ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
extern "C++"
{
extern char *strstr (char *__haystack, const char *__needle)
     __asm ("strstr");
extern const char *strstr (const char *__haystack, const char *__needle)
     __asm ("strstr");

# ifdef __OPTIMIZE__
extern char *
strstr (char *__haystack, const char *__needle)
{
  return __builtin_strstr (__haystack, __needle);
}

extern const char *
strstr (const char *__haystack, const char *__needle)
{
  return __builtin_strstr (__haystack, __needle);
}
# endif
}
#else
extern char *strstr (const char *__haystack, const char *__needle);
#endif


/* Divide S into tokens separated by characters in DELIM.  */
extern char *strtok (char *__restrict __s, const char *__restrict __delim)
;

/* Divide S into tokens separated by characters in DELIM.  Information
   passed between calls are stored in SAVE_PTR.  */
extern char *__strtok_r (char *__restrict __s,
			 const char *__restrict __delim,
			 char **__restrict __save_ptr);
#ifdef __USE_POSIX
extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
		       char **__restrict __save_ptr);
#endif

#ifdef __USE_MISC
/* Similar to `strstr' but this function ignores the case of both strings.  */
# ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
extern "C++" char *strcasestr (char *__haystack, const char *__needle)
     __asm ("strcasestr");
extern "C++" const char *strcasestr (const char *__haystack,
				     const char *__needle)
     __asm ("strcasestr");
# else
extern char *strcasestr (const char *__haystack, const char *__needle);
# endif
#endif


/* Return the length of S.  */
extern size_t strlen (const char *__s);

#ifdef	__USE_XOPEN2K8
/* Find the length of STRING, but scan at most MAXLEN characters.
   If no '\0' terminator is found in that many characters, return MAXLEN.  */
extern size_t strnlen (const char *__string, size_t __maxlen);
#endif


/* Return a string describing the meaning of the `errno' code in ERRNUM.  */
extern char *strerror (int __errnum);
#ifdef __USE_XOPEN2K
/* Reentrant version of `strerror'.
   There are 2 flavors of `strerror_r', GNU which returns the string
   and may or may not use the supplied temporary buffer and POSIX one
   which fills the string into the buffer.
   To use the POSIX version, -D_XOPEN_SOURCE=600 or -D_POSIX_C_SOURCE=200112L
   without -D_GNU_SOURCE is needed, otherwise the GNU version is
   preferred.  */
# if defined __USE_XOPEN2K && !defined __USE_GNU
/* Fill BUF with a string describing the meaning of the `errno' code in
   ERRNUM.  */
#  ifdef __REDIRECT_NTH
extern int __REDIRECT_NTH (strerror_r,
			   (int __errnum, char *__buf, size_t __buflen),
			   __xpg_strerror_r);
#  else
extern int __xpg_strerror_r (int __errnum, char *__buf, size_t __buflen);
#   define strerror_r __xpg_strerror_r
#  endif
# else
/* If a temporary buffer is required, at most BUFLEN bytes of BUF will be
   used.  */
extern char *strerror_r (int __errnum, char *__buf, size_t __buflen)
 __wur;
# endif

# ifdef __USE_GNU
/* Return a string describing the meaning of tthe error in ERR.  */
extern const char *strerrordesc_np (int __err);
/* Return a string with the error name in ERR.  */
extern const char *strerrorname_np (int __err);
# endif
#endif

#ifdef __USE_XOPEN2K8
/* Translate error number to string according to the locale L.  */
extern char *strerror_l (int __errnum, locale_t __l);
#endif

#ifdef __USE_MISC
# include <strings.h>

/* Set N bytes of S to 0.  The compiler will not delete a call to this
   function, even if S is dead after the call.  */
extern void explicit_bzero (void *__s, size_t __n)
    __fortified_attr_access (__write_only__, 1, 2);

/* Return the next DELIM-delimited token from *STRINGP,
   terminating it with a '\0', and update *STRINGP to point past it.  */
extern char *strsep (char **__restrict __stringp,
		     const char *__restrict __delim);
#endif

#ifdef	__USE_XOPEN2K8
/* Return a string describing the meaning of the signal number in SIG.  */
extern char *strsignal (int __sig);

# ifdef __USE_GNU
/* Return an abbreviation string for the signal number SIG.  */
extern const char *sigabbrev_np (int __sig);
/* Return a string describing the meaning of the signal number in SIG,
   the result is not translated.  */
extern const char *sigdescr_np (int __sig);
# endif

/* Copy SRC to DEST, returning the address of the terminating '\0' in DEST.  */
extern char *__stpcpy (char *__restrict __dest, const char *__restrict __src);
extern char *stpcpy (char *__restrict __dest, const char *__restrict __src);

/* Copy no more than N characters of SRC to DEST, returning the address of
   the last character written into DEST.  */
extern char *__stpncpy (char *__restrict __dest,
			const char *__restrict __src, size_t __n);
extern char *stpncpy (char *__restrict __dest,
		      const char *__restrict __src, size_t __n);
#endif

#ifdef __USE_MISC
/* Copy at most N - 1 characters from SRC to DEST.  */
extern size_t strlcpy (char *__restrict __dest,
		       const char *__restrict __src, size_t __n);

/* Append SRC to DEST, possibly with truncation to keep the total size
   below N.  */
extern size_t strlcat (char *__restrict __dest,
		       const char *__restrict __src, size_t __n);
#endif

#ifdef	__USE_GNU
/* Compare S1 and S2 as strings holding name & indices/version numbers.  */
extern int strverscmp (const char *__s1, const char *__s2);

/* Sautee STRING briskly.  */
extern char *strfry (char *__string);

# ifndef basename
/* Return the file name within directory of FILENAME.  We don't
   declare the function if the `basename' macro is available (defined
   in <libgen.h>) which makes the XPG version of this function
   available.  */
#  ifdef __CORRECT_ISO_CPP_STRING_H_PROTO
extern "C++" char *basename (char *__filename)
     __asm ("basename");
extern "C++" const char *basename (const char *__filename)
     __asm ("basename");
#  else
extern char *basename (const char *__filename);
#  endif
# endif
#endif

# if __USE_FORTIFY_LEVEL > 0 && defined __fortify_function
/* Functions with security checks.  */
#  include <bits/string_fortified.h>
# endif

#endif /* string.h  */
