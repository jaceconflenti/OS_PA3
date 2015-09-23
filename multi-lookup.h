/*
 * File: multi-lookup.h
 * Authors: Cameron Taylor and Steven Conflenti
 * Project: CSCI 3753 Programming Assignment 3
 * Date: 3/13/2015
 * Description: This file contains declarations of utility functions for
 *     the threaded DNS Name Resolution Engine.
 *  
 */
 
#ifndef MULTI_H
#define MULTI_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#include "queue.h"
#include "util.h"

void* requestDNS(void* file);
void* resolveDNS(void* output);

#endif
