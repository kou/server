/*****************************************************************************

Copyright (c) 1995, 2015, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2017, 2020, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/******************************************************************//**
@file include/mach0data.ic
Utilities for converting data from the database file
to the machine format.

Created 11/28/1995 Heikki Tuuri
***********************************************************************/

#ifndef UNIV_INNOCHECKSUM

#include "mtr0types.h"

/*******************************************************//**
The following function is used to store data in one byte. */
UNIV_INLINE
void
mach_write_to_1(
/*============*/
	byte*	b,	/*!< in: pointer to byte where to store */
	ulint	n)	/*!< in: ulint integer to be stored, >= 0, < 256 */
{
#ifndef HAVE_valgrind
	ut_ad((n & ~0xFFUL) == 0);
#endif

	b[0] = (byte) n;
}

#endif /* !UNIV_INNOCHECKSUM */

/*******************************************************//**
The following function is used to store data in two consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_2(
/*============*/
	byte*	b,	/*!< in: pointer to two bytes where to store */
	ulint	n)	/*!< in: ulint integer to be stored */
{
#ifndef HAVE_valgrind
	ut_ad((n & ~0xFFFFUL) == 0);
#endif

	b[0] = (byte)(n >> 8);
	b[1] = (byte)(n);
}

/** The following function is used to fetch data from one byte.
@param[in]	b	pointer to a byte to read
@return ulint integer, >= 0, < 256 */
UNIV_INLINE
uint8_t
mach_read_from_1(
	const byte*	b)
{
	return(uint8_t(*b));
}

/** The following function is used to fetch data from 2 consecutive
bytes. The most significant byte is at the lowest address.
@param[in]	b	pointer to 2 bytes to read
@return 2-byte integer, >= 0, < 64k */
UNIV_INLINE
uint16_t
mach_read_from_2(
	const byte*	b)
{
	return(uint16_t(uint16_t(b[0]) << 8 | b[1]));
}

#ifndef UNIV_INNOCHECKSUM

/********************************************************//**
The following function is used to convert a 16-bit data item
to the canonical format, for fast bytewise equality test
against memory.
@return 16-bit integer in canonical format */
UNIV_INLINE
uint16
mach_encode_2(
/*==========*/
	ulint	n)	/*!< in: integer in machine-dependent format */
{
	uint16	ret;
	ut_ad(2 == sizeof ret);
	mach_write_to_2((byte*) &ret, n);
	return(ret);
}
/********************************************************//**
The following function is used to convert a 16-bit data item
from the canonical format, for fast bytewise equality test
against memory.
@return integer in machine-dependent format */
UNIV_INLINE
ulint
mach_decode_2(
/*==========*/
	uint16	n)	/*!< in: 16-bit integer in canonical format */
{
	ut_ad(2 == sizeof n);
	return(mach_read_from_2((const byte*) &n));
}

/*******************************************************//**
The following function is used to store data in 3 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_3(
/*============*/
	byte*	b,	/*!< in: pointer to 3 bytes where to store */
	ulint	n)	/*!< in: ulint integer to be stored */
{
	ut_ad((n & ~0xFFFFFFUL) == 0);

	b[0] = (byte)(n >> 16);
	b[1] = (byte)(n >> 8);
	b[2] = (byte)(n);
}

/** The following function is used to fetch data from 3 consecutive
bytes. The most significant byte is at the lowest address.
@param[in]	b	pointer to 3 bytes to read
@return uint32_t integer */
UNIV_INLINE
uint32_t
mach_read_from_3(
	const byte*	b)
{
	return( (static_cast<uint32_t>(b[0]) << 16)
		| (static_cast<uint32_t>(b[1]) << 8)
		| static_cast<uint32_t>(b[2])
		);
}
#endif /* !UNIV_INNOCHECKSUM */

/*******************************************************//**
The following function is used to store data in four consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_4(
/*============*/
	byte*	b,	/*!< in: pointer to four bytes where to store */
	ulint	n)	/*!< in: ulint integer to be stored */
{
	b[0] = (byte)(n >> 24);
	b[1] = (byte)(n >> 16);
	b[2] = (byte)(n >> 8);
	b[3] = (byte) n;
}

/** The following function is used to fetch data from 4 consecutive
bytes. The most significant byte is at the lowest address.
@param[in]	b	pointer to 4 bytes to read
@return 32 bit integer */
UNIV_INLINE
uint32_t
mach_read_from_4(
	const byte*	b)
{
	return( (static_cast<uint32_t>(b[0]) << 24)
		| (static_cast<uint32_t>(b[1]) << 16)
		| (static_cast<uint32_t>(b[2]) << 8)
		| static_cast<uint32_t>(b[3])
		);
}

#ifndef UNIV_INNOCHECKSUM

/*********************************************************//**
Writes a ulint in a compressed form where the first byte codes the
length of the stored ulint. We look at the most significant bits of
the byte. If the most significant bit is zero, it means 1-byte storage,
else if the 2nd bit is 0, it means 2-byte storage, else if 3rd is 0,
it means 3-byte storage, else if 4th is 0, it means 4-byte storage,
else the storage is 5-byte.
@return compressed size in bytes */
UNIV_INLINE
ulint
mach_write_compressed(
/*==================*/
	byte*	b,	/*!< in: pointer to memory where to store */
	ulint	n)	/*!< in: ulint integer (< 2^32) to be stored */
{
	if (n < 0x80) {
		/* 0nnnnnnn (7 bits) */
		mach_write_to_1(b, n);
		return(1);
	} else if (n < 0x4000) {
		/* 10nnnnnn nnnnnnnn (14 bits) */
		mach_write_to_2(b, n | 0x8000);
		return(2);
	} else if (n < 0x200000) {
		/* 110nnnnn nnnnnnnn nnnnnnnn (21 bits) */
		mach_write_to_3(b, n | 0xC00000);
		return(3);
	} else if (n < 0x10000000) {
		/* 1110nnnn nnnnnnnn nnnnnnnn nnnnnnnn (28 bits) */
		mach_write_to_4(b, n | 0xE0000000);
		return(4);
	} else {
		/* 11110000 nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn (32 bits) */
		mach_write_to_1(b, 0xF0);
		mach_write_to_4(b + 1, n);
		return(5);
	}
}

/*********************************************************//**
Returns the size of a ulint when written in the compressed form.
@return compressed size in bytes */
UNIV_INLINE
ulint
mach_get_compressed_size(
/*=====================*/
	ulint	n)	/*!< in: ulint integer (< 2^32) to be stored */
{
	if (n < 0x80) {
		/* 0nnnnnnn (7 bits) */
		return(1);
	} else if (n < 0x4000) {
		/* 10nnnnnn nnnnnnnn (14 bits) */
		return(2);
	} else if (n < 0x200000) {
		/* 110nnnnn nnnnnnnn nnnnnnnn (21 bits) */
		return(3);
	} else if (n < 0x10000000) {
		/* 1110nnnn nnnnnnnn nnnnnnnn nnnnnnnn (28 bits) */
		return(4);
	} else {
		/* 11110000 nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn (32 bits) */
		return(5);
	}
}

/*********************************************************//**
Reads a ulint in a compressed form.
@return read integer (< 2^32) */
UNIV_INLINE
ulint
mach_read_compressed(
/*=================*/
	const byte*	b)	/*!< in: pointer to memory from where to read */
{
	ulint	val;

	val = mach_read_from_1(b);

	if (val < 0x80) {
		/* 0nnnnnnn (7 bits) */
	} else if (val < 0xC0) {
		/* 10nnnnnn nnnnnnnn (14 bits) */
		val = mach_read_from_2(b) & 0x3FFF;
		ut_ad(val > 0x7F);
	} else if (val < 0xE0) {
		/* 110nnnnn nnnnnnnn nnnnnnnn (21 bits) */
		val = mach_read_from_3(b) & 0x1FFFFF;
		ut_ad(val > 0x3FFF);
	} else if (val < 0xF0) {
		/* 1110nnnn nnnnnnnn nnnnnnnn nnnnnnnn (28 bits) */
		val = mach_read_from_4(b) & 0xFFFFFFF;
		ut_ad(val > 0x1FFFFF);
	} else {
		/* 11110000 nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn (32 bits) */
		ut_ad(val == 0xF0);
		val = mach_read_from_4(b + 1);
		ut_ad(val > 0xFFFFFFF);
	}

	return(val);
}

/** Read a 32-bit integer in a compressed form.
@param[in,out]	b	pointer to memory where to read;
advanced by the number of bytes consumed
@return unsigned value */
UNIV_INLINE
ib_uint32_t
mach_read_next_compressed(
	const byte**	b)
{
	ulint	val = mach_read_from_1(*b);

	if (val < 0x80) {
		/* 0nnnnnnn (7 bits) */
		++*b;
	} else if (val < 0xC0) {
		/* 10nnnnnn nnnnnnnn (14 bits) */
		val = mach_read_from_2(*b) & 0x3FFF;
		ut_ad(val > 0x7F);
		*b += 2;
	} else if (val < 0xE0) {
		/* 110nnnnn nnnnnnnn nnnnnnnn (21 bits) */
		val = mach_read_from_3(*b) & 0x1FFFFF;
		ut_ad(val > 0x3FFF);
		*b += 3;
	} else if (val < 0xF0) {
		/* 1110nnnn nnnnnnnn nnnnnnnn nnnnnnnn (28 bits) */
		val = mach_read_from_4(*b) & 0xFFFFFFF;
		ut_ad(val > 0x1FFFFF);
		*b += 4;
	} else {
		/* 11110000 nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn (32 bits) */
		ut_ad(val == 0xF0);
		val = mach_read_from_4(*b + 1);
		ut_ad(val > 0xFFFFFFF);
		*b += 5;
	}

	return(static_cast<ib_uint32_t>(val));
}

/*******************************************************//**
The following function is used to store data in 8 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_8(
/*============*/
	void*		b,	/*!< in: pointer to 8 bytes where to store */
	ib_uint64_t	n)	/*!< in: 64-bit integer to be stored */
{
	mach_write_to_4(static_cast<byte*>(b), (ulint) (n >> 32));
	mach_write_to_4(static_cast<byte*>(b) + 4, (ulint) n);
}

#endif /* !UNIV_INNOCHECKSUM */

/********************************************************//**
The following function is used to fetch data from 8 consecutive
bytes. The most significant byte is at the lowest address.
@return 64-bit integer */
UNIV_INLINE
ib_uint64_t
mach_read_from_8(
/*=============*/
	const byte*	b)	/*!< in: pointer to 8 bytes */
{
	ib_uint64_t	u64;

	u64 = mach_read_from_4(b);
	u64 <<= 32;
	u64 |= mach_read_from_4(b + 4);

	return(u64);
}

#ifndef UNIV_INNOCHECKSUM

/*******************************************************//**
The following function is used to store data in 7 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_7(
/*============*/
	byte*		b,	/*!< in: pointer to 7 bytes where to store */
	ib_uint64_t	n)	/*!< in: 56-bit integer */
{
	mach_write_to_3(b, (ulint) (n >> 32));
	mach_write_to_4(b + 3, (ulint) n);
}

/********************************************************//**
The following function is used to fetch data from 7 consecutive
bytes. The most significant byte is at the lowest address.
@return 56-bit integer */
UNIV_INLINE
ib_uint64_t
mach_read_from_7(
/*=============*/
	const byte*	b)	/*!< in: pointer to 7 bytes */
{
	return(ut_ull_create(mach_read_from_3(b), mach_read_from_4(b + 3)));
}

/*******************************************************//**
The following function is used to store data in 6 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_6(
/*============*/
	byte*		b,	/*!< in: pointer to 6 bytes where to store */
	ib_uint64_t	n)	/*!< in: 48-bit integer */
{
	mach_write_to_2(b, (ulint) (n >> 32));
	mach_write_to_4(b + 2, (ulint) n);
}

/********************************************************//**
The following function is used to fetch data from 6 consecutive
bytes. The most significant byte is at the lowest address.
@return 48-bit integer */
UNIV_INLINE
ib_uint64_t
mach_read_from_6(
/*=============*/
	const byte*	b)	/*!< in: pointer to 6 bytes */
{
	return(ut_ull_create(mach_read_from_2(b), mach_read_from_4(b + 2)));
}

/*********************************************************//**
Writes a 64-bit integer in a compressed form (5..9 bytes).
@return size in bytes */
UNIV_INLINE
ulint
mach_u64_write_compressed(
/*======================*/
	byte*		b,	/*!< in: pointer to memory where to store */
	ib_uint64_t	n)	/*!< in: 64-bit integer to be stored */
{
	ulint	size = mach_write_compressed(b, (ulint) (n >> 32));
	mach_write_to_4(b + size, (ulint) n);

	return(size + 4);
}

/** Read a 64-bit integer in a compressed form.
@param[in,out]	b	pointer to memory where to read;
advanced by the number of bytes consumed
@return unsigned value */
UNIV_INLINE
ib_uint64_t
mach_u64_read_next_compressed(
	const byte**	b)
{
	ib_uint64_t	val;

	val = mach_read_next_compressed(b);
	val <<= 32;
	val |= mach_read_from_4(*b);
	*b += 4;
	return(val);
}

/*********************************************************//**
Writes a 64-bit integer in a compressed form (1..11 bytes).
@return size in bytes */
UNIV_INLINE
ulint
mach_u64_write_much_compressed(
/*===========================*/
	byte*		b,	/*!< in: pointer to memory where to store */
	ib_uint64_t	n)	/*!< in: 64-bit integer to be stored */
{
	ulint	size;

	if (!(n >> 32)) {
		return(mach_write_compressed(b, (ulint) n));
	}

	*b = (byte)0xFF;
	size = 1 + mach_write_compressed(b + 1, (ulint) (n >> 32));

	size += mach_write_compressed(b + size, (ulint) n & 0xFFFFFFFF);

	return(size);
}

/*********************************************************//**
Reads a 64-bit integer in a compressed form.
@return the value read */
UNIV_INLINE
ib_uint64_t
mach_u64_read_much_compressed(
/*==========================*/
	const byte*	b)	/*!< in: pointer to memory from where to read */
{
	ib_uint64_t	n;

	if (*b != 0xFF) {
		return(mach_read_compressed(b));
	}

	b++;
	n = mach_read_next_compressed(&b);
	n <<= 32;
	n |= mach_read_compressed(b);

	return(n);
}

/** Read a 64-bit integer in a compressed form.
@param[in,out]	b	pointer to memory where to read;
advanced by the number of bytes consumed
@return unsigned value */
UNIV_INLINE
ib_uint64_t
mach_read_next_much_compressed(
	const byte**	b)
{
	ib_uint64_t	val = mach_read_from_1(*b);

	if (val < 0x80) {
		/* 0nnnnnnn (7 bits) */
		++*b;
	} else if (val < 0xC0) {
		/* 10nnnnnn nnnnnnnn (14 bits) */
		val = mach_read_from_2(*b) & 0x3FFF;
		ut_ad(val > 0x7F);
		*b += 2;
	} else if (val < 0xE0) {
		/* 110nnnnn nnnnnnnn nnnnnnnn (21 bits) */
		val = mach_read_from_3(*b) & 0x1FFFFF;
		ut_ad(val > 0x3FFF);
		*b += 3;
	} else if (val < 0xF0) {
		/* 1110nnnn nnnnnnnn nnnnnnnn nnnnnnnn (28 bits) */
		val = mach_read_from_4(*b) & 0xFFFFFFF;
		ut_ad(val > 0x1FFFFF);
		*b += 4;
	} else if (val == 0xF0) {
		/* 11110000 nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn (32 bits) */
		val = mach_read_from_4(*b + 1);
		ut_ad(val > 0xFFFFFFF);
		*b += 5;
	} else {
		/* 11111111 followed by up to 64 bits */
		ut_ad(val == 0xFF);
		++*b;
		val = mach_read_next_compressed(b);
		ut_ad(val > 0);
		val <<= 32;
		val |= mach_read_next_compressed(b);
	}

	return(val);
}

/*********************************************************//**
Reads a double. It is stored in a little-endian format.
@return double read */
UNIV_INLINE
double
mach_double_read(
/*=============*/
	const byte*	b)	/*!< in: pointer to memory from where to read */
{
	double	d;
	ulint	i;
	byte*	ptr;

	ptr = (byte*) &d;

	for (i = 0; i < sizeof(double); i++) {
#ifdef WORDS_BIGENDIAN
		ptr[sizeof(double) - i - 1] = b[i];
#else
		ptr[i] = b[i];
#endif
	}

	return(d);
}

/*********************************************************//**
Writes a double. It is stored in a little-endian format. */
UNIV_INLINE
void
mach_double_write(
/*==============*/
	byte*	b,	/*!< in: pointer to memory where to write */
	double	d)	/*!< in: double */
{
	ulint	i;
	byte*	ptr;

	ptr = (byte*) &d;

	for (i = 0; i < sizeof(double); i++) {
#ifdef WORDS_BIGENDIAN
		b[i] = ptr[sizeof(double) - i - 1];
#else
		b[i] = ptr[i];
#endif
	}
}

/*********************************************************//**
Reads a float. It is stored in a little-endian format.
@return float read */
UNIV_INLINE
float
mach_float_read(
/*============*/
	const byte*	b)	/*!< in: pointer to memory from where to read */
{
	float	d;
	ulint	i;
	byte*	ptr;

	ptr = (byte*) &d;

	for (i = 0; i < sizeof(float); i++) {
#ifdef WORDS_BIGENDIAN
		ptr[sizeof(float) - i - 1] = b[i];
#else
		ptr[i] = b[i];
#endif
	}

	return(d);
}

/*********************************************************//**
Writes a float. It is stored in a little-endian format. */
UNIV_INLINE
void
mach_float_write(
/*=============*/
	byte*	b,	/*!< in: pointer to memory where to write */
	float	d)	/*!< in: float */
{
	ulint	i;
	byte*	ptr;

	ptr = (byte*) &d;

	for (i = 0; i < sizeof(float); i++) {
#ifdef WORDS_BIGENDIAN
		b[i] = ptr[sizeof(float) - i - 1];
#else
		b[i] = ptr[i];
#endif
	}
}

/*********************************************************//**
Reads a ulint stored in the little-endian format.
@return unsigned long int */
UNIV_INLINE
ulint
mach_read_from_n_little_endian(
/*===========================*/
	const byte*	buf,		/*!< in: from where to read */
	ulint		buf_size)	/*!< in: from how many bytes to read */
{
	ulint	n	= 0;
	const byte*	ptr;

	ut_ad(buf_size > 0);

	ptr = buf + buf_size;

	for (;;) {
		ptr--;

		n = n << 8;

		n += (ulint)(*ptr);

		if (ptr == buf) {
			break;
		}
	}

	return(n);
}

/*********************************************************//**
Writes a ulint in the little-endian format. */
UNIV_INLINE
void
mach_write_to_n_little_endian(
/*==========================*/
	byte*	dest,		/*!< in: where to write */
	ulint	dest_size,	/*!< in: into how many bytes to write */
	ulint	n)		/*!< in: unsigned long int to write */
{
	byte*	end;

	ut_ad(dest_size <= sizeof(ulint));
	ut_ad(dest_size > 0);

	end = dest + dest_size;

	for (;;) {
		*dest = (byte)(n & 0xFF);

		n = n >> 8;

		dest++;

		if (dest == end) {
			break;
		}
	}

	ut_ad(n == 0);
}

/*********************************************************//**
Reads a ulint stored in the little-endian format.
@return unsigned long int */
UNIV_INLINE
ulint
mach_read_from_2_little_endian(
/*===========================*/
	const byte*	buf)		/*!< in: from where to read */
{
	return((ulint)(buf[0]) | ((ulint)(buf[1]) << 8));
}

/*********************************************************//**
Writes a ulint in the little-endian format. */
UNIV_INLINE
void
mach_write_to_2_little_endian(
/*==========================*/
	byte*	dest,		/*!< in: where to write */
	ulint	n)		/*!< in: unsigned long int to write */
{
	ut_ad(n < 256 * 256);

	*dest = (byte)(n & 0xFFUL);

	n = n >> 8;
	dest++;

	*dest = (byte)(n & 0xFFUL);
}

/*********************************************************//**
Convert integral type from storage byte order (big endian) to
host byte order.
@return integer value */
UNIV_INLINE
ib_uint64_t
mach_read_int_type(
/*===============*/
	const byte*	src,		/*!< in: where to read from */
	ulint		len,		/*!< in: length of src */
	ibool		unsigned_type)	/*!< in: signed or unsigned flag */
{
	/* XXX this can be optimized on big-endian machines */

	uintmax_t	ret;
	uint		i;

	if (unsigned_type || (src[0] & 0x80)) {

		ret = 0x0000000000000000ULL;
	} else {

		ret = 0xFFFFFFFFFFFFFF00ULL;
	}

	if (unsigned_type) {

		ret |= src[0];
	} else {

		ret |= src[0] ^ 0x80;
	}

	for (i = 1; i < len; i++) {
		ret <<= 8;
		ret |= src[i];
	}

	return(ret);
}
/*********************************************************//**
Swap byte ordering. */
UNIV_INLINE
void
mach_swap_byte_order(
/*=================*/
        byte*           dest,           /*!< out: where to write */
        const byte*     from,           /*!< in: where to read from */
        ulint           len)            /*!< in: length of src */
{
        ut_ad(len > 0);
        ut_ad(len <= 8);

        dest += len;

        switch (len & 0x7) {
        case 0: *--dest = *from++; /* fall through */
        case 7: *--dest = *from++; /* fall through */
        case 6: *--dest = *from++; /* fall through */
        case 5: *--dest = *from++; /* fall through */
        case 4: *--dest = *from++; /* fall through */
        case 3: *--dest = *from++; /* fall through */
        case 2: *--dest = *from++; /* fall through */
        case 1: *--dest = *from;
        }
}

/*************************************************************
Convert a ulonglong integer from host byte order to (big-endian)
storage byte order. */
UNIV_INLINE
void
mach_write_ulonglong(
/*=================*/
	byte*		dest,		/*!< in: where to write */
	ulonglong	src,		/*!< in: where to read from */
	ulint		len,		/*!< in: length of dest */
	bool		usign)		/*!< in: signed or unsigned flag */
{
	byte*		ptr = reinterpret_cast<byte*>(&src);

	ut_ad(len <= sizeof(ulonglong));

#ifdef WORDS_BIGENDIAN
	memcpy(dest, ptr + (sizeof(src) - len), len);
#else
	mach_swap_byte_order(dest, reinterpret_cast<byte*>(ptr), len);
#endif /* WORDS_BIGENDIAN */

	if (!usign) {
		*dest ^=  0x80;
	}
}

#endif /* !UNIV_INNOCHECKSUM */
