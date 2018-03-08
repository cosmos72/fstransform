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
        // order by length, then bits. used at runtime to match a given bitset
        return lhs.len < rhs.len || (lhs.len == rhs.len && lhs.bits < rhs.bits);
}

struct zinit_compare {
        FT_INLINE bool operator()(const zhuff & lhs, const zhuff & rhs)
        {
                // canonical huffman code ordering. used by zinit()
                return lhs.len < rhs.len || (lhs.len == rhs.len && lhs.sym < rhs.sym);
        }
};

static ft_u8 unzvec_start[31], code_min_len;
static zhuff unzvec[256], zvec[256];

void zinit()
{
        if (unzvec[0].len != 0)
                return;
        for (size_t i = 0; i < 256; i++) {
                unzvec[i].set(i, huffman_lengths[i], 0);
        }
        std::sort(unzvec, unzvec+256, zinit_compare());
        // rebuild canonical huffman code from lengths
        ft_u16 bits = unzvec[0].bits = 0;
        ft_u8 len = unzvec[0].len, newlen;

        unzvec_start[len] = 0;
        code_min_len = len;

        for (size_t i = 1; i < 256; i++) {
                newlen = unzvec[i].len;
                bits = (bits+1) << (newlen - len);
                unzvec[i].bits = bits;
                while (len < newlen)
                        unzvec_start[++len] = i;
        }
        for (size_t i = 0; i < 256; i++) {
                zhuff & code = unzvec[i];
                zvec[code.sym].set(code.sym, code.len, code.bits);
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
        ft_u8 len = 0, match_len = 0;

        if (s[0] != '\0') {
                // src is not compressed
                dst = src;
                return;
        }
        for (ft_size i = 1, n = src.size(); i < n; i++) {
                bits = (bits << 8) | (ft_u8)s[i];
                len += 8;
again:
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
                        match_len = 0;
                }
                if (match_len < len) {
                        match_len++;
                        goto again;
                }
        }
}

int ztest()
{
        enum { N = 12 };
        ft_string dst, tmp, src[N] = {
                "/bin/bash",
                "/sbin/init",
                "/usr/sbin/apache2",
                "/usr/lib/bluetooth/obexd",
                "/usr/lib/python2.7/dist-packages/OpenGL/GLES1/ARM/__init__.py",
                "/usr/lib/vmware/lib/libglibmm_generate_extra_defs-2.4.so.1/libglibmm_generate_extra_defs-2.4.so.1",
                "/usr/local/docs/ref/am_conf/byteorder.html",
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
