/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stephen Mak <smak@sun.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * npshell.c
 *
 * Netscape Client Plugin API
 * - Function that need to be implemented by plugin developers
 *
 * This file defines a "shell" plugin that plugin developers can use
 * as the basis for a real plugin.  This shell just provides empty
 * implementations of all functions that the plugin can implement
 * that will be called by Netscape (the NPP_xxx methods defined in 
 * npapi.h). 
 *
 * dp Suresh <dp@netscape.com>
 * updated 5/1998 <pollmann@netscape.com>
 * updated 9/2000 <smak@sun.com>
 *
 */


/*
The contents of this file are subject to the Mozilla Public License

Version 1.1 (the "License"); you may not use this file except in compliance 
with the License. You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis, 
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for 
the specific language governing rights and limitations under the License.

The Original Code is stub code that defines the binary interface to a Mozilla
plugin.

The Initial Developer of the Original Code is Mozilla.

Portions created by Adobe Systems Incorporated are Copyright (C) 2007. All Rights Reserved.

Contributor(s): Adobe Systems Incorporated.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include "npapi.h"
#include "strings.h"

#include "oppdf.h"

/***********************************************************************
 *
 * Implementations of plugin API functions
 *
 ***********************************************************************/

char*
NPP_GetMIMEDescription(void)
{
	//printf("NPP_GetMIMEDescription()\n");
	return MIME_TYPES_HANDLED;
}

NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
	NPError	err = NPERR_NO_ERROR;

	printf("NPP_GetValue(%d)\n", variable);
	switch (variable) {
		case NPPVpluginNameString:
			*((char **)value) = PLUGIN_NAME;
			break;
		case NPPVpluginDescriptionString:
			*((char **)value) = PLUGIN_DESCRIPTION;
			break;
		case NPPVpluginNeedsXEmbed:
			*((PRBool *)value) = PR_TRUE;
			break;
		default:
			printf("NPP_GetValue: not handled variable %d\n", variable);
			err = NPERR_GENERIC_ERROR;
	}
	return err;
}

NPError
NPP_Initialize(void)
{
	//printf("NPP_Initialize()\n");
	return NPERR_NO_ERROR;
}

#ifdef OJI
jref
NPP_GetJavaClass()
{
	return NULL;
}
#endif

void
NPP_Shutdown(void)
{
	//printf("NPP_Shutdown()\n");
}

NPError 
NPP_New(NPMIMEType pluginType, NPP instance,
		uint16 mode,
		int16 argc, char* argn[], char* argv[],
		NPSavedData* saved)
{
	PluginInstance	*This;
	int		xembedSupported = 0;

	//printf("NPP_New()\n");

	/* if the browser does not support XEmbed, let it down easy at this point */
	NPN_GetValue(instance, NPNVSupportsXEmbedBool, &xembedSupported);
	if (!xembedSupported)
	{
		printf("OPPDF: XEmbed not supported\n");
		return NPERR_GENERIC_ERROR;
	}

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));

	This = (PluginInstance*) instance->pdata;
	if (This == NULL)
		return NPERR_OUT_OF_MEMORY_ERROR;

	memset(This, 0, sizeof(PluginInstance));

	/* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
	This->mode = mode;
	This->instance = instance;
	This->buf = NULL;
	This->bufsize = 0;
	This->len = 0;

	/* override even if we are given otherwise: windowed, non-transparent */
	NPN_SetValue(instance, NPPVpluginWindowBool, (void*)TRUE);
	NPN_SetValue(instance, NPPVpluginTransparentBool, (void*)FALSE);

	return NPERR_NO_ERROR;
}

static void
free_buffer(PluginInstance *This) {
	if (This->buf == NULL)
		return;

	NPN_MemFree(This->buf);
	This->buf = NULL;
	This->bufsize = 0;
	This->len = 0;
}

NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
	PluginInstance	*This;

	//printf("NPP_Destroy()\n");

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	if (This != NULL)
	{
		int	status;

		//printf("NPP_Destroy This->pid %d\n", This->pid);
		/*
		 * If somehow This->pid ended up as (pid_t)-1
		 * then we shouldn't kill the whole system or desktop
		 */
		if (This->pid != (pid_t)-1) {
			/* stop the child */
			kill(This->pid, SIGINT);
			/* wait until the child really finished */
			waitpid(This->pid, &status, 0);
		} else
			return NPERR_GENERIC_ERROR;

		free_buffer(This);
		NPN_MemFree(This);
		instance->pdata = NULL;
	}

	return NPERR_NO_ERROR;
}

static int
my_fork(PluginInstance *This, const char *url)
{
	pid_t	pid;
	int	pipefd[2];
	char	window_id[64], pdf_len[64];

	if (pipe(pipefd) == -1)
		return -1;

	pid = fork();
	if (pid == (pid_t)-1)
		return -1;
	/* child */
	if (pid == 0)
	{
		char **args = malloc(16 * sizeof(char *));
		int	argc;

		close(pipefd[1]);
		close(0);
		dup(pipefd[0]);

		argc = 0;
		args[argc++] = "oppdf-handler";
		args[argc++] = "--g-fatal-warnings";
		sprintf(window_id, "%lu", (unsigned long)This->window->window);
		args[argc++] = window_id;
		//printf("my_fork window_id/args[%d]=\"%s\"\n", argc - 1, args[argc - 1]);
		sprintf(pdf_len, "%lu", (unsigned long)This->len);
		args[argc++] = pdf_len;
		/*
		 * Set the envvar OPPDF_REARRANGE (to any value) to make
		 * the widgets rearrange themselves on resize
		 */
		args[argc++] = (char *)url;
		args[argc] = NULL;

		execvp("oppdf-handler", args);
		printf("my_fork(): starting oppdf-handler failed\n");
		exit(0);
	}
	/* parent */
	else
	{
		This->pid = pid;
		//printf("my_fork I am parent. Child pid %d\n", pid);
		close(pipefd[0]);
		This->socket = pipefd[1];
		return 0;
	}
}


NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
	PluginInstance			*This;
//	NPSetWindowCallbackStruct	*ws_info;
	int				xembedSupported = 0;

//	printf("NPP_SetWindow()\n");

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;
	if (This == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

//	ws_info = (NPSetWindowCallbackStruct *)window->ws_info;

//	printf("NPP_SetWindow: type %d\n", window->type);

	/* Mozilla likes to re-run its greatest hits */
	if ((window == This->window) &&
		(window->x == This->x) &&
		(window->y == This->y) &&
		(window->width == This->width) &&
		(window->height == This->height)) {
//		printf("  (window re-run; returning)\n");
		return NPERR_NO_ERROR;
	}

	NPN_GetValue(instance, NPNVSupportsXEmbedBool, &xembedSupported);
	if (!xembedSupported)
	{
		printf("OPPDF: XEmbed not supported\n");
		return NPERR_GENERIC_ERROR;
	}

	This->window = window;
	This->x = window->x;
	This->y = window->y;
	This->width = window->width;
	This->height = window->height;

	return NPERR_NO_ERROR;
}

int32
NPP_WriteReady(NPP instance, NPStream *stream)
{
	PluginInstance	*This;
	int32		size;

	//printf("NPP_WriteReady()\n");

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	/* if stream size is known, consume it at once */
	size = stream->end > 0 ? stream->end - This->len : MAXCHUNK;

	if (This->buf == NULL)
	{
		//printf("NPP_WriteReady() first allocation %d\n", size);
		This->buf = NPN_MemAlloc(size);
		This->bufsize = size;
	}
	else if (This->bufsize == This->len)
	{
		char	*buf;

		This->bufsize += MAXCHUNK;
		buf = NPN_MemAlloc(This->bufsize);
		memcpy(buf, This->buf, This->len);
		//printf("NPP_WriteReady() reallocation to %d, %d bytes copied\n", This->bufsize, This->len);
		NPN_MemFree(This->buf);
		This->buf = buf;
	}

	return size;
}


int32
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
	PluginInstance	*This;
	int32		size;
	char		*buf;

	//printf("NPP_Write() stream->end=%d offset=%d len=%d\n", stream->end, offset, len);

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	//printf("NPP_Write() This->bufsize=%d This->len=%d\n", This->bufsize, This->len);

	size = (len < (This->bufsize - This->len) ? len : This->bufsize - This->len);

	buf = buffer;
	memcpy(This->buf + This->len, buf, len);
	This->len += size;

	return size;
}

NPError
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
	PluginInstance	*This;

	//printf("NPP_DestroyStream()\n");
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	//printf("stream length arrived: %u, reason: %d\n", This->len, reason);

	if (reason == NPRES_DONE)
	{
		int	written = 0;
		int	len;

		if (my_fork(This, stream->url) < 0) {
			free_buffer(This);
			return NPERR_GENERIC_ERROR;
		}

		for (len = 0; len < This->len; len += written) {
			written = write(This->socket, This->buf + len, This->len - len);
			if (written < 0) {
				close(This->socket);
				This->socket = -1;
				return NPERR_GENERIC_ERROR;
			}
		}
		close(This->socket);
		This->socket = -1;

		//printf("NPP_DestroyStream(): PDF arrived OK, passed %d , %d bytes to external helper\n", This->len, len);
	} else {
		free_buffer(This);
	}

	return NPERR_NO_ERROR;
}

void
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
	//printf("NPP_StreamAsFile()\n");

	/***** Insert NPP_StreamAsFile code here *****\
	PluginInstance* This;
	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;
	\*********************************************/
}


void
NPP_URLNotify(NPP instance, const char* url,
		NPReason reason, void* notifyData)
{
	//printf("NPP_URLNotify()\n");
	/***** Insert NPP_URLNotify code here *****\
	PluginInstance* This;
	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;
	\*********************************************/
}


void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
	//printf("NPP_Print()\n");
	if (printInfo == NULL)
		return;

	if (instance != NULL) {
		/***** Insert NPP_Print code here *****\
		PluginInstance* This = (PluginInstance*) instance->pdata;
		\**************************************/

		if (printInfo->mode == NP_FULL) {
			/*
			 * PLUGIN DEVELOPERS:
			 *  If your plugin would like to take over
			 *  printing completely when it is in full-screen mode,
			 *  set printInfo->pluginPrinted to TRUE and print your
			 *  plugin as you see fit.  If your plugin wants Netscape
			 *  to handle printing in this case, set
			 *  printInfo->pluginPrinted to FALSE (the default) and
			 *  do nothing.  If you do want to handle printing
			 *  yourself, printOne is true if the print button
			 *  (as opposed to the print menu) was clicked.
			 *  On the Macintosh, platformPrint is a THPrint; on
			 *  Windows, platformPrint is a structure
			 *  (defined in npapi.h) containing the printer name, port,
			 *  etc.
			 */

			/***** Insert NPP_Print code here *****\
			void* platformPrint =
					printInfo->print.fullPrint.platformPrint;
			NPBool printOne =
					printInfo->print.fullPrint.printOne;
			\**************************************/

			/* Do the default*/
			printInfo->print.fullPrint.pluginPrinted = FALSE;

			//printf("NPP_Print NP_FULL\n");
		}
		else {  /* If not fullscreen, we must be embedded */
			/*
			 * PLUGIN DEVELOPERS:
			 *  If your plugin is embedded, or is full-screen
			 *  but you returned false in pluginPrinted above, NPP_Print
			 *  will be called with mode == NP_EMBED.  The NPWindow
			 *  in the printInfo gives the location and dimensions of
			 *  the embedded plugin on the printed page.  On the
			 *  Macintosh, platformPrint is the printer port; on
			 *  Windows, platformPrint is the handle to the printing
			 *  device context.
			 */

			/***** Insert NPP_Print code here *****\
			NPWindow* printWindow =
					&(printInfo->print.embedPrint.window);
			void* platformPrint =
					printInfo->print.embedPrint.platformPrint;
			\**************************************/
			//printf("NPP_Print NP_EMBED\n");
		}
	}
}

int16 NPP_HandleEvent(NPP instance, void* event)
{
	printf("NPP_HandleEvent()\n");
	return 0;
}

