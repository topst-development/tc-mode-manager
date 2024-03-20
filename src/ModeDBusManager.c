
/****************************************************************************************
 *   FileName    : ModeDBusManager.c
 *   Description : Mode Dbus Manager C File
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

#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <dbus/dbus.h>

#include "TCLog.h"
#include "TCDBusRawAPI.h"
#include "DBusMsgDef.h"
#include "ModeDBusManager.h"
#include "ModeManager.h"

typedef void (*DBusMethodCallFunction)(DBusMessage *message);

static void DBusMethodChangeMode(DBusMessage *message);
static void DBusMethodReleaseResourceDone(DBusMessage *message);
static void DBusMethodEndMode(DBusMessage *message);
static void DBusMethodModeErrorOcuured(DBusMessage *message);
static void DBusMethodSuspend(DBusMessage *message);
static void DBusMethodResume(DBusMessage *message);
static DBusMsgErrorCode OnReceivedMethodCall(DBusMessage *message, const char *interface);


static DBusMethodCallFunction s_DBusMethodProcess[TotalMethodModeManagerEvent] = {
	DBusMethodChangeMode,
	DBusMethodReleaseResourceDone,
	DBusMethodEndMode,
	DBusMethodModeErrorOcuured,
	DBusMethodSuspend,
	DBusMethodResume,
};

void ModeDBusInitialize(void)
{
	SetDBusPrimaryOwner(MODEMANAGER_PROCESS_DBUS_NAME);
	SetCallBackFunctions(NULL, OnReceivedMethodCall);
	(void)AddMethodInterface(MODEMANAGER_EVENT_INTERFACE);
	InitializeRawDBusConnection("MODE MANAGER DBUS");
	TCLog(TCLogLevelInfo, "%s\n", __FUNCTION__);
}

void ModeDBusRelease(void)
{
	ReleaseRawDBusConnection();
	TCLog(TCLogLevelInfo, "%s\n", __FUNCTION__);
}

void SendDBusChangedMode(const char *mode, int32_t app)
{
	DBusMessage *message;
	const char *dbusMode = mode;
	int32_t dbusApp = app;

	message = CreateDBusMsgSignal(MODEMANAGER_PROCESS_OBJECT_PATH, MODEMANAGER_EVENT_INTERFACE,
								  g_signalModeManagerEventNames[ChangedMode],
								  DBUS_TYPE_STRING, &dbusMode,
								  DBUS_TYPE_INT32, &dbusApp,
								  DBUS_TYPE_INVALID);
	if(message != NULL)
	{
		if(SendDBusMessage(message, NULL) == 1)
		{
			TCLog(TCLogLevelDebug, "%s : %s, %d\n", __FUNCTION__, dbusMode, dbusApp);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: SendDBusMessage failed\n", __FUNCTION__);
		}
		dbus_message_unref(message);
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}

void SendDBusReleaseResource(int32_t resources, int32_t app)
{
	DBusMessage *message;
	int32_t dbusResources = resources;
	int32_t dbusApp = app;

	message = CreateDBusMsgSignal(MODEMANAGER_PROCESS_OBJECT_PATH, MODEMANAGER_EVENT_INTERFACE,
								  g_signalModeManagerEventNames[ReleaseResource],
								  DBUS_TYPE_INT32, &dbusResources,
								  DBUS_TYPE_INT32, &dbusApp,
								  DBUS_TYPE_INVALID);
	if(message != NULL)
	{
		if(SendDBusMessage(message, NULL) == 1)
		{
			TCLog(TCLogLevelDebug, "%s: %d, %d\n", __FUNCTION__, dbusResources, dbusApp);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: SendDBusMessage failed\n", __FUNCTION__);
		}
		dbus_message_unref(message);
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}
void SendDBusEndedMode(const char* mode, int32_t app)
{
	DBusMessage *message;
	const char* dbusMode = mode;
	int32_t dbusApp = app;

	message = CreateDBusMsgSignal(MODEMANAGER_PROCESS_OBJECT_PATH, MODEMANAGER_EVENT_INTERFACE,
								  g_signalModeManagerEventNames[EndedMode],
								  DBUS_TYPE_STRING, &dbusMode,
								  DBUS_TYPE_INT32, &dbusApp,
								  DBUS_TYPE_INVALID);
	if(message != NULL)
	{
		if(SendDBusMessage(message, NULL) == 1)
		{
			TCLog(TCLogLevelDebug, "%s: %s, %d\n", __FUNCTION__, dbusMode, dbusApp);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: SendDBusMessage failed\n", __FUNCTION__);
		}
		dbus_message_unref(message);
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}

void SendDBusSuspendMode(void)
{
	DBusMessage *message;

	message = CreateDBusMsgSignal(MODEMANAGER_PROCESS_OBJECT_PATH, MODEMANAGER_EVENT_INTERFACE,
								  g_signalModeManagerEventNames[SuspendMode],
								  DBUS_TYPE_INVALID);
	if(message != NULL)
	{
		if(SendDBusMessage(message, NULL) == 1)
		{
			TCLog(TCLogLevelDebug, "%s\n", __FUNCTION__);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: SendDBusMessage failed\n", __FUNCTION__);
		}
		dbus_message_unref(message);
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}

void SendDBusResumeMode(void)
{
	DBusMessage *message;

	message = CreateDBusMsgSignal(MODEMANAGER_PROCESS_OBJECT_PATH, MODEMANAGER_EVENT_INTERFACE,
								  g_signalModeManagerEventNames[ResumeMode],
								  DBUS_TYPE_INVALID);
	if(message != NULL)
	{
		if(SendDBusMessage(message, NULL) == 1)
		{
			TCLog(TCLogLevelDebug, "%s\n", __FUNCTION__);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: SendDBusMessage failed\n", __FUNCTION__);
		}
		dbus_message_unref(message);
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}

static DBusMsgErrorCode OnReceivedMethodCall(DBusMessage *message, const char *interface)
{
	DBusMsgErrorCode error = ErrorCodeNoError;
	uint32_t index;
	int32_t stop = 0;
	int32_t err = strncmp(interface, MODEMANAGER_EVENT_INTERFACE, 12);
	if ((interface != NULL) && (err == 0))
	{
		for(index = (uint32_t)ChangeMode; index < (uint32_t)TotalMethodModeManagerEvent && !stop; index++)
		{
			if(dbus_message_is_method_call(message, MODEMANAGER_EVENT_INTERFACE, g_methodModeManagerEventNames[index]) == (uint32_t)1)
			{
				s_DBusMethodProcess[index](message);
				stop = 1;
			}
		}
	}
	return error;
}

static void DBusMethodChangeMode(DBusMessage * message)
{
	if(message != NULL)
	{
		DBusMessage *returnMessage;
		const char* mode;
		int32_t app;
		int32_t retVal = 0;
		if(GetArgumentFromDBusMessage(message,
									  DBUS_TYPE_STRING, &mode,
									  DBUS_TYPE_INT32, &app,
									  DBUS_TYPE_INVALID) != 0)
		{
			TCLog(TCLogLevelDebug, "%s mode : %s, from : %d\n", __FUNCTION__, mode, app);
			retVal = cmpModePriority(mode, app);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: GetArgumentFromDBusMessage failed\n", __FUNCTION__);
		}
		returnMessage = CreateDBusMsgMethodReturn(message,
												  DBUS_TYPE_INT32, &retVal,
												  DBUS_TYPE_INVALID);
		if(returnMessage != NULL)
		{
			if(SendDBusMessage(returnMessage, NULL) != 1)
			{
				TCLog(TCLogLevelError, "%s: SendDBusMessage failed\n", __FUNCTION__);
			}
			dbus_message_unref(returnMessage);
		}
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}

static void DBusMethodEndMode(DBusMessage * message)
{
	if(message != NULL)
	{
		const char* mode;
		int32_t app;
		if(GetArgumentFromDBusMessage(message,
									  DBUS_TYPE_STRING, &mode,
									  DBUS_TYPE_INT32, &app,
									  DBUS_TYPE_INVALID) != 0)
		{
			TCLog(TCLogLevelDebug, "%s mode : %s, from : %d\n", __FUNCTION__, mode, app);
			resumeMode(mode, app);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: GetArgumentFromDBusMessage failed\n", __FUNCTION__);
		}
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}

static void DBusMethodReleaseResourceDone(DBusMessage * message)
{
	if(message != NULL)
	{
		int32_t resources;
		int32_t app;
		if(GetArgumentFromDBusMessage(message,
									  DBUS_TYPE_INT32, &resources,
									  DBUS_TYPE_INT32, &app,
									  DBUS_TYPE_INVALID) != 0)
		{
			TCLog(TCLogLevelDebug, "%s resources : %d, to : %d\n", __FUNCTION__, resources, app);
			sendModeChanged(resources, app);
		}
		else
		{
			TCLog(TCLogLevelError, "%s: GetArgumentFromDBusMessage failed\n", __FUNCTION__);
		}
	}
	else
	{
		TCLog(TCLogLevelError, "%s: message is NULL\n", __FUNCTION__);
	}
}
static void DBusMethodModeErrorOcuured(DBusMessage * message)
{
	(void)message;
}
static void DBusMethodSuspend(DBusMessage * message)
{
	TCLog(TCLogLevelDebug, "%s \n", __FUNCTION__);
	(void)message;
	systemSuspendMode();
}
static void DBusMethodResume(DBusMessage * message)
{
	TCLog(TCLogLevelDebug, "%s \n", __FUNCTION__);
	(void)message;
	systemResumeMode();
}
