
/****************************************************************************************
 *   FileName    : ModeManager.c
 *   Description : Mode Managering C File
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

#include <cstdio>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <iterator>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include "TCLog.h"
#include "ModeManager.h"

#define RELEASENONE 		0x0000
#define RELEASEDISPLAY		0x0001
#define RELEASEAUDIO  		0x0002
#define RELEASETUNER  		0x0010

#define DEFAULTMODE			"home"
#define DEFAULTAPP			0
#define OSDAPP				100

typedef struct
{
	std::string mode;
	int32_t app;
	int32_t audio;
	int32_t display;
	int32_t tuner;
	int32_t full;
	int32_t resume;
	int32_t mixing;
	int32_t exclusive;
	int32_t state;
} Resource;

typedef struct
{
	int32_t app;
	int32_t resource;
} ReleaseApp;

bool operator==(Resource a, Resource b)
{
	bool ret;
	ret = (a.mode.compare(b.mode) == 0) && (a.app == b.app);
	return ret;
}

std::vector<Mode> _policy;
std::vector<Resource> _audio;
std::vector<Resource> _display;
std::vector<Resource> _tuner;
std::vector<ReleaseApp> _relAppList;

static ChangedMode_cb		_ChangedMode = NULL;
static ReleaseResource_cb	_ReleaseResource = NULL;
static EndedMode_cb			_EndedMode = NULL;
static SuspendMode_cb		_SuspendMode = NULL;
static ResumeMode_cb		_ResumeMode = NULL;

Resource _cmdMode;
static pthread_mutex_t _cmdMutex;
static pthread_mutex_t *_cmdMutexPtr = NULL;
static bool _modemanagerStatus = false;
pthread_t _modemanagerThread;

static void ModeAllResourcePrint();
static void ModeResume();
static void ModeShutdown();
static void ModeClearcmd();
static void *ModeManagerThread(void *arg);
static bool ModeCompareAudio(Resource mode);
static bool ModeCompareDisplay(Resource mode);
static bool ModeCompareTuner(Resource mode);
static bool ModeExclusiveCheck(Resource mode);
static void ModeManagerResources();
static void ModeChangeBackGround(void);
static void ModeRestoreBackGround(void);
static void ModeSendReleaseResource();
static Resource ModeFindwithinPolicy(const char* mode, int32_t app);
static void AddReleaseResources(int32_t app, int32_t resource);
static void RemoveReleaseResources(int32_t app, int32_t resource);


int32_t ModeManagerInitiallize()
{
	TCLog(TCLogLevelInfo, "%s\n", __FUNCTION__);
	int32_t err = 0;
	int32_t ret = 0;

	err = pthread_mutex_init(&_cmdMutex, NULL);
	_cmdMutexPtr = &_cmdMutex;
	if(err == 0)
	{
		ret = 1;
	}
	else
	{
		(void)fprintf(stderr, "pthread_mutex_init failed \n");
	}
	_modemanagerStatus = true;
	err = pthread_create(&_modemanagerThread, NULL, ModeManagerThread, NULL);
	if(err != 0)
	{
		_modemanagerStatus = false;
		ret = 0;
	}
	return ret;
}

void ModeManagerRelease()
{
	TCLog(TCLogLevelInfo, "%s\n", __FUNCTION__);
	if(_modemanagerStatus)
	{
		void *res;
		int32_t err;
		_modemanagerStatus = false;
		err = pthread_join(_modemanagerThread, &res);
		if(err != 0)
		{
			(void)fprintf(stderr, "pthread_join failed \n");
		}
	}

	if(_cmdMutexPtr != NULL)
	{
		int32_t err;
		err = pthread_mutex_destroy(_cmdMutexPtr);
		if(err != 0)
		{
			(void)fprintf(stderr, "pthread_mutex_destroy failed \n");
		}
	}
}

void setModeManagerSignalCB(ModeManagerSignalCB *cb)
{
	if(cb != NULL)
	{
		_ChangedMode = cb->_ChangedMode;
		_ReleaseResource = cb->_ReleaseResource;
		_EndedMode = cb->_EndedMode;
		_SuspendMode = cb->_SuspendMode;
		_ResumeMode = cb->_ResumeMode;
	}
}

void setModePolicy(Mode policy)
{
	_policy.push_back(policy);
}

int32_t cmpModePriority(const char* mode, int32_t app)
{
	int32_t ret = 0;
	Resource compare;
	bool audio = true;
	bool display = true;
	bool tuner = true;
	bool exclusive = true;

	if(strncmp(mode, "idle", 4) == 0)
	{
		bool idle = false;
		std::vector<Resource>::iterator iter;
		for(iter = _audio.begin(); iter != _audio.end(); ++iter)
		{
			if(iter->app == app)
			{
				idle = true;
				break;
			}
		}

		for(iter = _display.begin(); iter != _display.end(); ++iter)
		{
			if(iter->app == app)
			{
				idle = true;
				break;
			}
		}

		if(idle)
		{
			compare = ModeFindwithinPolicy(mode, -1);
			compare.app = app;
			compare.state = 2; /* idle mode */
		}
		else
		{
			TCLog(TCLogLevelWarn, "%s : This App(%s) is not in Mode Lists\n", __FUNCTION__, mode);
		}
	}
	else
	{
		compare = ModeFindwithinPolicy(mode, app);
		compare.state = 0; /* managering mode */
	}
	if(compare.exclusive != 0)
	{
		exclusive = ModeExclusiveCheck(compare);
	}
	if(exclusive && compare.mode.empty() == false)
	{
		if(compare.audio)
		{
			audio = ModeCompareAudio(compare);
		}
		if(compare.display)
		{
			display = ModeCompareDisplay(compare);
		}
		if(compare.tuner)
		{
			tuner = ModeCompareTuner(compare);
		}
		if(compare.audio && audio == true && display == false)
		{
			std::string bgMode = compare.mode;
			bgMode.append("bg");
			compare = ModeFindwithinPolicy(bgMode.c_str(), compare.app);
			compare.state = 0; /* managering mode */
			display = true;
		}
		if(audio && display && tuner)
		{
			pthread_mutex_lock(&_cmdMutex);
			_cmdMode = compare;
			pthread_mutex_unlock(&_cmdMutex);
			ret = 1;
		}
	}
	if(compare.mode.empty() == false)
	{
		TCLog(TCLogLevelInfo, "%s %s, %d result : %d\n", __FUNCTION__, compare.mode.c_str(), compare.app, ret);
	}
	return ret;
}

void resumeMode(const char* mode, int32_t app)
{
	bool end = false;
	std::vector<Resource>::iterator iter;
	std::string tmpMode = mode;
	std::string bgMode = mode;
	bgMode.append("bg");
	for(iter = _audio.begin(); iter != _audio.end(); ++iter)
	{
		if(iter->mode == mode)
		{
			end = true;
			break;
		}
		else if(iter->mode == bgMode)
		{
			end = true;
			tmpMode.append("bg");
			break;
		}
	}

	for(iter = _display.begin(); iter != _display.end(); ++iter)
	{
		if(iter->mode == mode)
		{
			end = true;
			break;
		}
	}
	if(end)
	{
		_EndedMode(mode, app);
		Resource resume;
		resume = ModeFindwithinPolicy(tmpMode.c_str(), app);
		resume.state = 1;
		pthread_mutex_lock(&_cmdMutex);
		_cmdMode = resume;
		pthread_mutex_unlock(&_cmdMutex);
	}
	else
	{
		TCLog(TCLogLevelWarn, "%s This Mode is not in Resource Lists\n", __FUNCTION__);
	}
}

void sendModeChanged(int32_t resources, int32_t app)
{
	RemoveReleaseResources(app, resources);

	if(_relAppList.empty())
	{
		if(resources & RELEASEDISPLAY)
		{
			if(!_display.empty())
			{
				if(_display.back().full == 0)
				{
					_ChangedMode("view", OSDAPP);
				}
				_ChangedMode(_display.back().mode.c_str(), _display.back().app);
			}
		}
		else if(resources & RELEASEAUDIO)
		{
			if(!_audio.empty())
			{
				if(_audio.back().display != 0 && _audio.back().full == 0)
				{
					_ChangedMode("view", OSDAPP);
				}
				_ChangedMode(_audio.back().mode.c_str(), _audio.back().app);
				if(_audio.back().app != _display.back().app)
				{
					_ChangedMode(_display.back().mode.c_str(), _display.back().app);
				}
			}
		}
		else if(resources & RELEASETUNER)
		{
			if(!_tuner.empty())
			{
				if(_tuner.back().display != 0 && _tuner.back().full == 0)
				{
					_ChangedMode("view", OSDAPP);
				}
				_ChangedMode(_tuner.back().mode.c_str(), _tuner.back().app);
			}
		}
	}
}

void systemSuspendMode()
{
	ModeClearcmd();
	_relAppList.clear();
	if(_display.back().full == 0)
	{
		_ReleaseResource(RELEASEDISPLAY, OSDAPP);
	}
	_display.clear();
	_audio.clear();
	_tuner.clear();
	_SuspendMode();
}

void systemResumeMode()
{
	_ResumeMode();
}

static void ModeAllResourcePrint()
{
	std::vector<Resource>::iterator iter;
	Resource res;

	TCLog(TCLogLevelDebug, " Audio Resource\n");
	for(iter = _audio.begin(); iter != _audio.end(); ++iter)
	{
		res = *iter;
		TCLog(TCLogLevelDebug, "  %s : %d \n", res.mode.c_str(), res.app);
	}

	TCLog(TCLogLevelDebug, " Display Resource\n");
	for(iter = _display.begin(); iter != _display.end(); ++iter)
	{
		res = *iter;
		TCLog(TCLogLevelDebug,"  %s : %d \n", res.mode.c_str(), res.app);
	}

	TCLog(TCLogLevelDebug, " Tuner Resource\n");
	for(iter = _tuner.begin(); iter != _tuner.end(); ++iter)
	{
		res = *iter;
		TCLog(TCLogLevelDebug, "  %s : %d \n", res.mode.c_str(), res.app);
	}

	std::vector<ReleaseApp>::iterator appiter;
	ReleaseApp relApps;

	TCLog(TCLogLevelDebug, " Release Apps\n");
	for(appiter = _relAppList.begin(); appiter != _relAppList.end(); ++appiter)
	{
		relApps = *appiter;
		TCLog(TCLogLevelDebug, "appID - %d resources : 0x%x \n", relApps.app, relApps.resource);
	}
}

static void ModeResume()
{
	TCLog(TCLogLevelDebug, "End mode : %s\n", _cmdMode.mode.c_str());
	std::vector<Resource>::iterator iter;
	bool resumeAudio = false, resumeDisplay = false, insertHome = false;
	for(iter = _audio.begin(); iter != _audio.end(); ++iter)
	{
		if(*iter == _cmdMode)
		{
			if(*iter == _audio.back())
			{
				if(_audio.size() > 1)
				{
					resumeAudio = true;
				}
			}
			_audio.erase(iter);
			break;
		}
	}
	for(iter = _display.begin(); iter != _display.end(); ++iter)
	{
		if(*iter == _cmdMode)
		{
			if(*iter == _display.back())
			{
				if(_display.size() > 1)
				{
					resumeDisplay = true;
				}
				else
				{
					resumeDisplay = false;
				}
			}
			if(*iter == _display.front())
			{
				insertHome = true;
			}
			_display.erase(iter);
			break;
		}
	}
	if(insertHome)
	{
		Resource defmode;
		if(_display.empty())
		{
			resumeDisplay = true;
		}
		defmode = ModeFindwithinPolicy(DEFAULTMODE, DEFAULTAPP);
		_display.insert(_display.begin(), defmode);
	}
	ModeChangeBackGround();
	ModeRestoreBackGround();

	if(resumeAudio && resumeDisplay)
	{
		if(_audio.back().app != _display.back().app)
		{
			_ChangedMode(_audio.back().mode.c_str(), _audio.back().app);
			if(_display.back().full == 0)
			{
				_ChangedMode("view", OSDAPP);
			}
			else
			{
				_ReleaseResource(RELEASEDISPLAY, OSDAPP);
			}
			_ChangedMode(_display.back().mode.c_str(), _display.back().app);
		}
		else
		{
			if(_display.back().full == 0)
			{
				_ChangedMode("view", OSDAPP);
			}
			else
			{
				_ReleaseResource(RELEASEDISPLAY, OSDAPP);
			}
			_ChangedMode(_display.back().mode.c_str(), _display.back().app);
		}
	}
	else if(resumeAudio && resumeDisplay == false)
	{
		_ChangedMode(_audio.back().mode.c_str(), _audio.back().app);
	}
	else if(resumeDisplay && resumeAudio == false)
	{
		if(_display.back().full == 0)
		{
			_ChangedMode("view", OSDAPP);
		}
		else
		{
			_ReleaseResource(RELEASEDISPLAY, OSDAPP);
		}
		_ChangedMode(_display.back().mode.c_str(), _display.back().app);
	}
	ModeAllResourcePrint();

	ModeClearcmd();
	_relAppList.clear();
}

static void ModeShutdown()
{
	TCLog(TCLogLevelDebug, "Shutdown app : %d\n", _cmdMode.app);
	std::vector<Resource>::iterator iter;
	bool resumeAudio = false, resumeDisplay = false, insertHome = false;

	for(iter = _audio.begin(); iter != _audio.end();)
	{
		if(iter->app == _cmdMode.app)
		{
			if(_audio.size() > 1 && *iter == _audio.back())
			{
				resumeAudio = true;
			}
			else
			{
				resumeAudio = false;
			}
			_audio.erase(iter);
		}
		else
		{
			++iter;
		}
	}
	for(iter = _display.begin(); iter != _display.end();)
	{
		if(iter->app == _cmdMode.app)
		{
			if(_display.size() > 1 && *iter == _display.back())
			{
				resumeDisplay = true;
			}
			else
			{
				resumeDisplay = false;
			}
			if(*iter == _display.front())
			{
				insertHome = true;
			}
			_display.erase(iter);
		}
		else
		{
			++iter;
		}
	}
	for(iter = _tuner.begin(); iter != _tuner.end();)
	{
		if(iter->app == _cmdMode.app)
		{
			_tuner.erase(iter);
		}
		else
		{
			++iter;
		}
	}
	if(insertHome)
	{
		Resource defmode;
		if(_display.empty())
		{
			resumeDisplay = true;
		}
		defmode = ModeFindwithinPolicy("home", 0);
		_display.insert(_display.begin(), defmode);
	}
	ModeChangeBackGround();
	ModeRestoreBackGround();
	if(resumeAudio && resumeDisplay)
	{
		if(_audio.back().app != _display.back().app)
		{
			_ChangedMode(_audio.back().mode.c_str(), _audio.back().app);
			if(_display.back().full == 0)
			{
				_ChangedMode("view", OSDAPP);
			}
			else
			{
				_ReleaseResource(RELEASEDISPLAY, OSDAPP);
			}
			_ChangedMode(_display.back().mode.c_str(), _display.back().app);
		}
		else
		{
			if(_display.back().full == 0)
			{
				_ChangedMode("view", OSDAPP);
			}
			else
			{
				_ReleaseResource(RELEASEDISPLAY, OSDAPP);
			}
			_ChangedMode(_display.back().mode.c_str(), _display.back().app);
		}
	}
	else if(resumeAudio)
	{
		_ChangedMode(_audio.back().mode.c_str(), _audio.back().app);
	}
	else if(resumeDisplay)
	{
		if(_display.back().full == 0)
		{
			_ChangedMode("view", OSDAPP);
		}
		else
		{
			_ReleaseResource(RELEASEDISPLAY, OSDAPP);
		}
		_ChangedMode(_display.back().mode.c_str(), _display.back().app);
	}

	ModeAllResourcePrint();

	ModeClearcmd();
	_relAppList.clear();
}

static void ModeClearcmd()
{
	_cmdMode.mode.clear();
	_cmdMode.app = -1;
	_cmdMode.audio = -1;
	_cmdMode.display = -1;
	_cmdMode.resume = -1;
	_cmdMode.mixing = -1;
}

static void *ModeManagerThread(void *arg)
{
	while(_modemanagerStatus)
	{
		usleep(1000);

		pthread_mutex_lock(&_cmdMutex);
		if(!_cmdMode.mode.empty())
		{
			if(_cmdMode.state == 0)
			{
				ModeManagerResources();
			}
			else if(_cmdMode.state == 1)
			{
				ModeResume();
			}
			else if(_cmdMode.state == 2)
			{
				ModeShutdown();
			}
			else
			{
				TCLog(TCLogLevelDebug, "ModeManager Waiting\n");
			}
		}
		pthread_mutex_unlock(&_cmdMutex);
	}
	pthread_exit((void *)"Mode Manager thread exit\n");
}

static bool ModeCompareAudio(Resource mode)
{
	bool ret = true;
	std::vector<Resource>::iterator iter;
	for(iter = _audio.begin(); iter != _audio.end(); ++iter)
	{
		if(*iter == mode)
		{
			ret = false;
			break;
		}
	}
	if(ret)
	{
		if(!_audio.empty())
		{
			if(_audio.back().audio <= mode.audio)
			{
				if(!mode.mixing)
				{
					std::vector<Resource>::reverse_iterator riter;
					for(riter = _audio.rbegin(); riter != _audio.rend(); ++riter)
					{
						if(riter->mixing)
						{
							if(riter->app != mode.app && riter->audio < mode.audio)
							{
								AddReleaseResources(riter->app, RELEASEAUDIO);
							}
						}
						else
						{
							if(riter->app != mode.app)
							{
								AddReleaseResources(riter->app, RELEASEAUDIO);
								break;
							}
						}
					}
				}
			}
			else
			{
				ret = false;
			}
		}
	}
	TCLog(TCLogLevelDebug, "%s : %d\n", __FUNCTION__, ret);
	return ret;
}

static bool ModeCompareDisplay(Resource mode)
{
	bool ret = true;
	int32_t releaseDisplay = -1;
	if(!_display.empty())
	{
		if(mode == _display.back())
		{
			ret = false;
		}
	}
	if(ret)
	{
		if(_display.empty() || _display.back().display <= mode.display)
		{
			if(!_display.empty())
			{
				AddReleaseResources(_display.back().app, RELEASEDISPLAY);
				releaseDisplay = _display.back().app;
			}
			if(releaseDisplay == mode.app)
			{
				RemoveReleaseResources(_display.back().app, RELEASEDISPLAY);
			}
		}
		else
		{
			ret = false;
		}
	}
	TCLog(TCLogLevelDebug, "%s : %d\n", __FUNCTION__, ret);
	return ret;
}

static bool ModeCompareTuner(Resource mode)
{
	bool ret = true;
	int32_t releaseTuner = -1;
	if(ret)
	{
		if(_tuner.empty() || _tuner.back().tuner <= mode.tuner)
		{
			if(!_tuner.empty())
			{
				AddReleaseResources(_tuner.back().app, RELEASETUNER);
				releaseTuner = _tuner.back().app;
			}
			if(releaseTuner == mode.app)
			{
				RemoveReleaseResources(_tuner.back().app, RELEASETUNER);
			}
		}
		else
		{
			ret = false;
		}
	}
	TCLog(TCLogLevelDebug, "%s : %d\n", __FUNCTION__, ret);
	return ret;

}

static bool ModeExclusiveCheck(Resource mode)
{
	bool ret = true;
	std::vector<Resource>::iterator iter;
	if(!_audio.empty())
	{
		for(iter =_audio.begin(); iter != _audio.end(); ++iter)
		{
			if(iter->exclusive == mode.exclusive)
			{
				ret = false;
				break;
			}
		}
	}
	if(!_display.empty())
	{
		for(iter = _display.begin(); iter != _display.end(); ++iter)
		{
			if(iter->exclusive == mode.exclusive)
			{
				ret = false;
				break;
			}
		}
	}
	TCLog(TCLogLevelDebug, "%s : %d\n", __FUNCTION__, ret);
	return ret;
}

static void ModeManagerResources()
{
	TCLog(TCLogLevelDebug, "%s\n", __FUNCTION__);
	if(_cmdMode.resume)
	{
		if(_cmdMode.audio)
		{
			_audio.push_back(_cmdMode);
		}
		if(_cmdMode.display)
		{
			_display.push_back(_cmdMode);
		}
		if(_cmdMode.tuner)
		{
			_tuner.push_back(_cmdMode);
		}
	}
	else
	{
		if(_cmdMode.audio)
		{
			if(!_audio.empty())
			{
				if(_audio.back().mixing)
				{
					std::vector<Resource>::reverse_iterator riter;
					for(riter = _audio.rbegin(); riter != _audio.rend();)
					{
						if(riter->mixing)
						{
							++riter;
						}
						else
						{
							_audio.erase(--riter.base());
						}
					}
					_audio.insert(_audio.begin(), _cmdMode);
				}
				else
				{
					_audio.clear();
					_audio.push_back(_cmdMode);
				}
			}
			else
			{
				_audio.push_back(_cmdMode);
			}
		}
		if(_cmdMode.display)
		{
			_display.clear();
			_display.push_back(_cmdMode);
		}
		if(_cmdMode.tuner)
		{
			_tuner.clear();
			_tuner.push_back(_cmdMode);
		}
	}
	ModeChangeBackGround();
	ModeRestoreBackGround();
	ModeSendReleaseResource();
	ModeAllResourcePrint();
	ModeClearcmd();
}

static void ModeChangeBackGround(void)
{
	if(!_audio.empty() && !_display.empty())
	{
		int32_t audioPriority = _audio.back().audio;
		int32_t audioIdx = _audio.size() - 1;
		std::vector<Resource>::reverse_iterator riter;
		for(riter = _audio.rbegin(); riter != _audio.rend(); ++riter)
		{
			if(riter->audio < audioPriority)
			{
				break;
			}
			else
			{
				audioPriority = riter->audio;
				if(riter->mode.find("bg") != -1)
				{
					TCLog(TCLogLevelDebug, "This Mode is already Background\n");
				}
				else
				{
					if(riter->app != _display.back().app && riter->display)
					{
						Resource tmpMode;
						std::string tmpstring = riter->mode;
						tmpstring.append("bg");
						tmpMode = ModeFindwithinPolicy(tmpstring.c_str(), riter->app);
						if(!tmpMode.mode.empty())
						{
							_audio.erase(--riter.base());
							_audio.insert(_audio.begin() + audioIdx, tmpMode);
							_ChangedMode(tmpMode.mode.c_str(), tmpMode.app);
							TCLog(TCLogLevelDebug, "This Mode changed to Background\n");
						}
						else
						{
							AddReleaseResources(riter->app, RELEASEAUDIO);
							if(!_display.back().resume)
							{
								_audio.erase(--riter.base());
							}
							TCLog(TCLogLevelDebug, "This Mode is not exist Background\n");
						}
					}
				}
				audioIdx--;
			}
		}
	}
}

static void ModeRestoreBackGround(void)
{
	if(!_audio.empty() && !_display.empty())
	{
		int32_t audioIdx = 0;
		std::vector<Resource>::iterator iter;
		for(iter = _audio.begin(); iter != _audio.end(); ++iter)
		{
			if(iter->mode.find("bg") != -1)
			{
				if(iter->app == _display.back().app)
				{
					if(iter->audio >= _audio.back().audio)
					{
						Resource tmpMode;
						iter->mode.erase(iter->mode.end()-2, iter->mode.end());
						tmpMode = ModeFindwithinPolicy(iter->mode.c_str(), iter->app);
						if(iter->resume == 0)
						{
							_display.pop_back();
						}
						_audio.erase(iter);
						_display.push_back(tmpMode);
						_audio.insert(_audio.begin() + audioIdx, tmpMode);
						TCLog(TCLogLevelDebug, "This Mode is restored\n");
					}
				}
			}
			audioIdx++;
		}
	}
}

static void ModeSendReleaseResource()
{
	if(!_relAppList.empty())
	{
		if(_cmdMode.full == 1)
		{
			_ReleaseResource(RELEASEDISPLAY, OSDAPP);
		}
		std::vector<ReleaseApp>::iterator iter;
		for(iter = _relAppList.begin(); iter != _relAppList.end(); ++iter)
		{
			_ReleaseResource(iter->resource, iter->app);
		}
	}
	else
	{
		if(_cmdMode.full == 1)
		{
			_ReleaseResource(RELEASEDISPLAY, OSDAPP);
		}
		else if(_cmdMode.full == 0 && _cmdMode.display != 0)
		{
			_ChangedMode("view", OSDAPP);
		}
		_ChangedMode(_cmdMode.mode.c_str(), _cmdMode.app);
	}
}

static Resource ModeFindwithinPolicy(const char* mode, int32_t app)
{
	std::vector<Mode>::iterator iter;
	Resource tmpResource;
	for(iter = _policy.begin(); iter != _policy.end(); ++iter)
	{
		if((iter->app == app) && (strcmp(iter->mode, mode) == 0))
		{
			tmpResource.mode = iter->mode;
			tmpResource.app = iter->app;
			tmpResource.audio = iter->audio;
			tmpResource.display = iter->display;
			tmpResource.tuner = iter->tuner;
			tmpResource.full = iter->full;
			tmpResource.resume = iter->resume;
			tmpResource.mixing = iter->mixing;
			tmpResource.exclusive = iter->exclusive;
			TCLog(TCLogLevelDebug, "mode: %s app: %d A=%d,D=%d,T=%d,F=%d,R=%d,M=%d,E=%d\n",
					tmpResource.mode.c_str(),
					tmpResource.app,
					tmpResource.audio,
					tmpResource.display,
					tmpResource.tuner,
					tmpResource.full,
					tmpResource.resume,
					tmpResource.mixing,
					tmpResource.exclusive);
			break;
		}
	}
	return tmpResource;
}

static void AddReleaseResources(int32_t app, int32_t resource)
{
	TCLog(TCLogLevelDebug, "%s : App(%d) Resource(%d)\n", __FUNCTION__, app, resource);
	ReleaseApp relApp;
	relApp.app = app;
	relApp.resource = resource;

	if(!_relAppList.empty())
	{
		std::vector<ReleaseApp>::iterator iter;
		for(iter = _relAppList.begin(); iter != _relAppList.end(); ++iter)
		{
			if(iter->app == relApp.app)
			{
				iter->resource |= relApp.resource;
				break;
			}
		}
		if(iter == _relAppList.end())
		{
			_relAppList.push_back(relApp);
		}

	}
	else
	{
		_relAppList.push_back(relApp);
	}
}

static void RemoveReleaseResources(int32_t app, int32_t resource)
{
	if(!_relAppList.empty())
	{
		TCLog(TCLogLevelDebug, "%s : App(%d) Resource(%d)\n", __FUNCTION__, app, resource);
		ReleaseApp relApp;
		relApp.app = app;
		relApp.resource = resource;

		std::vector<ReleaseApp>::iterator iter;
		for(iter = _relAppList.begin(); iter != _relAppList.end(); ++iter)
		{
			if(iter->app == relApp.app)
			{
				relApp.resource &= iter->resource;
				iter->resource ^= relApp.resource;
				if(!iter->resource)
				{
					_relAppList.erase(iter);
				}
				break;
			}
		}
	}
}
