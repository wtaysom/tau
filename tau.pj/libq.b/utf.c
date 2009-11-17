/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/


/*
 * UTF-8 is a encoding of Unicode developed by Bell Labs
 * and adopted as a standard.  It has numerous advantages
 * over regular unicode for serial communication.  The best
 * is that C ASCII strings have the same representation.
 *
 * 0xxxxxxx
 * 110xxxxx 10xxxxxx
 * 1110xxxx 10xxxxxx 10xxxxxx
 *
 * Information taken from "Java in a Nutshell":
 * 1. All ASCII characters are one-byte UTF-8 characters.  A legal
 * ASCII string is a legal UTF-8 string.
 * 2. Any non-ASCII character (i.e., any character with the high-
 * order bit set) is part of a multibyte character.
 * 3. The first byte of any UTF-8 character indicates the number
 * of additional bytes in the character.
 * 4. The first byte of a multibyte character is easily distinguished
 * from the subsequent bytes.  Thus, it is easy to locate the start
 * of a character from an arbitrary position in a data stream.
 * 5. It is easy to convert between UTF-8 and Unicode.
 * 6. The UTF-8 encoding is relatively compact.  For text with a large
 * percentage of ASCII characters, is it more compact than Unicode.  In
 * the worst case, a UTF-8 string is only 50% larger than the corresponding
 * Unicode string.
 */

#include <style.h>
#include <utf.h>


unint uni2utf (	/* Return -1 = err, else # utf bytes in dest. */
	const unicode_t	*unicode, /* Source */
	utf8_t		*utf, 	/* Destination */
	unint		bufLen)	/* Length in bytes */
{
	unicode_t	u;
	utf8_t		*end;

	if (!unicode || !utf || (bufLen < 1))
	{
		return -1;
	}

	end = utf + bufLen - 1;
	for (u = *unicode; u != 0; u = *++unicode)
	{
		if (u < 0x80)
		{
			if (utf >= end)
			{
				*utf = 0;
				return -1;
			}
			*utf++ = u;
		}
		else if (u < 0x800)
		{
			if (utf + 1 >= end)
			{
				*utf = 0;
				return -1;
			}
			*utf++ = (0xc0 | (u >> 6));
			*utf++ = (0x80 | (u & 0x3f));
		}
		else
		{
			if (utf + 2 >= end)
			{
				*utf = 0;
				return -1;
			}
			*utf++ = (0xe0 | (u >> 12));
			*utf++ = (0x80 | ((u >> 6) & 0x3f));
			*utf++ = (0x80 | (u & 0x3f));
		}
	}
	*utf = 0;
	return bufLen - 1 - (end - utf);
}

unint utf2uni (	/* Return: -1 = err, else # uni chars in dest. */
	utf8_t		*utf,		/* Source */
	unicode_t	*unicode, 	/* Destination */
	unint		bufLen)		/* Length in bytes */
{
	unsigned int	c;
	unicode_t	u;
	unicode_t	*uni;
	unicode_t	*end;

	if (!utf || !unicode || (bufLen < sizeof(unicode_t)))
	{
		goto vBad;
	}

	end = (unicode_t *)((addr)unicode) + ((bufLen / sizeof(unicode_t)) - 1); /* subtract one to allow room for a null */

	uni = unicode;
	for (;;)
	{
		c = *utf++;
		if (c == 0)
		{
			break;
		}

		if (uni >= end)
		{
			goto bad;
		}

		if (c < 0x80)
		{				/* Regular ASCII */
			*uni = c;
		}
		else if (c < 0xc0)
		{
			goto bad;
		}
		else if (c < 0xe0)
		{				/* 2 Byte c */
			u = (c & 0x1f) << 6;
			c = *utf++;
			if (c == 0)
			{
				goto bad;
			}
			if ((c < 0x80) || (c >= 0xc0))
			{
				goto bad;
			}
			*uni = (u | (c & 0x3f));
		}
		else if (c < 0xf0)
		{
			u = (c & 0xf) << 12;
			c = *utf++;
			if (c == 0)
			{
				goto bad;
			}
			if ((c < 0x80) || (c >= 0xc0))
			{
				goto bad;
			}
			u |= (c & 0x3f) << 6;
			c = *utf++;
			if ((c < 0x80) || (c >= 0xc0))
			{
				goto bad;
			}
			*uni = (u | (c & 0x3f));
		}
		else
		{
			goto bad;
		}
		uni++;
	}
	*uni = 0;
	return uni - unicode;
bad:
	*uni = 0;
vBad:
	return -1;
}

