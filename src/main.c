
/****************************************************************************************
 *   FileName    : main.c
 *   Description : Main C File
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved

This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited
to re-distribution in source or binary form is strictly prohibited.
This source code is provided “AS IS” and nothing contained in this source code
shall constitute any express or implied warranty of any kind, including without limitation,
any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent,
copyright or other third party intellectual property right.
No warranty is made, express or implied, regarding the information’s accuracy,
completeness, or performance.
In no event shall Telechips be liable for any claim, damages or other liability arising from,
out of or in connection with this source code or the use in the source code.
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement
between Telechips and Company.
*
****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <systemd/sd-daemon.h>

#include "TCLog.h"
#include "ModeXMLParser.h"
#include "ModeDBusManager.h"
#include "ModeManager.h"

static GMainLoop *s_mainLoop = NULL;

const char *pid_file = "/var/run/TCModeManager.pid";

static void SignalHandler(int32_t sig)
{
	TCLog(TCLogLevelError, "Exit Application : %d", sig);
	if (s_mainLoop != NULL)
	{
		g_main_loop_quit(s_mainLoop);
	}
}

static void Daemonize(void)
{
	pid_t pid;
	FILE *pid_fp;

	pid = fork();

	// fork failed
	if (pid < 0)
	{
		TCLog(TCLogLevelError, "fork failed\n");
		exit(1);
	}

	// parent process
	if (pid > 0)
	{
    	//exit parent process for daemonize
 		exit(0);
	}

	// umask the file mode
	(void)umask(0);

	pid_fp = fopen(pid_file, "w+");
	if (pid_fp != NULL)
	{
		(void)fprintf(pid_fp, "%d\n", getpid());
		(void)fclose(pid_fp);
	}
	else
	{
		perror("pid file open failed");
	}

	// set new session
	if (setsid() < 0)
	{
		TCLog(TCLogLevelError, "set new session failed\n");
		exit(1);
	}

	// change the current working directory for safety
	if (chdir("/") < 0)
	{
		TCLog(TCLogLevelError, "change directory failed\n");
		exit(1);
	}
}

static void usage(void)
{
	TCLog(TCLogLevelInfo, "TCModeManager : Telechips mode managering daemon.\n");
	TCLog(TCLogLevelInfo, "Usage :  TCModeManager [OPTIONS]...\n");
	TCLog(TCLogLevelInfo, "--help : debug log on \n");
	TCLog(TCLogLevelInfo, "\t--debug : debug log on \n");
	TCLog(TCLogLevelInfo, "\t--no-daemon : Don't fork(default fork)\n");
	TCLog(TCLogLevelInfo, "\t--config-file=FILE : external mode config file(FILE: full file path)\n");
}

int32_t main(int32_t argc, char *argv[])
{
	int32_t ret = 0;
	int32_t index;
	char *configPath = NULL;
	int32_t s_daemonize = 1;

	TCLogInitialize("MODEMAN", NULL, 0);

	if (argc > 1)
	{
		for (index = 1; index < argc; index++)
		{
			if (strncmp(argv[index], "--debug", 7) == 0)
			{
				TCLogSetLevel(TCLogLevelDebug);
			}
			else if (strncmp(argv[index], "--no-daemon", 11) == 0)
			{
				s_daemonize = 0;
			}
			else if (strncmp(argv[index], "--config-file", 13) == 0)
			{
				configPath = argv[index+1];
			}
			else if (strncmp(argv[index], "--help", 6) == 0)
			{
				usage();
				ret = -1;
			}
			else
			{
				TCLog(TCLogLevelError, "Excuted Default Option \n");
			}
		}
	}
	if (s_daemonize == 1)
	{
		Daemonize();
	}

	(void)signal(SIGINT, SignalHandler);
	(void)signal(SIGTERM, SignalHandler);
	(void)signal(SIGABRT, SignalHandler);

	if(ret == 0)
	{
		s_mainLoop = g_main_loop_new(NULL, FALSE);

		if (s_mainLoop != NULL)
		{
			if(configPath != NULL)
			{
				ret = parseDoc((const char*)configPath);
			}
			else
			{
				ret = parseDoc("/usr/share/mode/defaultmode.xml");
			}
			if(ret == 0)
			{
				ModeDBusInitialize();
				(void)ModeManagerInitiallize();
				ModeManagerSignalCB cb;
				cb._ChangedMode = SendDBusChangedMode;
				cb._ReleaseResource = SendDBusReleaseResource;
				cb._EndedMode = SendDBusEndedMode;
				cb._SuspendMode = SendDBusSuspendMode;
				cb._ResumeMode = SendDBusResumeMode;
				setModeManagerSignalCB(&cb);

				(void)sd_notify(0, "READY=1");
				g_main_loop_run(s_mainLoop);
				g_main_loop_unref(s_mainLoop);
				s_mainLoop = NULL;

				ModeManagerRelease();
				ModeDBusRelease();
			}
		}
		else
		{
			TCLog(TCLogLevelError, "g_main_loop_new failed\n");
			ret = -1;
		}

		if (access(pid_file, F_OK) == 0)
		{
			if (unlink(pid_file) != 0)
			{
				perror("delete pid file failed");
			}
		}
	}
	return ret;
}


