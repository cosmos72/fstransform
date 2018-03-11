/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * zstring.hh
 *
 *  Created on: Mar 5, 2018
 *      Author: max
 */

#include "first.hh"

#include <algorithm> // std::sort()
#include <set>       // std::set<T>
#include <stdexcept> // std::invalid_argument
#include <vector>    // std::vector<T>

#include "zstring.hh"

FT_NAMESPACE_BEGIN

static const ft_u8 huffman_lengths[256] = {
        14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
        7, 11, 11, 11, 11, 11, 11, 11, 12, 11, 11, 11, 11, 6, 5, 3, 6, 6, 7, 7, 7, 7, 7, 8, 7, 8, 11, 11, 11, 11, 11, 11,
        11, 9, 9, 8, 8, 8, 9, 9, 10, 8, 11, 11, 9, 8, 10, 10, 8, 11, 10, 8, 8, 10, 10, 9, 10, 11, 11, 11, 11, 11, 11, 6, 11,
        4, 6, 5, 5, 4, 6, 6, 6, 4, 8, 7, 5, 5, 5, 4, 6, 10, 5, 5, 5, 6, 7, 7, 6, 8, 9, 11, 12, 12, 12, 11, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
};

struct zhuff {
        ft_u8 sym, len;
        ft_u16 bits;

        FT_INLINE void set(ft_u8 symbol, ft_u8 length, ft_u64 bitset) {
                sym = symbol;
                len = length;
                bits = bitset;
        }
};

static FT_INLINE bool operator<(const zhuff & lhs, const zhuff & rhs)
{
        // canonical huffman code ordering. used by zinit()
        return lhs.len < rhs.len || (lhs.len == rhs.len && lhs.sym < rhs.sym);
};

struct unzbyte {
        // how to decode a single byte
        ft_u8 nbits[3], sym[3];
};

static ft_u8 unzvec_start[31], code_min_len;
static zhuff unzvec[256], zvec[256];

enum { UNZBITS = 12 };
static unzbyte unztable[1 << UNZBITS];

static void unztable_init();

void zinit()
{
        if (unzvec[0].len != 0)
                return;
        for (ft_size i = 0; i < 256; i++) {
                unzvec[i].set(i, huffman_lengths[i], 0);
        }
        std::sort(unzvec, unzvec+256);
        // rebuild canonical huffman code from lengths
        ft_u16 bits = unzvec[0].bits = 0;
        ft_u8 len = unzvec[0].len, newlen;

        unzvec_start[len] = 0;
        code_min_len = len;

        for (ft_size i = 1; i < 256; i++) {
                newlen = unzvec[i].len;
                bits = (bits+1) << (newlen - len);
                unzvec[i].bits = bits;
                while (len < newlen)
                        unzvec_start[++len] = i;
        }
        for (ft_size i = 0; i < 256; i++) {
                zhuff & code = unzvec[i];
                zvec[code.sym].set(code.sym, code.len, code.bits);
        }
        unztable_init();
}

static void unztable_init()
{
        ft_u8 ilen, jlen, klen;
        for (ft_size i = 0; i < 256; i++) {
                ilen = unzvec[i].len;
                if (ilen > UNZBITS)
                        break;
                for (ft_size j = 0; j < 256; j++) {
                        jlen = unzvec[j].len;
                        if (ilen + jlen > UNZBITS)
                                break;
                        for (ft_size k = 0; k < 256; k++) {
                                klen = unzvec[k].len;
                                if (ilen + jlen + klen > UNZBITS)
                                        break;
                                ft_u8 free_len = UNZBITS - ilen - jlen - klen;
                                ft_u16 bits = ((ft_u16)unzvec[i].bits << (UNZBITS - ilen)) |
                                        ((ft_u16)unzvec[j].bits << (UNZBITS - ilen - jlen)) |
                                        ((ft_u16)unzvec[k].bits << free_len);
                                for (ft_size f = 0; f < (1U<<free_len); f++) {
                                        unzbyte & entry = unztable[bits + f];
                                        entry.nbits[0] = ilen;
                                        entry.nbits[1] = jlen;
                                        entry.nbits[2] = klen;
                                        entry.sym[0] = unzvec[i].sym;
                                        entry.sym[1] = unzvec[j].sym;
                                        entry.sym[2] = unzvec[k].sym;
                                }
                        }
                        ft_u8 free_len = UNZBITS - ilen - jlen;
                        ft_u16 bits = ((ft_u16)unzvec[i].bits << (UNZBITS - ilen)) | ((ft_u16)unzvec[j].bits << free_len);
                        for (ft_size f = 0; f < (1U<<free_len); f++) {
                                unzbyte & entry = unztable[bits + f];
                                if (entry.nbits[0])
                                        continue;
                                entry.nbits[0] = ilen;
                                entry.nbits[1] = jlen;
                                entry.nbits[2] = 0;
                                entry.sym[0] = unzvec[i].sym;
                                entry.sym[1] = unzvec[j].sym;
                                entry.sym[2] = 0;
                        }
                }
                ft_u8 free_len = UNZBITS - ilen;
                ft_u16 bits = (ft_u16)unzvec[i].bits << free_len;
                for (ft_size f = 0; f < (1U<<free_len); f++) {
                        unzbyte & entry = unztable[bits + f];
                        if (entry.nbits[0])
                                continue;
                        entry.nbits[0] = ilen;
                        entry.nbits[1] = 0;
                        entry.nbits[2] = 0;
                        entry.sym[0] = unzvec[i].sym;
                        entry.sym[1] = 0;
                        entry.sym[2] = 0;
                }
        }
}


static FT_INLINE ft_u8 compress_byte(ft_u16 & dst_bits, ft_u8 byte) {
        zhuff & code = zvec[byte];
        dst_bits = code.bits;
        return code.len;
}

void z(ft_string & dst, const ft_string & src, bool force)
{
        dst.resize(0);
        if (src.empty())
                return;

        const char * s = src.c_str();
        ft_u32 bits = 0;
        ft_u8 len = 0;

        dst.push_back('\0'); // compressed strings marker
        for (ft_size i = 0, n = src.size(); i < n; i++) {
                const zhuff & code = zvec[(ft_u8)s[i]];
                bits = (bits << code.len) | (ft_u32)code.bits;
                len += code.len;
                while (len >= 8) {
                        dst.push_back((ft_u8)(bits >> (len - 8)));
                        len -= 8;
                }
        }
        if (len) {
                // fill any remainder with ones. 0b111..111 is the longest code:
                // it will never fit 7 bits or less, so we can just discard
                // any incomplete last fragment when decompressing
                bits = (bits << (8-len)) | ((ft_u32)0xff >> len);
                dst.push_back((ft_u8)bits);
        }
        if (!force && dst.size() > src.size()) {
                // no gain. keep src uncompressed
                dst = src;
        }
}

void unz(ft_string & dst, const ft_string & src)
{
        dst.resize(0);
        if (src.empty())
                return;

        const char * s = src.c_str();
        ft_u32 bits = 0;
        ft_u8 len = 0, match_len = code_min_len;

        if (s[0] != '\0') {
                // src is not compressed
                dst = src;
                return;
        }
        ft_size i = 1, n = src.size();
        while (i < n) {
                do {
                        bits = (bits << 8) | (ft_u8)s[i++];
                        len += 8;
                } while (i < n && len <= 24);
fast:
                // fast decode
                if (match_len <= UNZBITS) {
                        while (len >= code_min_len) {
                                ft_u16 match_bits = len >= UNZBITS ? (bits >> (len - UNZBITS)) : (bits << (UNZBITS - len));
                                const unzbyte & entry = unztable[match_bits & ((1<<UNZBITS)-1)];
                                ft_u8 nbits;
                                if (!(nbits = entry.nbits[0])) {
                                        match_len = UNZBITS + 1;
                                        if (match_len <= len)
                                                goto slow;
                                        break;
                                }
                                if (nbits > len)
                                        break;

                                dst.push_back(entry.sym[0]);
                                len -= nbits;
                                if ((nbits = entry.nbits[1]) && nbits <= len) {
                                        dst.push_back(entry.sym[1]);
                                        len -= nbits;
                                        if ((nbits = entry.nbits[2]) && nbits <= len) {
                                                dst.push_back(entry.sym[2]);
                                                len -= nbits;
                                        }
                                }
                                // actually consume bits
                                bits &= ((ft_u32)0x7fffffffu >> (31 - len)); // (ft_u32)x >> 32 is implementation-defined
                                match_len = code_min_len;
                        }
                        if (match_len > len || (match_len <= UNZBITS && len < UNZBITS && i < n)) // read more bits only if needed
                                continue;
                }
slow:
                // slow decode
                ft_u16 code_start, code_end;
                while ((code_start = unzvec_start[match_len]) == (code_end = unzvec_start[match_len + 1]) && match_len < len)
                        match_len++;

                if (code_start == code_end) { // also implies match_len == len
                        if (match_len < 16)
                                continue; // read more compressed bits
                        if (i == n - 1 && bits == ((ft_u32)0x7fffffffu >> (31 - len))) // (ft_u32)x >> 32 is implementation-defined
                                break; // ignore final padding with 0b11...11
                        throw std::invalid_argument("invalid compressed string");
                }
                ft_u32 match_bits = (bits >> (len - match_len));
                ft_u32 code_start_bits = unzvec[code_start].bits;
                if (match_bits >= code_start_bits && match_bits <= unzvec[(ft_u8)(code_end-1)].bits) { // code_end == 256 is stored as 0 in unzvec_start[]
                        // for a given length, unzvec[].bits have consecutive values:
                        // no need to search, simply jump to the correct code
                        code_start += match_bits - code_start_bits;
                        dst.push_back(unzvec[code_start].sym);
                        
                        // actually consume bits
                        len -= match_len;
                        bits &= ((ft_u32)0x7fffffffu >> (31 - len)); // (ft_u32)x >> 32 is implementation-defined
                        match_len = code_min_len - 1;
                }
                if (match_len < len) {
                        match_len++;
                        if (i < n)
                                continue;
                        if (match_len <= UNZBITS && len >= UNZBITS)
                                goto fast;
                        if (match_len <= len)
                                goto slow;
                }
        }
}

int ztest()
{
        enum { N = 12 };
        ft_string dst, tmp, src[N] = {
                "/usr/local/docs/ref/am_conf/byteorder.html",
                "/bin/bash",
                "/sbin/init",
                "/usr/sbin/apache2",
                "/usr/lib/bluetooth/obexd",
                "/usr/lib/python2.7/dist-packages/OpenGL/GLES1/ARM/__init__.py",
                "/usr/lib/vmware/lib/libglibmm_generate_extra_defs-2.4.so.1/libglibmm_generate_extra_defs-2.4.so.1",
                "/sys/devices/pci0000:00/power",
                "/sys/kernel/mm/transparent_hugepage/khugepaged/alloc_sleep_millisecs",
                "/media/windows/Users/Default/AppData/Local/Microsoft/Windows/Temporary Internet Files/Content.IE5/87654321/desktop.ini",
                "/media/windows/Users/Default/AppData/Local/Microsoft/Windows/Temporary Internet Files/Low/Content.IE5/12345678/56K1B4N7B8NY[W95M8MW6948Y69W4MN8BM6WMB98WN8U9GMT8T07DMYTRVVROIWURYNOIMUAOUURREWREWLKMLNBHOIU786RUYFUTFDE64EDCJHGJYURY5ESGFCHDSTSKBOTGO.jpg",
                "",
        };

        tmp.resize(1024*1024*1024);
        for (ft_size i = 0, n = tmp.size(); i < n; i++)
                tmp[i] = (ft_u8)i;
        src[N-1].swap(tmp);

        zinit();
        for (ft_size i = 0; i < N; i++) {
                z(tmp, src[i], true);
                unz(dst, tmp);
                if (dst != src[i])
                        throw std::invalid_argument("test failed! mismatch between original and decompressed string");
        }
        return 0;
}

FT_NAMESPACE_END
