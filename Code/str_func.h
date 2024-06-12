#ifndef STR_FUNC_H
#define STR_FUNC_H

char *strdup(char *src);

char *createFormattedString(const char *format, ...);

unsigned int str_hash(void *str);

int str_compare(void *str1, void *str2);

#endif
