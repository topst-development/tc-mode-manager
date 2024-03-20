
/****************************************************************************************
 *   FileName    : ModeXMLParser.c
 *   Description : Mode XML Parser C File
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "TCLog.h"
#include "ModeXMLParser.h"
#include "ModeManager.h"


int32_t parseDoc(const char *docname)
{
	TCLog(TCLogLevelInfo, "%s\n", __FUNCTION__);
	int32_t ret = 0;
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);

	if(doc == NULL)
	{
		TCLog(TCLogLevelError, "[PARSER]Document not parsed successfully. \n");
		ret = -1;
	}
	if(ret != -1)
	{
		cur = xmlDocGetRootElement(doc);
		if(cur == NULL)
		{
			TCLog(TCLogLevelError, "[PARSER]empty document\n");
			xmlFreeDoc(doc);
			ret = -1;
		}
	}
	if(ret != -1)
	{
		if(xmlStrcmp((cur->name), (const xmlChar *)"policies") != 0)
		{
			TCLog(TCLogLevelError, "[PARSER]document of the wrong type, root node != policies");
			xmlFreeDoc(doc);
			ret = -1;
		}
	}
	if(ret != -1)
	{
		cur = cur->xmlChildrenNode;
		Mode configMode;
		xmlChar *key;
		while (cur != NULL)
		{
			(void)memset(&configMode, 0, sizeof(Mode));
			if (xmlStrcmp(cur->name, (const xmlChar *)"mode") == 0)
			{
				key = xmlGetProp(cur, (const xmlChar *)"name");
				if(key != NULL)
				{
					(void)strncpy(configMode.mode,(char*)key, (uint32_t)xmlStrlen(key));
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"app");
				if(key != NULL)
				{
					configMode.app = atoi((char*)key);
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"audio");
				if(key != NULL)
				{
					configMode.audio = atoi((char*)key);
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"display");
				if(key != NULL)
				{
					configMode.display = atoi((char*)key);
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"tuner");
				if(key != NULL)
				{
					configMode.tuner = atoi((char*)key);
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"full");
				if(key != NULL)
				{
					configMode.full = atoi((char*)key);
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"resume");
				if(key != NULL)
				{
					configMode.resume = atoi((char*)key);
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"mixing");
				if(key != NULL)
				{
					configMode.mixing = atoi((char*)key);
					xmlFree(key);
				}
				key = xmlGetProp(cur, (const xmlChar *)"exclusive");
				if(key != NULL)
				{
					configMode.exclusive = atoi((char*)key);
					xmlFree(key);
				}
				setModePolicy(configMode);
			    TCLog(TCLogLevelInfo, "[PARSER]mode: %s app: %d audio: %d display: %d  tuner : %d  full : %d resume: %d mixing: %d exclusive: %d\n",
						configMode.mode,
						configMode.app,
						configMode.audio,
						configMode.display,
						configMode.tuner,
						configMode.full,
						configMode.resume,
						configMode.mixing,
						configMode.exclusive);
			}
			cur = cur->next;
		}
	}
	xmlFreeDoc(doc);
	if(ret != 0)
	{
		TCLog(TCLogLevelError, "[PARSER]Config File Parsing Failed\n");
	}
	return ret;
}
