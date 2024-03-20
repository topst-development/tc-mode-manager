
/****************************************************************************************
 *   FileName    : DBusMsgDef.h
 *   Description : DBUS Message Define Header
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

#ifndef DBUSMSGDEF_H
#define DBUSMSGDEF_H

#define MODEMANAGER_PROCESS_DBUS_NAME					"telechips.mode.manager"
#define MODEMANAGER_PROCESS_OBJECT_PATH					"/telechips/mode/manager"
#define MODEMANAGER_EVENT_INTERFACE						"mode.manager"

/********************************METHOD*************************************************/
#define CHANGE_MODE										"change_mode"
#define RELEASE_RESOURCE_DONE							"release_resource_done"
#define END_MODE										"end_mode"
#define MODE_ERROR_OCCURED								"mode_error_occured"
#define SUSPEND											"suspend"
#define RESUME											"resume"

typedef enum{
	ChangeMode,
	ReleaseResourceDone,
	EndMode,
	ModeErrorOccured,
	Suspend,
	Resume,
	TotalMethodModeManagerEvent
}MethodModeManagerEvent;
extern const char* g_methodModeManagerEventNames[TotalMethodModeManagerEvent];

/********************************SIGNAL*************************************************/
#define CHANGED_MODE									"changed_mode"
#define RELEASE_RESOURCE								"release_resource"
#define ENDED_MODE										"ended_mode"
#define SUSPEND_MODE									"suspend_mode"
#define RESUME_MODE										"resume_mode"

typedef enum{
	ChangedMode,
	ReleaseResource,
	EndedMode,
	SuspendMode,
	ResumeMode,
	TotalSignalModeManagerEvent
}SignalModeManagerEvent;
extern const char* g_signalModeManagerEventNames[TotalSignalModeManagerEvent];

#endif
