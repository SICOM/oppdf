#ifndef OPPDF_H
#define OPPDF_H

#include <stdint.h>

#include "moz-sdk/npapi.h"

#define PLUGIN_NAME		"OPPDF Plugin"
#define MIME_TYPES_HANDLED	\
				"application/acrobat:pdf:PDF document;" \
				"application/pdf:pdf:PDF document;" \
				"appliation/octet-stream:pdf:PDF document;" \
				"application/x-pdf:pdf:PDF document;" \
				"application/vnd.pdf:pdf:PDF document;" \
				"text/pdf:pdf:PDF document;" \
				"text/x-pdf:pdf:PDF document;"

#define PLUGIN_DESCRIPTION	"OPPDF Plugin"

#define MAXCHUNK		(32*1024)

typedef struct {
	uint16		mode;
	NPWindow	*window;
	uint32		x, y;
	uint32		width, height;
	NPP		instance;

	pid_t		pid;
	int		socket;

	char		*buf;
	uint32		bufsize;
	uint32		len;
} PluginInstance;

#endif /* OPPDF_H */
