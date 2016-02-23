/*
 * Minimalistic GTK XEMBED test for OPPDF
 *
 * There is one "bug", caused by the minimalistic nature
 * of this test code: the window cannot be resized to be
 * smaller. The GtkViewport without automatic scrollbars
 * and the fixed size image makes it this way.
 *
 * In real browsers like Webkit and Mozilla, the parent is
 * modified in a way that it can for the XEMBED client to
 * resize anyway. GTK happily conforms to that.
 * 
 */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

static gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	printf("parent: delete event\n");
	gtk_main_quit();
	return TRUE;
}

#define PDFBUFSZ	4096
char	pdfbuf[PDFBUFSZ];

int my_fork(const char *self, const char *filename, const char *bookmark, unsigned long nw) {
	pid_t	pid;
	int	fd;
	size_t	read_bytes;
	gchar	*url, *final_url;
	GError	*err = NULL;
	struct stat statbuf;
	int	pipefd[2];
	char	window_id[64], pdf_len[64];

	url = g_filename_to_uri(filename, NULL, &err);
	if (err && err->code) {
		printf("%s: %s\n", self, err->message);
		g_error_free(err);
		return -1;
	}
	if (err)
		g_error_free(err);

	if (bookmark)
		final_url = g_strdup_printf("%s#%s", url, bookmark);
	else
		final_url = g_strdup(url);
	g_free(url);

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf("%s: cannot open \"%s\": %s\n", self, filename, strerror(errno));
		g_free(url);
		return -1;
	}

	if (fstat(fd, &statbuf) == -1) {
		printf("%s: cannot stat \"%s\": %s\n", self, filename, strerror(errno));
		close(fd);
		g_free(url);
		return -1;
	}

	if (pipe(pipefd) == -1)
		return -1;

	pid = fork();
	if (pid == (pid_t)-1)
		return -1;
	/* child */  
	if (pid == 0) {
		char **args = malloc(16 * sizeof(char *));
		int	argc;

		close(fd);
		close(pipefd[1]);
		close(0);
		dup(pipefd[0]);
		argc = 0;
		args[argc++] = "oppdf-handler";
		args[argc++] = "--g-fatal-warnings";
		sprintf(window_id, "%lu", nw);
		args[argc++] = window_id;
		//printf("my_fork window_id/args[%d]=\"%s\"\n", argc - 1, args[argc - 1]);
		sprintf(pdf_len, "%lu", (unsigned long)statbuf.st_size);
		args[argc++] = pdf_len;
		args[argc++] = final_url;
		args[argc] = NULL;
		execvp("oppdf-handler", args);
		printf("my_fork(): starting oppdf-handler failed\n");
		exit(0);
	} else { /* parent */
		g_free(final_url);

		close(pipefd[0]);

		do {
			read_bytes = read(fd, pdfbuf, PDFBUFSZ);
			if (read_bytes > 0)
				write(pipefd[1], pdfbuf, read_bytes);
		} while (read_bytes > 0);
		close(pipefd[1]);
		close(fd);
		return 0;
	}
}

int main(int argc, char **argv) {
	GtkWidget	*window;
	GtkWidget	*socket;
	char		*self = argv[0];
	char		*filename;
	char		*bookmark = NULL;

	gtk_init(&argc, &argv);

	if (argc != 2 && argc != 3) {
		printf("usage: %s filename\n", self);
		printf("usage: filename is a PDF file with absolute path\n");
		return 1;
	}

	filename = argv[1];
	if (argc == 3)
		bookmark = argv[2];

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_widget_show(window);
	g_signal_connect ((gpointer) window, "delete_event",
				G_CALLBACK (on_window_delete),
				NULL);

	socket = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(window), socket);
	gtk_widget_show(socket);

	my_fork(self, filename, bookmark, (unsigned long)gtk_socket_get_id(GTK_SOCKET(socket)));

	gtk_main();

	return 0;
}
