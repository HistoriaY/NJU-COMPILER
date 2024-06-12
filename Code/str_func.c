#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "str_func.h"
#include "hash_table.h"

char *strdup(char *src)
{
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    strncpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

// so magic, so amazing, just use it
char *createFormattedString(const char *format, ...)
{
    // 获取格式化字符串长度
    va_list args;                                  // 创建 va_list 变量，用于存储可变参数列表
    va_start(args, format);                        // 初始化 va_list 变量，与最后一个固定参数之前的可变参数关联起来
    int length = vsnprintf(NULL, 0, format, args); // 获取格式化字符串在不分配内存的情况下的长度
    va_end(args);                                  // 清理 va_list 变量
    // 动态分配内存
    char *str = malloc((length + 1) * sizeof(char)); // 根据格式化字符串长度动态分配内存
    if (str == NULL)
    {
        // 内存分配失败处理
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }
    // 格式化字符串填充到动态分配的内存中
    va_start(args, format);                   // 初始化 va_list 变量，与最后一个固定参数之前的可变参数关联起来
    vsnprintf(str, length + 1, format, args); // 将格式化字符串填充到动态分配的内存中
    va_end(args);                             // 清理 va_list 变量
    return str;                               // 返回动态分配的字符串
}

unsigned int str_hash(void *str)
{
    char *p = (char *)str;
    unsigned int val = 0, i;
    for (; *p; ++p)
    {
        val = (val << 2) + *p;
        if ((i = val) & ~HASH_TABLE_SIZE)
            val = (val ^ (i >> 12)) & HASH_TABLE_SIZE;
    }
    return val;
}

int str_compare(void *str1, void *str2)
{
    return strcmp((char *)str1, (char *)str2);
}