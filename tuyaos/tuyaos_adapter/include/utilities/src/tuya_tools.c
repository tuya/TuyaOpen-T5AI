#include <string.h>
#include "tuya_tools.h"


#define __TOLOWER(c) ((('A' <= (c))&&((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c))


SIZE_T tuya_strlen(const CHAR_T *str)
{
    return strlen(str);
}

CHAR_T *tuya_strcpy(CHAR_T *dst, const CHAR_T *src)
{
    return strcpy(dst, src);
}

CHAR_T *tuya_strcat(CHAR_T* dst, const CHAR_T* src)
{
    return strcat(dst, src);
}



int tuya_strncasecmp(const CHAR_T *s1, const CHAR_T *s2, size_t n)
{
    const uint8_t *p1 = (const uint8_t *) s1;
    const uint8_t *p2 = (const uint8_t *) s2;
    int result = 0;
    int cnt = 0;

    if (p1 == p2) {
        return 0;
    }

    while (((result = __TOLOWER(*p1) - __TOLOWER(*p2)) == 0)) {
        if(++cnt >= n) {
            break;
        }

        if (*p1++ == '\0') {
            result = -1;
            break;
        }

        if(*p2++ == '\0') {
            result = 1;
            break;
        }
    }

    return result;
}

int tuya_strcmp(const CHAR_T *src,const CHAR_T *dst)
{
    int ret = 0;

    while(!(ret  =  *(uint8_t *)src - *(uint8_t *)dst) && *dst) {
        ++src,++dst;
    }

    if(ret < 0) {
        ret = -1;
    } else if(ret > 0) {
        ret = 1;
    }

    return ret;
}


uint8_t tuya_asc2hex(CHAR_T asccode)
{
    uint8_t ret;

    if('0'<=asccode && asccode<='9')
        ret=asccode-'0';
    else if('a'<=asccode && asccode<='f')
        ret=asccode-'a'+10;
    else if('A'<=asccode && asccode<='F')
        ret=asccode-'A'+10;
    else
        ret = 0;

    return ret;
}

void tuya_ascs2hex(uint8_t *hex, uint8_t *ascs, int srclen)
{
    uint8_t l4,h4;
    int i,lenstr;
    lenstr = srclen;

    if(lenstr%2) {
        lenstr -= (lenstr%2);
    }

    if(lenstr==0){
        return;
    }

    for(i=0; i < lenstr; i+=2) {
        h4 = tuya_asc2hex(ascs[i]);
        l4 = tuya_asc2hex(ascs[i+1]);
        hex[i/2]=(h4<<4)+l4;
    }
}

void tuya_hex2str(uint8_t *str, uint8_t *hex, int hexlen)
{
    CHAR_T ddl,ddh;
    int i;

    for (i=0; i<hexlen; i++) {
        ddh = 48 + hex[i] / 16;
        ddl = 48 + hex[i] % 16;
        if (ddh > 57) ddh = ddh + 7;
        if (ddl > 57) ddl = ddl + 7;
        str[i*2] = ddh;
        str[i*2+1] = ddl;
    }

    str[hexlen*2] = '\0';
}

BOOL_T tuya_str2num(uint32_t *number, const CHAR_T *str, uint8_t strlen)
{
    uint32_t value = 0;
    uint8_t i;

    for (i=0; i<strlen; i++) {
        if(str[i]>='0' && str[i]<='9') {
            value = value*10 + (str[i]-'0');
        }
        else {
            return FALSE;
        }
    }
    *number = value;
    return TRUE;
}

uint32_t tuya_intArray2int(uint8_t *intArray, uint32_t index, uint8_t len)
{
    if(index >= len) {
        return (uint32_t)-1;
    }
    
	uint32_t num = 0;
    uint8_t i = 0;
	for (i = index; i < index+len; i++) {
		num = (num*10) + intArray[i];
	}

	return num;
}

uint32_t tuya_int2intArray(uint32_t num, uint8_t *intArray, uint8_t len)
{
    uint8_t i = 0;
    uint32_t tmp = 0;
    
    tmp = num;
    do {
        tmp /= 10;
        i++;
    } while(tmp != 0);
    
    if(len < i) {
        return 0;
    }
    
    tmp = num;
    for(i = 0; tmp != 0; i++) {
        intArray[i] = tmp % 10;
        tmp /= 10;
    }
    
    tuya_buff_reverse(intArray, i);
    
    return i;
}

void tuya_buff_reverse(uint8_t *buf, UINT16_T len)
{
    uint8_t* p_tmp = buf;
    uint8_t  tmp;
    UINT16_T i;

    for(i=0; i<len/2; i++) {
        tmp = *(p_tmp+i);
        *(p_tmp+i) = *(p_tmp+len-1-i);
        *(p_tmp+len-1-i) = tmp;
    }
}

void tuya_data_reverse(uint8_t *dst, uint8_t *src, UINT16_T srclen)
{
    UINT16_T i;
    UINT16_T max_len = srclen;

    for(i=0; i<srclen; i++) {
        dst[i] = src[--max_len];
    }
}

// input: str->string
//        index->reverse index,start from 0
//        ch->find CHAR_T
// return: find position
int tuya_find_char_with_reverse_idx(const CHAR_T *str, const int index, const CHAR_T ch)
{
    if(NULL == str) {
        return -1;
    }

    int len = strlen(str);
    if(index >= len) {
        return -1;
    }

    int i = 0;
    for(i = (len-1-index);i >= 0;i--) {
        if(str[i] == ch) {
            return i;
        }
    }

    return -2;
}

void tuya_byte_sort(uint8_t is_ascend, uint8_t *buf, int len)
{
    int i,j;
    uint8_t tmp = 0;

    for(j = 1;j < len;j++) {
        for(i = 0;i < len-j;i++) {
            if(is_ascend) {
                if(buf[i] > buf[i+1]) {
                    tmp = buf[i];
                    buf[i] = buf[i+1];
                    buf[i+1] = tmp;
                }
            }
            else {
                if(buf[i] < buf[i+1]) {
                    tmp = buf[i];
                    buf[i] = buf[i+1];
                    buf[i+1] = tmp;
                }
            }
        }
    }
}

uint32_t tuya_bit1_count(uint32_t num)
{
    uint32_t count = 0;

    for (count = 0; num; ++count) {
        num &= (num -1); // clear lower bit
    }

    return count;
}

uint32_t tuya_leading_zeros_count(uint32_t num)
{
    uint32_t count = 0;
    uint32_t temp = ~num;

    while(temp & 0x80000000) {
        temp <<= 1;
        count++;
    }

    return count;
}

uint8_t tuya_check_sum8(uint8_t *buf, uint32_t len)
{
    uint8_t sum = 0;
    uint32_t idx=0;
    for(idx=0; idx<len; idx++) {
        sum += buf[idx];
    }
    return sum;
}

UINT16_T tuya_check_sum16(uint8_t *buf, uint32_t len)
{
    UINT16_T sum = 0;
    uint32_t idx=0;
    for(idx=0; idx<len; idx++) {
        sum += buf[idx];
    }
    return sum;
}



