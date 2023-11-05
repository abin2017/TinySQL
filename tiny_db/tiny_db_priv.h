

#ifndef _TINY_DB_PRIVATE_
#define _TINY_DB_PRIVATE_


#define TR_SUCCESS  0
#define TR_FAIL     -1

#define td_int32 int32_t
#define td_int16 signed short
#define td_uint32 uint32_t
#define td_uint16 unsigned short
#define td_char char
#define td_uchar unsigned char

#define TD_SET_BIT(x, bit)    ((x) |= (1 << (bit)))
#define TD_CLR_BIT(x, bit)    ((x) &= (~(1 << (bit))))
#define TD_GET_BIT(x, bit)    (((x) >> bit) & 1)
#define TD_ALIGN4(x) (((x)+3)&(~3))

//#define TD_MAKE_WORD(low, high)     ((td_uint16)(((td_uchar)(low)) | (((td_uint16)((td_uchar)(high))) << 8)))
//#define TD_MAKE_DWORD(low, high)    ((td_uint32)(((td_uint16)(low)) | (((td_uint32)((td_uint16)(high))) << 16)))

#define TD_MAKE_WORD(serial)     ((td_uint16)(((td_uchar)(serial[0])) | (((td_uint16)((td_uchar)(serial[1]))) << 8)))
#define TD_MAKE_DWORD(serial)    (serial[0] | (serial[1] >> 8) | (serial[2] >> 16) | (serial[3] >> 24))

#define TD_WORD_SERIALIZE(serial, word)     { serial[0] = word & 0xFF; serial[1] = (word >> 8) & 0xFF;}
#define TD_DWORD_SERIALIZE(serial, dword)   { serial[0] = dword & 0xFF; serial[1] = (dword >> 8) & 0xFF; serial[2] = (dword >> 16) & 0xFF; serial[3] = (dword >> 24) & 0xFF;}

#define TD_TRUE_JUMP(condition, tag, format, ...) if(condition){TINY_LOG_JUMP("\e[1;32m[%s] line %d "format"\033[0m", __FUNCTION__, __LINE__, ##__VA_ARGS__); goto tag;}
#define TD_TRUE_RETVAL(condition, val, format, ...) if(condition){TINY_LOG_JUMP("\e[1;32m[%s] line %d "format"\033[0m", __FUNCTION__, __LINE__, ##__VA_ARGS__); return val;}
#define TD_TRUE_RETVOD(condition, val, format, ...) if(condition){TINY_LOG_JUMP("\e[1;32m[%s] line %d "format"\033[0m", __FUNCTION__, __LINE__, ##__VA_ARGS__); return ;}

#endif
