/*
 * kl_string.h
 *
 *  Created on: 5 дек. 2020 г.
 *      Author: layst
 */

#pragma once

#ifndef LONG_MIN
#define LONG_MIN    (-2147483648)
#endif

#ifndef LONG_MAX
#define LONG_MAX    (2147483647)
#endif


int kl_strcasecmp(const char *s1, const char *s2);
char* kl_strtok(char* s, const char* delim, char**PLast);
int kl_sscanf(const char* s, const char* format, ...);
int kl_strlen(const char* s);
long kl_strtol(const char *nptr, char **endptr, int base);
