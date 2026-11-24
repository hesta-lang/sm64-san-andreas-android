/* 
 * TinySHA1 - header-only SHA-1 implementation.
 * Copyright (c) 2012-22 SAURAV MOHAPATRA <mohaps@gmail.com>
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */
#ifndef TINY_SHA1_HPP
#define TINY_SHA1_HPP
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace sha1 {
class SHA1 {
public:
    using digest32_t = uint32_t[5];
    SHA1() { reset(); }
    SHA1& reset() {
        m_digest[0]=0x67452301; m_digest[1]=0xEFCDAB89; m_digest[2]=0x98BADCFE;
        m_digest[3]=0x10325476; m_digest[4]=0xC3D2E1F0;
        m_index=0; m_bytes=0; return *this;
    }
    SHA1& processByte(uint8_t value) {
        m_block[m_index++]=value; ++m_bytes;
        if(m_index==64) { m_index=0; processBlock(); }
        return *this;
    }
    SHA1& processBytes(const void* data, size_t length) {
        const auto* p=static_cast<const uint8_t*>(data);
        for(size_t i=0;i<length;++i) processByte(p[i]);
        return *this;
    }
    const uint32_t* getDigest(digest32_t out) {
        const uint64_t bits=static_cast<uint64_t>(m_bytes)*8u;
        processByte(0x80);
        while(m_index!=56) processByte(0);
        processByte(static_cast<uint8_t>(bits>>56));
        processByte(static_cast<uint8_t>(bits>>48));
        processByte(static_cast<uint8_t>(bits>>40));
        processByte(static_cast<uint8_t>(bits>>32));
        processByte(static_cast<uint8_t>(bits>>24));
        processByte(static_cast<uint8_t>(bits>>16));
        processByte(static_cast<uint8_t>(bits>>8));
        processByte(static_cast<uint8_t>(bits));
        std::memcpy(out,m_digest,sizeof(m_digest));
        return out;
    }
private:
    static uint32_t rol(uint32_t x,unsigned n) { return (x<<n)|(x>>(32-n)); }
    void processBlock() {
        uint32_t w[80];
        for(size_t i=0;i<16;++i) {
            w[i]=(uint32_t(m_block[i*4])<<24)|(uint32_t(m_block[i*4+1])<<16)|
                 (uint32_t(m_block[i*4+2])<<8)|uint32_t(m_block[i*4+3]);
        }
        for(size_t i=16;i<80;++i) w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        uint32_t a=m_digest[0],b=m_digest[1],c=m_digest[2],d=m_digest[3],e=m_digest[4];
        for(size_t i=0;i<80;++i) {
            uint32_t f,k;
            if(i<20) { f=(b&c)|(~b&d); k=0x5A827999; }
            else if(i<40) { f=b^c^d; k=0x6ED9EBA1; }
            else if(i<60) { f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDC; }
            else { f=b^c^d; k=0xCA62C1D6; }
            const uint32_t t=rol(a,5)+f+e+k+w[i];
            e=d; d=c; c=rol(b,30); b=a; a=t;
        }
        m_digest[0]+=a; m_digest[1]+=b; m_digest[2]+=c; m_digest[3]+=d; m_digest[4]+=e;
    }
    uint32_t m_digest[5]{};
    uint8_t m_block[64]{};
    size_t m_index=0;
    size_t m_bytes=0;
};
}
#endif
