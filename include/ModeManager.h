
/****************************************************************************************
 *   FileName    : ModeManager.h
 *   Description : Mode Managering Header
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

#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t g_debug;

typedef struct
{
	char mode[128];
	int32_t app;
	int32_t audio;
	int32_t display;
	int32_t tuner;
	int32_t full;
	int32_t resume;
	int32_t mixing;
	int32_t exclusive;
} Mode;

typedef void (*ChangedMode_cb)(const char *mode, int32_t app);
typedef void (*ReleaseResource_cb)(int32_t resources, int32_t app);
typedef void (*EndedMode_cb)(const char *mode, int32_t app);
typedef void (*SuspendMode_cb)(void);
typedef void (*ResumeMode_cb)(void);

typedef struct _ModeManagerSignalCB {
	ChangedMode_cb			_ChangedMode;
	ReleaseResource_cb		_ReleaseResource;
	EndedMode_cb			_EndedMode;
	SuspendMode_cb			_SuspendMode;
	ResumeMode_cb			_ResumeMode;
} ModeManagerSignalCB;

int32_t ModeManagerInitiallize();
void ModeManagerRelease();

void setModeManagerSignalCB(ModeManagerSignalCB *cb);
void setModePolicy(Mode policy);
int32_t cmpModePriority(const char* mode, int32_t app);
void resumeMode(const char* mode, int32_t app);
void sendModeChanged(int32_t resources, int32_t app);
void systemSuspendMode();
void systemResumeMode();



#ifdef __cplusplus
}
#endif
#endif

