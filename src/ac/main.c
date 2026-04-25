/*
 * ============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  AC Controller server main entry point.
 *                  - Initializes JSON file database
 *                  - Loads configuration from UCI
 *                  - Starts IP pool manager
 *                  - Generates AC UUID
 *                  - Sets up AP hash table
 *                  - Starts message processing threads
 *                  - Initializes network layer (datalink + TCP)
 *                  - Runs as daemon
 *
 *        Version:  2.0
 *        Created:  2026-04-12
 *       Revision:  complete rewrite - daemon mode, signal handling, UCI config
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  OpenWrt AC Controller Project
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "net.h"
#include "log.h"
#include "msg.h"
#include "aphash.h"
#include "arg.h"
#include "netlayer.h"
#include "apstatus.h"
#include "link.h"
#include "process.h"
#include "resource.h"
#include "sec.h"
#include "dllayer.h"
#include "db.h"
#include "chap.h"
#include "thread.h"
