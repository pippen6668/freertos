#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1/UCHAR_MAX)                                                                      
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)

#define SS (sizeof(size_t))
void *memset(void *dest, int c, size_t n)
{
	unsigned char *s = dest;
	c = (unsigned char)c;
	for (; ((uintptr_t)s & ALIGN) && n; n--) *s++ = c;
	if (n) {
		size_t *w, k = ONES * c;
		for (w = (void *)s; n>=SS; n-=SS, w++) *w = k;
		for (s = (void *)w; n; n--, s++) *s = c;
	}
	return dest;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	void *ret = dest;
	
	//Cut rear
	uint8_t *dst8 = dest;
	const uint8_t *src8 = src;
	switch (n % 4) {
		case 3 : *dst8++ = *src8++;
		case 2 : *dst8++ = *src8++;
		case 1 : *dst8++ = *src8++;
		case 0 : ;
	}
	
	//stm32 data bus width
	uint32_t *dst32 = (void *)dst8;
	const uint32_t *src32 = (void *)src8;
	n = n / 4;
	while (n--) {
		*dst32++ = *src32++;
	}
	
	return ret;
}

char *strchr(const char *s, int c)
{
	for (; *s && *s != c; s++);
	return (*s == c) ? (char *)s : NULL;
}

char *strcpy(char *dest, const char *src)
{
	const unsigned char *s = src;
	unsigned char *d = dest;
	while ((*d++ = *s++));
	return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	const unsigned char *s = src;
	unsigned char *d = dest;
	while (n-- && (*d++ = *s++));
	return dest;
}
size_t strlen ( const char * str )
{
    int count;
    for(count=0 ; str[count]!='\0';count++);
    return count;
}

int strcmp ( const char * str1, const char * str2)
{
   int i=0;
    while( (str1[i]!='\0' && str2[i]!='\0') )
    {
        if(str1[i]!=str2[i])
        {
            return (str1[i]>str2[i])?1:-1;
        }
        i++;
    }

    return 0;
 }
 char* itoa(int num, char* string)
{
    int i=1;
        int num2=num;

        if(num2 < 10)
        {   string[0]=num%10+48;
            string[1]='\0';
            return string;
        }
    while(num2 > 9)
    {

        num2 /=10;
            i++;
        }

        num2=num;
        i=i-1;
        while(num2>0)
        {
          string[i]= (num2 %10)+48;
          num2/=10;
          i--;
        }

    string[i]='\0';
    return string;

}
int atoi(const char *string)
{
    int value = 0;
    for (; *string != '\0'; ++string)
        value = value*10 + *string - '0';
    return value;
}
char * strcat ( char * dst, const char * src)
{
    int dst_l = strlen(dst);
    int src_l = strlen(src);
    int i;
    for (i = 0; i<src_l; i++)
    {
        dst[dst_l+i]=src[i];
    }
    dst[dst_l+src_l]='\0';
    return dst;
}
 
