
#include <cstdio>
#include <cstdint>

#include "simple_interval.h"

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
    // The test just does all the operations with random values and checks whether it outputs
    // something sensible. Of course, this cannot check whether something accidentally becomes top.
    
    using APInt = llvm::APInt;
    
    std::fprintf(stderr, "Checking width %2d using %6d iterations, rand_state = 0x%lxull\n", (int)w, (int)iters, rand_state);

    u64 mask = ((u64)-1) >> (64 - w);
    
    SimpleInterval a {true};
    SimpleInterval b {true};
    APInt x {w, 0}, y {w, 0}, z {w, 0};

    SimpleInterval a_;
    SimpleInterval b_;

    SimpleInterval add0, add1, add2, sub0, sub1;
    SimpleInterval sub2, mul0, mul1, mul2, udiv;
    SimpleInterval urem, srem, lub, glb;
    SimpleInterval aeq, ane, aslt, asle, asge;
    SimpleInterval asgt, ault, aule, auge, augt;
    SimpleInterval beq, bne, bslt, bsle, bsge;
    SimpleInterval bsgt, bult, bule, buge, bugt;
    
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

        a = SimpleInterval {APInt {w, a_beg}, APInt {w, a_end}};
        b = SimpleInterval {APInt {w, b_beg}, APInt {w, b_end}};

        // Set to bottom if last 8 bits are 0
        if (!(flags      & 0x7f)) a = {!!(flags      & 0xff)};
        if (!(flags >> 8 & 0x7f)) b = {!!(flags >> 8 & 0xff)};

        a_beg &= ( !(flags & 0x7f) - 1     ) & mask;
        a_end |= (-!(flags & 0x7f)         ) & mask;
        a_end -= ( !(flags & 0x7f)         ) & mask;
        b_beg &= ( !(flags & 0x7f << 8) - 1) & mask;
        b_end |= (-!(flags & 0x7f << 8)    ) & mask;
        b_end -= ( !(flags & 0x7f << 8)    ) & mask;

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

        a_ = a._makeTopInterval(w);
        b_ = b._makeTopInterval(w);
               
        add0 = a_._Add(b_, false, false)._makeTopSpecial();
        add1 = a_._Add(b_, true, false) ._makeTopSpecial();
        add2 = a_._Add(b_, false, true) ._makeTopSpecial();
        sub0 = a_._Sub(b_, false, false)._makeTopSpecial();
        sub1 = a_._Sub(b_, true, false) ._makeTopSpecial();
        sub2 = a_._Sub(b_, false, true) ._makeTopSpecial();
        mul0 = a_._Mul(b_, false, false)._makeTopSpecial();
        mul1 = a_._Mul(b_, true, false) ._makeTopSpecial();
        mul2 = a_._Mul(b_, false, true) ._makeTopSpecial();
        udiv = a_._UDiv(b_)             ._makeTopSpecial();
        urem = a_._URem(b_)             ._makeTopSpecial();
        srem = a_._SRem(b_)             ._makeTopSpecial();
        lub  = a_._upperBound(b_)       ._makeTopSpecial();
        glb  = a_._narrow(b_)           ._makeTopSpecial();
        aeq  = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_EQ,  a_, b_)._makeTopSpecial();
        ane  = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_NE,  a_, b_)._makeTopSpecial();
        aslt = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SLT, a_, b_)._makeTopSpecial();
        asle = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SLE, a_, b_)._makeTopSpecial();
        asge = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SGE, a_, b_)._makeTopSpecial();
        asgt = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SGT, a_, b_)._makeTopSpecial();
        ault = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_ULT, a_, b_)._makeTopSpecial();
        aule = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_ULE, a_, b_)._makeTopSpecial();
        auge = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_UGE, a_, b_)._makeTopSpecial();
        augt = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_UGT, a_, b_)._makeTopSpecial();
        beq  = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_EQ,  b_, a_)._makeTopSpecial();
        bne  = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_NE,  b_, a_)._makeTopSpecial();
        bslt = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SLT, b_, a_)._makeTopSpecial();
        bsle = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SLE, b_, a_)._makeTopSpecial();
        bsge = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SGE, b_, a_)._makeTopSpecial();
        bsgt = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_SGT, b_, a_)._makeTopSpecial();
        bult = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_ULT, b_, a_)._makeTopSpecial();
        bule = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_ULE, b_, a_)._makeTopSpecial();
        buge = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_UGE, b_, a_)._makeTopSpecial();
        bugt = SimpleInterval::_refineBranch(llvm::CmpInst::Predicate::ICMP_UGT, b_, a_)._makeTopSpecial();

        // General sanity checks
        *errs += (a.isTop() || b.isTop()) && !add0.isTop();
        *errs += (a.isTop() || b.isTop()) && !sub0.isTop();
        *errs += (a.isTop() || b.isTop()) && !lub.isTop();
        *errs += (a.isTop() && b.isTop()) && !glb.isTop();

#define SANITY(x)                                                       \
        *errs += x.state == SimpleInterval::NORMAL && (                 \
            w != x.begin.getBitWidth() || w != x.end.getBitWidth() || x.begin == x.end + 1)

        SANITY(add0); SANITY(add1); SANITY(add2); SANITY(sub0); SANITY(sub1);
        SANITY(sub2); SANITY(mul0); SANITY(mul1); SANITY(mul2); SANITY(udiv);
        SANITY(urem); SANITY(srem); SANITY(lub ); SANITY(glb );

        SANITY(aeq ); SANITY(ane ); SANITY(aslt); SANITY(asle); SANITY(asge);
        SANITY(asgt); SANITY(ault); SANITY(aule); SANITY(auge); SANITY(augt);
        SANITY(beq ); SANITY(bne ); SANITY(bslt); SANITY(bsle); SANITY(bsge);
        SANITY(bsgt); SANITY(bult); SANITY(bule); SANITY(buge); SANITY(bugt);

#undef SANITY
        
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
            *errs += !a.contains(y) && glb.contains(y);

            *errs += (x==y     && !aeq .contains(x)) || (y==x     && !beq.contains(y));
            *errs += (x!=y     && !ane .contains(x)) || (y!=x     && !bne.contains(y));
            *errs += (x.ult(y) && !ault.contains(x)) || (y.ult(x) && !bult.contains(y));
            *errs += (x.ule(y) && !aule.contains(x)) || (y.ule(x) && !bule.contains(y));
            *errs += (x.uge(y) && !auge.contains(x)) || (y.uge(x) && !buge.contains(y));
            *errs += (x.ugt(y) && !augt.contains(x)) || (y.ugt(x) && !bugt.contains(y));
            *errs += (x.slt(y) && !aslt.contains(x)) || (y.slt(x) && !bslt.contains(y));
            *errs += (x.sle(y) && !asle.contains(x)) || (y.sle(x) && !bsle.contains(y));
            *errs += (x.sge(y) && !asge.contains(x)) || (y.sge(x) && !bsge.contains(y));
            *errs += (x.sgt(y) && !asgt.contains(x)) || (y.sgt(x) && !bsgt.contains(y));

            if (*errs) goto err;
        }

        if (*errs) {
          err:
            std::fprintf(stderr, "Error in iteration %d, rand_state = 0x%lxull\n", (int)i, init_state);
            std::fprintf(stderr, "To debug this, please update the initial value for rand_state in main() and set a watchpoint to the global variable error_count.\n");
            std::abort();
        }
    }
}

} // end of namespace pcpo


u64 error_count;
int main() {
    using namespace pcpo;
    u64 iters = 64;

    // Use this to reproduce a failing example more quickly. Simply insert the
    // last random hash the script outputs and the correct bitwidth.
    //rand_state = 0xe596fd2a27fe71c7ull;
    //testSimpleDomain(16, iters, &error_count);
    
    while (true) {
        testSimpleDomain( 8, iters, &error_count);
        testSimpleDomain(16, iters, &error_count);
        testSimpleDomain(17, iters, &error_count);
        testSimpleDomain(32, iters, &error_count);
        testSimpleDomain(64, iters, &error_count);
        iters *= 2;
    }
}
