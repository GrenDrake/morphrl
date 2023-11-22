#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>

class RNG {
public:
    RNG();
    RNG(uint64_t theSeed);

    void seed(uint64_t theSeed);
    uint64_t getState() const;
    uint64_t next64();
    uint64_t next32();

    int upto(int max);
    int between(int min, int max);
private:
    uint64_t mState;
};

#endif // RANDOM_H