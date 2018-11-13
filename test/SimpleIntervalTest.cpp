
#include <cstdio>
#include <cstdint>

#include "abstract_domain/SimpleInterval.h"

// Standard integer types
using s64 = std::int64_t;
using u64 = std::uint64_t;
using s32 = std::int32_t;
using u32 = std::uint32_t;
using s16 = std::int16_t;
using u16 = std::uint16_t;
using s8 = std::int8_t;
using u8 = std::uint8_t;


namespace pcpo {

static u64 rand_state = 0xd1620b2a7a243d4bull;
u64 rand64() {
    u64 x = rand_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rand_state = x;
    return x * 0x2545f4914f6cdd1dull;
}

void testSimpleDomain(u32 w, u32 iters, u64* errs) {
    std::fprintf(stderr, "Checking width %2d using %6d iterations, rand_state = 0x%lxull\n", (int)w, (int)iters, rand_state);

    u64 mask = ((u64)-1) >> (64 - w);
    
    SimpleInterval a (true, w);
    SimpleInterval b (true, w);
    APInt x (w, 0), y (w, 0), z (w, 0);

    SimpleInterval add0 (true, w);
    SimpleInterval add1 (true, w);
    SimpleInterval add2 (true, w);
    SimpleInterval sub0 (true, w);
    SimpleInterval sub1 (true, w);
    SimpleInterval sub2 (true, w);
    SimpleInterval mul0 (true, w);
    SimpleInterval mul1 (true, w);
    SimpleInterval mul2 (true, w);
    SimpleInterval udiv (true, w);
    SimpleInterval urem (true, w);
    SimpleInterval srem (true, w);
    SimpleInterval lub (true, w);
    SimpleInterval glb (true, w);
    SimpleInterval aeq (true, w);
    SimpleInterval ane (true, w);
    SimpleInterval aslt (true, w);
    SimpleInterval asle (true, w);
    SimpleInterval asge (true, w);
    SimpleInterval asgt (true, w);
    SimpleInterval ault (true, w);
    SimpleInterval aule (true, w);
    SimpleInterval auge (true, w);
    SimpleInterval augt (true, w);
    SimpleInterval beq (true, w);
    SimpleInterval bne (true, w);
    SimpleInterval bslt (true, w);
    SimpleInterval bsle (true, w);
    SimpleInterval bsge (true, w);
    SimpleInterval bsgt (true, w);
    SimpleInterval bult (true, w);
    SimpleInterval bule (true, w);
    SimpleInterval buge (true, w);
    SimpleInterval bugt (true, w);
    
    for (s64 i = 0; i < iters; ++i) {
        const s64 freq = 0x7ff;
        if ((i&freq) == freq && i+1 != iters) {
            std::fprintf(stderr, "iteration %d/%d, rand_state = 0x%lxull\n", (int)(i+1), (int)iters, rand_state);
        }
        u64 init_state = rand_state;
        u64 a_beg = rand64() & mask;
        u64 a_end = rand64() & mask;
        u64 b_beg = rand64() & mask;
        u64 b_end = rand64() & mask;
        u64 flags = rand64();

        // Ensure that the interval is not accidentally top
        a_end += a_beg == ((a_end+1) & mask);
        b_end += b_beg == ((b_end+1) & mask);
        
        a.begin = a_beg;
        a.end = a_end;
        b.begin = b_beg;
        b.end = b_end;

        // Set to bottom if last 8 bits are 0
        a.isBot = !(flags & 0xff);
        b.isBot = !(flags & 0xff << 8);

        // Clear begin and end if last 7 bits are 0, so if we end in 0x80 we get top instead of bottom
        a.begin &= !(flags & 0x7f) - 1;
        a.end |= -!(flags & 0x7f);
        b.begin &= !(flags & 0x7f << 8) - 1;
        b.end |= -!(flags & 0x7f << 8);


        // Interval comparison test
        u64 i_flags = (a == b) | (a <= b) << 1 | (b <= a) << 2;

        *errs += a.isTop() && !(i_flags & 4);
        *errs += b.isTop() && !(i_flags & 2);
        *errs += a.isBottom() && !(i_flags & 2);
        *errs += b.isBottom() && !(i_flags & 4);

        for (s64 j = 0; j < 256; ++j) {
            x = rand64() & mask;
            y = rand64() & mask;
            bool c_ax = a.contains(x);
            bool c_ay = a.contains(y);
            bool c_bx = b.contains(x);
            bool c_by = b.contains(y);
            *errs += (i_flags&1) && !(c_ax == c_bx && c_ay == c_by);
            *errs += (i_flags&2) && !(c_ax <= c_bx && c_ay <= c_by);
            *errs += (i_flags&4) && !(c_ax >= c_bx && c_ay >= c_by);
            
            if (*errs) goto err;
        }

        // One is bottom, cannot check operators
        if ((flags & 0xff) == 0 || (flags & 0xff00) == 0) continue;

        add0 = a._add(b, false, false);
        add1 = a._add(b, true, false);
        add2 = a._add(b, false, true);
        sub0 = a._sub(b, false, false);
        sub1 = a._sub(b, true, false);
        sub2 = a._sub(b, false, true);
        mul0 = a._mul(b, false, false);
        mul1 = a._mul(b, true, false);
        mul2 = a._mul(b, false, true);
        udiv = a._udiv(b);
        urem = a._urem(b);
        srem = a._srem(b);
        lub  = a._leastUpperBound(b);
        glb  = a._intersect(b);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_EQ, a, b,  &aeq , &beq );
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_NE, a, b,  &ane , &bne );
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_SLT, a, b, &aslt, &bslt);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_SLE, a, b, &asle, &bsle);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_SGE, a, b, &asge, &bsge);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_SGT, a, b, &asgt, &bsgt);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_ULT, a, b, &ault, &bult);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_ULE, a, b, &aule, &bule);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_UGE, a, b, &auge, &buge);
        SimpleInterval::_icmp(CmpInst::Predicate::ICMP_UGT, a, b, &augt, &bugt);

        // General sanity checks
        *errs += (a.isTop() || b.isTop()) && !add0.isTop();
        *errs += (a.isTop() || b.isTop()) && !sub0.isTop();
        *errs += (a.isTop() || b.isTop()) && !lub.isTop();
        *errs += (a.isTop() && b.isTop()) && !glb.isTop();
        *errs += !add0.isBot && (w != add0.bitWidth || w != add0.begin.getBitWidth() || w != add0.end.getBitWidth());
        *errs += !add1.isBot && (w != add1.bitWidth || w != add1.begin.getBitWidth() || w != add1.end.getBitWidth());
        *errs += !add2.isBot && (w != add2.bitWidth || w != add2.begin.getBitWidth() || w != add2.end.getBitWidth());
        *errs += !sub0.isBot && (w != sub0.bitWidth || w != sub0.begin.getBitWidth() || w != sub0.end.getBitWidth());
        *errs += !sub1.isBot && (w != sub1.bitWidth || w != sub1.begin.getBitWidth() || w != sub1.end.getBitWidth());
        *errs += !sub2.isBot && (w != sub2.bitWidth || w != sub2.begin.getBitWidth() || w != sub2.end.getBitWidth());
        *errs += !mul0.isBot && (w != mul0.bitWidth || w != mul0.begin.getBitWidth() || w != mul0.end.getBitWidth());
        *errs += !mul1.isBot && (w != mul1.bitWidth || w != mul1.begin.getBitWidth() || w != mul1.end.getBitWidth());
        *errs += !mul2.isBot && (w != mul2.bitWidth || w != mul2.begin.getBitWidth() || w != mul2.end.getBitWidth());
        *errs += !udiv.isBot && (w != udiv.bitWidth || w != udiv.begin.getBitWidth() || w != udiv.end.getBitWidth());
        *errs += !urem.isBot && (w != urem.bitWidth || w != urem.begin.getBitWidth() || w != urem.end.getBitWidth());
        *errs += !srem.isBot && (w != srem.bitWidth || w != srem.begin.getBitWidth() || w != srem.end.getBitWidth());
        *errs += !lub .isBot && (w != lub .bitWidth || w != lub .begin.getBitWidth() || w != lub .end.getBitWidth());
        *errs += !glb .isBot && (w != glb .bitWidth || w != glb .begin.getBitWidth() || w != glb .end.getBitWidth());

        *errs += !aeq .isBot && (w != aeq .bitWidth || w != aeq .begin.getBitWidth() || w != aeq .end.getBitWidth());
        *errs += !ane .isBot && (w != ane .bitWidth || w != ane .begin.getBitWidth() || w != ane .end.getBitWidth());
        *errs += !aslt.isBot && (w != aslt.bitWidth || w != aslt.begin.getBitWidth() || w != aslt.end.getBitWidth());
        *errs += !asle.isBot && (w != asle.bitWidth || w != asle.begin.getBitWidth() || w != asle.end.getBitWidth());
        *errs += !asge.isBot && (w != asge.bitWidth || w != asge.begin.getBitWidth() || w != asge.end.getBitWidth());
        *errs += !asgt.isBot && (w != asgt.bitWidth || w != asgt.begin.getBitWidth() || w != asgt.end.getBitWidth());
        *errs += !ault.isBot && (w != ault.bitWidth || w != ault.begin.getBitWidth() || w != ault.end.getBitWidth());
        *errs += !aule.isBot && (w != aule.bitWidth || w != aule.begin.getBitWidth() || w != aule.end.getBitWidth());
        *errs += !auge.isBot && (w != auge.bitWidth || w != auge.begin.getBitWidth() || w != auge.end.getBitWidth());
        *errs += !augt.isBot && (w != augt.bitWidth || w != augt.begin.getBitWidth() || w != augt.end.getBitWidth());
        *errs += !beq .isBot && (w != beq .bitWidth || w != beq .begin.getBitWidth() || w != beq .end.getBitWidth());
        *errs += !bne .isBot && (w != bne .bitWidth || w != bne .begin.getBitWidth() || w != bne .end.getBitWidth());
        *errs += !bslt.isBot && (w != bslt.bitWidth || w != bslt.begin.getBitWidth() || w != bslt.end.getBitWidth());
        *errs += !bsle.isBot && (w != bsle.bitWidth || w != bsle.begin.getBitWidth() || w != bsle.end.getBitWidth());
        *errs += !bsge.isBot && (w != bsge.bitWidth || w != bsge.begin.getBitWidth() || w != bsge.end.getBitWidth());
        *errs += !bsgt.isBot && (w != bsgt.bitWidth || w != bsgt.begin.getBitWidth() || w != bsgt.end.getBitWidth());
        *errs += !bult.isBot && (w != bult.bitWidth || w != bult.begin.getBitWidth() || w != bult.end.getBitWidth());
        *errs += !bule.isBot && (w != bule.bitWidth || w != bule.begin.getBitWidth() || w != bule.end.getBitWidth());
        *errs += !buge.isBot && (w != buge.bitWidth || w != buge.begin.getBitWidth() || w != buge.end.getBitWidth());
        *errs += !bugt.isBot && (w != bugt.bitWidth || w != bugt.begin.getBitWidth() || w != bugt.end.getBitWidth());
        
        if (*errs) goto err;

        // Operator test
        for (s64 j = 0; j < 256; ++j) {
            u64 ux = (rand64() % ((a_end - a_beg + 1) & mask) + a_beg) & mask;
            u64 uy = (rand64() % ((b_end - b_beg + 1) & mask) + b_beg) & mask;
            x = ux;
            y = uy;

            *errs += !(a.contains(x));
            *errs += !(b.contains(y));

            bool uov, sov;

            x.uadd_ov(y, uov); x.sadd_ov(y, sov);
            *errs += !add0.contains(x+y);
            *errs += !uov && !add1.contains(x+y);
            *errs += !sov && !add2.contains(x+y);
            x.usub_ov(y, uov); x.ssub_ov(y, sov);
            *errs += !sub0.contains(x-y);
            *errs += !uov && !sub1.contains(x-y);
            *errs += !sov && !sub2.contains(x-y);
            x.umul_ov(y, uov); x.smul_ov(y, sov);
            *errs += !mul0.contains(x*y);
            *errs += !uov && !mul1.contains(x*y);
            *errs += !sov && !mul2.contains(x*y);
            *errs += !y.isNullValue() && !udiv.contains(x.udiv(y));
            *errs += !y.isNullValue() && !urem.contains(x.urem(y));
            *errs += !y.isNullValue() && !srem.contains(x.srem(y));
            *errs += !lub.contains(x) || !lub.contains(y);
            *errs += b.contains(x) && !glb.contains(x);
            *errs += a.contains(y) && !glb.contains(y);
            *errs += !a.contains(y) && glb.contains(y); // It is not obvious that this condition has to be met. However, it is needed for narrowing.

            *errs += (x==y && !aeq.contains(x)) || (!(x==y) && !beq.contains(x));
            *errs += (x!=y && !ane.contains(x)) || (!(x!=y) && !bne.contains(x));
            *errs += (x.ult(y) && !ault.contains(x)) || (!(x.ult(y)) && !bult.contains(x));
            *errs += (x.ule(y) && !aule.contains(x)) || (!(x.ule(y)) && !bule.contains(x));
            *errs += (x.uge(y) && !auge.contains(x)) || (!(x.uge(y)) && !buge.contains(x));
            *errs += (x.ugt(y) && !augt.contains(x)) || (!(x.ugt(y)) && !bugt.contains(x));
            *errs += (x.slt(y) && !aslt.contains(x)) || (!(x.slt(y)) && !bslt.contains(x));
            *errs += (x.sle(y) && !asle.contains(x)) || (!(x.sle(y)) && !bsle.contains(x));
            *errs += (x.sge(y) && !asge.contains(x)) || (!(x.sge(y)) && !bsge.contains(x));
            *errs += (x.sgt(y) && !asgt.contains(x)) || (!(x.sgt(y)) && !bsgt.contains(x));

            if (*errs) goto err;
        }

        if (*errs) {
          err:
            std::fprintf(stderr, "Error in iteration %d, rand_state = 0x%lxull\n", (int)i, init_state);
            std::abort();
        }
    }
}

} // namespace pcpo


u64 error_count;
int main() {
    using namespace pcpo;
    u64 iters = 64;

    // Use this to reproduce a failing example more quickly. Simply insert the
    // last random hash the script outputs and the correct bitwidth.
    //rand_state = 0x98275dd51679216dull;
    //testSimpleDomain(17, iters, &error_count);
    
    while (true) {
        testSimpleDomain( 8, iters, &error_count);
        testSimpleDomain(16, iters, &error_count);
        testSimpleDomain(17, iters, &error_count);
        testSimpleDomain(32, iters, &error_count);
        testSimpleDomain(64, iters, &error_count);
        iters *= 2;
    }
}
