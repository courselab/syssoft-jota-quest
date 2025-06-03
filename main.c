/*
 *    SPDX-FileCopyrightText: 2021 Monaco F. J. <monaco@usp.br>
 *    SPDX-FileCopyrightText: 2025 jotaefepinho <jotaefepinho@gmail.com>
 *   
 *    SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  This file is a derivative of SYSeg (https://gitlab.com/monaco/syseg)
 *  and includes modifications made by the following author(s):
 *  jotaefepinho <jotaefepinho@gmail.com>
 */

#include <stdio.h>
char *itoa(int n) {
  static char buf[6];
  char *p = buf + 5;
  *p = '\0';
  while (n > 0) {
    *(--p) = '0' + (n % 10);
    n /= 10;
  }
  return p;
}

int main(void)
{
  int mem_kb = call_mem();
  printf("Conventional Memory: ");
  printf(itoa(mem_kb));
  return 0;
}
