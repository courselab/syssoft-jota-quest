/*
 *    SPDX-FileCopyrightText: 2025 jotaefepinho <jotaefepinho@gmail.com>
 *   
 *    SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  This file is a derivative of SYSeg (https://gitlab.com/monaco/syseg)
 *  and includes modifications made by the following author(s):
 *  jotaefepinho <jotaefepinho@gmail.com>
 */

#ifndef E8_H
#define E8_H

void __attribute__((fastcall, naked)) printf(const char *);
int __attribute__((fastcall, naked)) call_mem(void);

#endif	
