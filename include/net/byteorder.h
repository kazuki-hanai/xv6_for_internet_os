#pragma once
//
// endianness support
//

static inline uint16_t bswaps(uint16_t val)
{
  return (((val & 0x00ffU) << 8) |
          ((val & 0xff00U) >> 8));
}

static inline uint32_t bswapl(uint32_t val)
{
  return (((val & 0x000000ffUL) << 24) |
          ((val & 0x0000ff00UL) << 8) |
          ((val & 0x00ff0000UL) >> 8) |
          ((val & 0xff000000UL) >> 24));
}

// Use these macros to convert network bytes to the native byte order.
// Note that Risc-V uses little endian while network order is big endian.
#define ntohs bswaps
#define ntohl bswapl
#define htons bswaps
#define htonl bswapl

#define nbuftohs(buf) bswaps((uint16_t)*buf)
#define nbuftohl(buf) bswapl((uint32_t)*buf)
#define hbuftons(buf) bswaps((uint16_t)*buf)
#define hbuftonl(buf) bswapl((uint32_t)*buf)
