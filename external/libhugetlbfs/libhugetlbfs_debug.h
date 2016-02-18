/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 IBM Corporation.
 * Author: Andy Whitcroft
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef _LIBHUGETLBFS_DEBUG_H
#define _LIBHUGETLBFS_DEBUG_H

/* Severe, unrecoverable errors */
#define ERROR(...)		REPORT(1, "ERROR", ##__VA_ARGS__)
#define ERROR_CONT(...)		REPORT_CONT(1, "ERROR", ##__VA_ARGS__)

/* A condition that is recoverable, but may result in altered semantics */
#define WARNING(...)		REPORT(2, "WARNING", ##__VA_ARGS__)
#define WARNING_CONT(...)	REPORT_CONT(2, "WARNING", ##__VA_ARGS__)

/* Detailed information about normal library operations */
#define INFO(...)		REPORT(3, "INFO", ##__VA_ARGS__)
#define INFO_CONT(...)		REPORT_CONT(3, "INFO", ##__VA_ARGS__)

/* Diagnostic information used for debugging problems */
#define DEBUG(...)		REPORT(4, "DEBUG", ##__VA_ARGS__)
#define DEBUG_CONT(...)		REPORT_CONT(4, "DEBUG", ##__VA_ARGS__)

#define VERBOSITY_MAX 4
#define VERBOSITY_DEFAULT 2

#endif
