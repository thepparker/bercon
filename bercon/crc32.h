#include <inttypes.h>

class crc32
{
    public:
        crc32(void);
        ~crc32(void);

        void init();

        uint32_t calc_crc32(const char *buf, size_t len);

    private:
        unsigned int lookupTable[256];
};
