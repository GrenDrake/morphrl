#include <ctime>
#include "random.h"

RNG::RNG()
: mState(0)
{
    seed(0);
}

RNG::RNG(uint64_t theSeed)
: mState(theSeed)
{
    if (theSeed == 0) seed(0);
}

void RNG::seed(uint64_t theSeed) {
    if (theSeed == 0) {
        mState = time(nullptr);
    } else mState = theSeed;
}

uint64_t RNG::getState() const {
    return mState;
}

uint64_t RNG::next64() {
    mState ^= mState >> 12;
    mState ^= mState << 25;
    mState ^= mState >> 27;
    return mState * 0x2545F4914F6CDD1DULL;
}

uint64_t RNG::next32() {
    return next64() >> 32;
}

int RNG::upto(int max) {
    return next32() % max;
}

int RNG::between(int min, int max) {
    int range = max - min + 1;
    return min + upto(range);
}
