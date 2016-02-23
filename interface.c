#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <poppler/glib/poppler.h>

#include "support.h"
#include "callbacks.h"
#include "interface.h"

typedef struct {
	int	check_width;
	int	width;
} handler_widths;

static handler_widths handler_width[PDF_HANDLERS] = {
	{ 1, 34 }, { 1, 34 }, { 1, 34 }, { 1, 34 }, { 1, 35 },
	{ 1, 80 }, { 1, 80 }, { 1, 128 }, { 1, 34 }, { 1, 32 },
	{ 1, 30 }, { 1, 160 }, { 1, 34 }, { 1, 34 },
	{ 0, -1 }
};

#define add_hboxes(ptr, hbox_idx)							\
{											\
	int	i;									\
	for (i = 0; i < PDF_HANDLERS; i++) {						\
		GList	*children =							\
			gtk_container_get_children(GTK_CONTAINER(ptr->handler_hbox[i]));\
		if ((ptr->handler_hbox_added[i] == 0) &&				\
				(g_list_length(children) > 0)) {			\
			/*								\
			printf("add_hboxes: "						\
				"adding hbox %d to main container\n", i);		\
			*/								\
			gtk_box_pack_start (GTK_BOX (ptr->container),			\
					ptr->handler_hbox[i],				\
						FALSE, FALSE, 0);			\
			ptr->handler_hbox_added[i] = 1;					\
		}									\
	}										\
}

#define add_handler_to_hbox(ptr, hbox_idx, widget_idx, widget_num)			\
	while (hbox_idx < PDF_HANDLERS)							\
	{										\
		int	next_width;							\
		if (widget_num == 0)							\
			next_width = handler_width[widget_idx].width;			\
		else									\
			next_width = handler_width[widget_idx].width + 4;		\
		if (handler_width[widget_idx].check_width == 0) {			\
			/*								\
			printf("add_handler_to_hbox: "					\
				"adding widget %d to main container\n", widget_idx);	\
			*/								\
			gtk_widget_show(ptr->handlers[widget_idx]);			\
			gtk_box_pack_start (GTK_BOX (ptr->container),			\
					ptr->handlers[widget_idx], TRUE, TRUE, 0);	\
			widget_num++;							\
			break;								\
		} else if ((handler_width[widget_idx].check_width == 1 &&		\
				next_width < width) ||					\
				/*							\
				 * Webkit hack: webkit passes the XEMBED parent		\
				 * as 1x1 window					\
				 */							\
				(ptr->w == 1 && ptr->h == 1)) {				\
			gtk_widget_show(ptr->handler_hbox[hbox_idx]);			\
			gtk_widget_show(ptr->handlers[widget_idx]);			\
			/*								\
			printf("add_handler_to_hbox: "					\
				"adding widget %d to hbox %d\n", widget_idx, hbox_idx);	\
			*/								\
			gtk_box_pack_start (GTK_BOX (ptr->handler_hbox[hbox_idx]),	\
					ptr->handlers[widget_idx], FALSE, FALSE, 0);	\
			/*								\
			 * Only account for the dynamic width if			\
			 * the Webkit hack is not in effect.				\
			 */								\
			if (!(ptr->w == 1 && ptr->h == 1) &&				\
				handler_width[widget_idx].check_width == 1)		\
				width -= next_width;					\
			widget_num++;							\
			break;								\
		} else {								\
			hbox_idx++;							\
			widget_num = 0;							\
			width = ptr->w - 8;						\
		}									\
	}										\
	add_hboxes(ptr, hbox_idx);							\
	widget_idx++;

#define add_handler_to_layout(ptr, hbox_idx, widget_idx, widget_num)			\
	while (hbox_idx < PDF_HANDLERS)							\
	{										\
		int	next_width;							\
		if (widget_num == 0)							\
			next_width = handler_width[widget_idx].width;			\
		else									\
			next_width = handler_width[widget_idx].width + 4;		\
		if (handler_width[widget_idx].check_width == 0) {			\
			/*								\
			printf("add_handler_to_layout: "				\
				"adding widget %d to main container\n", widget_idx);	\
			*/								\
			widget_num++;							\
			break;								\
		} else if ((handler_width[widget_idx].check_width == 1 &&		\
				next_width < width) ||					\
				/*							\
				 * Webkit hack: webkit passes the XEMBED parent		\
				 * as 1x1 window					\
				 */							\
				(ptr->w == 1 && ptr->h == 1)) {				\
			/*								\
			printf("add_handler_to_layout: "				\
				"adding widget %d to hbox %d\n", widget_idx, hbox_idx);	\
			*/								\
			new_layout[hbox_idx] = g_list_append(new_layout[hbox_idx],	\
							ptr->handlers[widget_idx]);	\
			/*								\
			 * Only account for the dynamic width if			\
			 * the Webkit hack is not in effect.				\
			 */								\
			if (!(ptr->w == 1 && ptr->h == 1) &&				\
				handler_width[widget_idx].check_width == 1)		\
				width -= next_width;					\
			widget_num++;							\
			break;								\
		} else {								\
			hbox_idx++;							\
			widget_num = 0;							\
			width = ptr->w - 8;						\
		}									\
	}										\
	widget_idx++;


pdf_browser_contents*
create_pdf_browser (GdkNativeWindow nw, oppdf_doc *doc_ptr)
{
	pdf_browser_contents	*ptr;
	GtkWidget		*button_image;
	GtkObject		*adjustment;
	int			i, num, hbox_idx, widget_num, width;

	ptr = g_new0(pdf_browser_contents, 1);
	ptr->doc_ptr = doc_ptr;

	if (nw == 0) {
		ptr->window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gdk_window_get_geometry(ptr->window1->window, &ptr->x, &ptr->y, &ptr->w, &ptr->h, &ptr->depth);
	} else {
#if GTK_CHECK_VERSION(2,24,0)
		GdkWindow	*fw = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(), nw);
#else
		GdkWindow	*fw = gdk_window_foreign_new(nw);
#endif

		gdk_window_get_geometry(fw, &ptr->x, &ptr->y, &ptr->w, &ptr->h, &ptr->depth);
		printf("create_pdf_browser xembed parent size w %d h %d\n", ptr->w, ptr->h);

		ptr->window1 = gtk_plug_new(nw);
	}

	GTK_WIDGET_SET_FLAGS (ptr->window1, GTK_CAN_FOCUS);
#if 1
	gtk_widget_set_events(ptr->window1, GDK_ALL_EVENTS_MASK);
#else
	gtk_widget_add_events(ptr->window1,
						GDK_BUTTON_PRESS_MASK |
						GDK_BUTTON_RELEASE_MASK |
						GDK_KEY_PRESS_MASK |
						GDK_KEY_RELEASE_MASK |
						GDK_POINTER_MOTION_MASK |
						GDK_SCROLL_MASK |
						GDK_EXPOSURE_MASK |
						GDK_VISIBILITY_NOTIFY_MASK |
						GDK_ENTER_NOTIFY_MASK |
						GDK_LEAVE_NOTIFY_MASK |
						GDK_FOCUS_CHANGE_MASK);
#endif

	gtk_window_set_title (GTK_WINDOW (ptr->window1), "OPPDF");

	g_signal_connect((gpointer) ptr->window1, "delete_event",
			G_CALLBACK (on_window1_delete_event),
			ptr);
	g_signal_connect((gpointer) ptr->window1, "expose_event",
			G_CALLBACK (on_window1_expose_event),
			ptr);

	ptr->container = gtk_vbox_new (FALSE, 4);
	gtk_widget_show(ptr->container);
	gtk_container_add (GTK_CONTAINER (ptr->window1), ptr->container);
	gtk_container_set_border_width (GTK_CONTAINER (ptr->container), 4);

	for (i = 0; i < PDF_HANDLERS; i++) {
		ptr->handler_hbox[i] = gtk_hbox_new (FALSE, 4);
		g_object_ref(G_OBJECT(ptr->handler_hbox[i]));

#if 0
//		gtk_widget_show(ptr->handler_hbox[i]);
		gtk_box_pack_start (GTK_BOX (ptr->container), ptr->handler_hbox[i], FALSE, FALSE, 0);
#endif
	}

	/*
	 * The handler widgets are added by default to the main window
	 * in the initial arrangement, depending on the window width.
	 */

	hbox_idx = 0;	/* index in ptr->handler_hbox[] */
	widget_num = 0;	/* the widget nr in the current hbox */
	num = 0;	/* index in ptr->handlers[] */
	width = ptr->w - 8; /* usable width */

/* 0 */	ptr->firstpage_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_GOTO_FIRST, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->firstpage_button), button_image);
	g_signal_connect((gpointer) ptr->firstpage_button, "clicked",
			G_CALLBACK (on_firstpage_button_clicked), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 1 */	ptr->prevpage_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->prevpage_button), button_image);
	g_signal_connect((gpointer) ptr->prevpage_button, "clicked",
			G_CALLBACK (on_prevpage_button_clicked), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 2 */	ptr->nextpage_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->nextpage_button), button_image);
	g_signal_connect((gpointer) ptr->nextpage_button, "clicked",
			G_CALLBACK (on_nextpage_button_clicked), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 3 */	ptr->lastpage_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_GOTO_LAST, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->lastpage_button), button_image);
	g_signal_connect((gpointer) ptr->lastpage_button, "clicked",
			G_CALLBACK (on_lastpage_button_clicked), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 4 */	ptr->handlers[num] = gtk_label_new (_("Page:"));
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

	adjustment = gtk_adjustment_new (0, 0, 100, 1, 0, 0);
/* 5 */	ptr->page_spin = ptr->handlers[num] = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
	gtk_widget_set_size_request (ptr->page_spin, 80, -1);
	GTK_WIDGET_SET_FLAGS(ptr->page_spin, GTK_CAN_DEFAULT); /* from GTK 2.22 the proper way is:
								  gtk_widget_set_can_default(ptr->page_spin, TRUE); */
	g_signal_connect((gpointer) ptr->page_spin, "value_changed",
			G_CALLBACK (on_page_spin_value_changed), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 6 */	ptr->npages_label = ptr->handlers[num] = gtk_label_new (_("/ N"));
	gtk_widget_set_size_request (ptr->npages_label, 80, -1);
	gtk_misc_set_alignment (GTK_MISC (ptr->npages_label), 0, 0.5);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 7 */	ptr->scale_combo = ptr->handlers[num] = gtk_combo_box_new_text ();
	for (i = 0; scales[i].str; i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (ptr->scale_combo), _(scales[i].str));
	g_signal_connect((gpointer) ptr->scale_combo, "changed",
			G_CALLBACK (on_scale_combo_changed), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 8 */	ptr->print_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_PRINT, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->print_button), button_image);
	g_signal_connect((gpointer) ptr->print_button, "clicked",
			G_CALLBACK (on_print_button_clicked), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 9 */	ptr->email_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->email_button), button_image);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 10 */
	ptr->handlers[num] = gtk_label_new (_("Find:"));
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 11 */
	ptr->find_entry = ptr->handlers[num] = gtk_entry_new ();
	gtk_entry_set_invisible_char (GTK_ENTRY (ptr->find_entry), 8226);
	g_signal_connect((gpointer) ptr->find_entry, "activate",
			G_CALLBACK (on_find_entry_activate), ptr);
	g_signal_connect((gpointer) ptr->find_entry, "delete-from-cursor",
			G_CALLBACK (on_find_entry_delete), ptr);
	g_signal_connect((gpointer) ptr->find_entry, "insert-at-cursor",
			G_CALLBACK (on_find_entry_insert), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 12 */
	ptr->findprev_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->findprev_button), button_image);
	g_signal_connect((gpointer) ptr->findprev_button, "clicked",
			G_CALLBACK (on_findprev_button_clicked), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

/* 13 */
	ptr->findnext_button = ptr->handlers[num] = gtk_button_new();
	button_image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_NEXT, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(button_image);
	gtk_button_set_image(GTK_BUTTON(ptr->findnext_button), button_image);
	g_signal_connect((gpointer) ptr->findnext_button, "clicked",
			G_CALLBACK (on_findnext_button_clicked), ptr);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

#if ICONVIEW

/* 14 */
	ptr->hpaned = ptr->handlers[num] = gtk_hpaned_new ();
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);
	gtk_paned_set_position (GTK_PANED (ptr->hpaned), 0); /* FIXME: was 150 */

	ptr->iconview_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(ptr->iconview_scroll);
	gtk_paned_pack1(GTK_PANED (ptr->hpaned), ptr->iconview_scroll, FALSE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ptr->iconview_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (ptr->iconview_scroll), GTK_SHADOW_NONE);

	ptr->iconview = gtk_icon_view_new();
	gtk_widget_show (ptr->iconview);
	gtk_container_add (GTK_CONTAINER (ptr->iconview_scroll), ptr->iconview);
	g_signal_connect((gpointer) ptr->iconview, "selection_changed",
			G_CALLBACK (on_iconview_selection_changed),
			ptr);

	ptr->thumbs = gtk_list_store_new(N_THUMB, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	gtk_icon_view_set_model(GTK_ICON_VIEW(ptr->iconview), GTK_TREE_MODEL(ptr->thumbs));
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(ptr->iconview), THUMB_LABEL);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(ptr->iconview), THUMB_ICON);

	ptr->table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(ptr->table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(ptr->table), 4);
	gtk_widget_show(ptr->table);
	gtk_paned_pack2(GTK_PANED(ptr->hpaned), ptr->table, TRUE, TRUE);

#else

/* 14 */
	ptr->table = ptr->handlers[num] = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(ptr->table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(ptr->table), 4);
	gtk_widget_show(ptr->table);
	add_handler_to_hbox(ptr, hbox_idx, num, widget_num);

#endif

#if VIEWPORT
	ptr->viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show (ptr->viewport);
	gtk_table_attach(GTK_TABLE(ptr->table), ptr->viewport,
						0, 1, 0, 1,
						GTK_EXPAND | GTK_SHRINK | GTK_FILL,
						GTK_EXPAND | GTK_SHRINK | GTK_FILL,
						0, 0);
#if 0
	gtk_widget_set_events(ptr->viewport,
#else
	gtk_widget_add_events(ptr->viewport,
#endif
						GDK_BUTTON_MOTION_MASK |
						GDK_POINTER_MOTION_HINT_MASK |
						GDK_BUTTON_PRESS_MASK |
						GDK_BUTTON_RELEASE_MASK |
						GDK_SCROLL_MASK);
	g_signal_connect_after ((gpointer) ptr->viewport, "button_press_event",
			G_CALLBACK (on_viewport_button_press_event),
			ptr);
	g_signal_connect_after ((gpointer) ptr->viewport, "button_release_event",
			G_CALLBACK (on_viewport_button_release_event),
			ptr);
	g_signal_connect((gpointer) ptr->viewport, "motion_notify_event",
			G_CALLBACK (on_viewport_motion_notify_event),
			ptr);
	g_signal_connect((gpointer) ptr->viewport, "scroll-event",
			G_CALLBACK (on_viewport_scroll_event),
			ptr);
	g_signal_connect_after ((gpointer) ptr->viewport, "expose_event",
			G_CALLBACK (on_viewport_expose_event),
			ptr);
#else
	ptr->eventbox = gtk_event_box_new();
	gtk_widget_show(ptr->eventbox);
	gtk_table_attach(GTK_TABLE(ptr->table), ptr->eventbox,
						0, 1, 0, 1,
						GTK_EXPAND | GTK_SHRINK | GTK_FILL,
						GTK_EXPAND | GTK_SHRINK | GTK_FILL,
						0, 0);
#if 0
	gtk_widget_set_events(ptr->eventbox,
#else
	gtk_widget_add_events(ptr->eventbox,
#endif
						GDK_BUTTON_MOTION_MASK |
						GDK_POINTER_MOTION_HINT_MASK |
						GDK_BUTTON_PRESS_MASK |
						GDK_BUTTON_RELEASE_MASK |
						GDK_SCROLL_MASK);
	g_signal_connect_after ((gpointer) ptr->eventbox, "button_press_event",
			G_CALLBACK (on_viewport_button_press_event),
			ptr);
	g_signal_connect_after ((gpointer) ptr->eventbox, "button_release_event",
			G_CALLBACK (on_viewport_button_release_event),
			ptr);
	g_signal_connect((gpointer) ptr->eventbox, "motion_notify_event",
			G_CALLBACK (on_viewport_motion_notify_event),
			ptr);
	g_signal_connect((gpointer) ptr->eventbox, "scroll-event",
			G_CALLBACK (on_viewport_scroll_event),
			ptr);
	g_signal_connect_after ((gpointer) ptr->eventbox, "expose_event",
			G_CALLBACK (on_viewport_expose_event),
			ptr);
#endif

	ptr->image = gtk_image_new();
	gtk_widget_show (ptr->image);
#if VIEWPORT
	gtk_container_add(GTK_CONTAINER(ptr->viewport), ptr->image);
#else
	gtk_container_add(GTK_CONTAINER(ptr->eventbox), ptr->image);
#endif

	ptr->hscrollbar = gtk_hscrollbar_new(NULL);
	gtk_widget_show(ptr->hscrollbar);
	ptr->hadj = gtk_range_get_adjustment(GTK_RANGE(ptr->hscrollbar));
	gtk_table_attach(GTK_TABLE(ptr->table), ptr->hscrollbar,
						0, 1, 1, 2,
						GTK_EXPAND | GTK_SHRINK | GTK_FILL,
						GTK_SHRINK /* 0 , GTK_EXPAND | GTK_SHRINK *//* | GTK_FILL */,
						0, 0);
	g_signal_connect((gpointer) ptr->hadj, "value-changed",
			G_CALLBACK (on_scrollbar_adj_changed),
			ptr);

	ptr->vscrollbar = gtk_vscrollbar_new(NULL);
	gtk_widget_show(ptr->vscrollbar);
	ptr->vadj = gtk_range_get_adjustment(GTK_RANGE(ptr->vscrollbar));
	gtk_table_attach(GTK_TABLE(ptr->table), ptr->vscrollbar,
						1, 2, 0, 1,
						GTK_SHRINK /* 0 , GTK_EXPAND | GTK_SHRINK *//* | GTK_FILL */,
						GTK_EXPAND | GTK_SHRINK | GTK_FILL,
						0, 0);
	g_signal_connect((gpointer) ptr->vadj, "value-changed",
			G_CALLBACK (on_scrollbar_adj_changed),
			ptr);

	/*
	 * Add a refcount to all handler widgets so they are not lost
	 * when gtk_container_remove() is called on children.
	 */
	for (i = 0; i < PDF_HANDLERS; i++) {
//		printf("g_object_ref'ing object %d\n", i);
		g_object_ref(G_OBJECT(ptr->handlers[i]));
	}

#if 0
	/*
	 * Hide all remaining empty hboxes
	 */
	printf("last used hbox %d add %d\n", hbox_idx, (widget_num ? 1 : 0));
	for (i = hbox_idx + (widget_num ? 1 : 0); i < PDF_HANDLERS; i++) {
		printf("hiding hbox %d\n", i);
		gtk_widget_hide(ptr->handler_hbox[i]);
	}
#endif

	return ptr;
}

void
destroy_pdf_browser(pdf_browser_contents *ptr) {
	oppdf_doc	*doc_ptr = ptr->doc_ptr;
	int	i;

	for (i = 0; i < PDF_HANDLERS; i++)
		g_object_unref(ptr->handlers[i]);

	if (doc_ptr->pages) {
		for (i = 0; i < doc_ptr->n_pages; i++) {
			oppdf_pages *pageptr = &doc_ptr->pages[i];

			g_object_unref(pageptr->page);
			if (pageptr->pixmap)
				g_object_unref(pageptr->pixmap);
		}
		g_free(doc_ptr->pages);
	}

	g_object_unref(doc_ptr->doc);

	g_free(doc_ptr->pdf_buf);

	g_free(doc_ptr);

#if 0 /* segfault */
	gtk_widget_destroy(ptr->window1);
#endif

	g_free(ptr);
}

oppdf_doc * pdf_read(int size) {
	password_dialog	*password_dlg;
	error_dialog	*error_dlg;
	GError		*err = NULL;
	gchar		*pwd = NULL;
	int		len, i;
	double		offset;
	oppdf_doc	*ptr;

	ptr = g_new0(oppdf_doc, 1);

	ptr->pdf_buf = g_malloc(size);
	if (ptr->pdf_buf == NULL) {
		printf("oppdf-handler malloc error: %s\n", strerror(errno));
		exit(1);
	}
	len = 0;

	while (len < size) {
		ssize_t bytes_read = read(0, ptr->pdf_buf + len, size - len);

		if (bytes_read < 0) {
			printf("oppdf-handler stream error: %s\n", strerror(errno));
			exit(1);
		}
		printf("oppdf-handler read chunk %" PRIdFAST32 ", previous size %d\n", bytes_read, len);
		len += bytes_read;
	}
	printf("oppdf-handler stream read %d bytes\n", len);

	while ((ptr->doc = poppler_document_new_from_data(ptr->pdf_buf, len, pwd, &err)) == NULL) {
		switch (err->code)
		{
		case POPPLER_ERROR_ENCRYPTED:
			if (pwd != NULL)
				free(pwd);
			password_dlg = create_password();
			if (gtk_dialog_run(GTK_DIALOG(password_dlg->password)) != GTK_RESPONSE_OK) {
				destroy_password(password_dlg);
				exit(1);
			}
			pwd = strdup(gtk_entry_get_text(GTK_ENTRY(password_dlg->entry1)));
			destroy_password(password_dlg);
			break;
		default:
			error_dlg = create_error();
			gtk_label_set_label(GTK_LABEL(error_dlg->label), err->message);
			gtk_dialog_run(GTK_DIALOG(error_dlg->error));
			destroy_error(error_dlg);
			g_error_free(err);
			exit(1);
		}
		g_error_free(err);
		err = NULL;
	}

	ptr->n_pages = poppler_document_get_n_pages(ptr->doc);

	/* get all pages, for pre-rendering thumbnails in an idle function */
	ptr->pages = g_new0(oppdf_pages, ptr->n_pages);
	offset = 0.0;
	for (i = 0; i < ptr->n_pages; i++) {
		oppdf_pages	*page = &ptr->pages[i];

		page->page = poppler_document_get_page(ptr->doc, i);
		poppler_page_get_size(page->page, &page->pw, &page->ph);
		page->offset = offset;
		offset += page->ph;
	}

	return ptr;
}

void pdf_browser_display_data(pdf_browser_contents *ptr, char *url) {
	oppdf_doc	*doc_ptr = ptr->doc_ptr;
	PopplerViewerPreferences view_prefs;
	PopplerPageLayout page_layout;
	PopplerPageMode page_mode;
	PopplerPermissions permissions;
	gchar		*pages;
	int		i;

	pages = g_strdup_printf("/ %d", doc_ptr->n_pages);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(ptr->page_spin), 1, doc_ptr->n_pages);
	gtk_label_set_label(GTK_LABEL(ptr->npages_label), pages);
	g_free(pages);

	g_object_get(doc_ptr->doc,  "viewer-preferences", &view_prefs,
				"page-layout", &page_layout,
				"page-mode", &page_mode,
				"permissions", &permissions,
				NULL);
	//printf("viewer-preferences=%d page-layout=%d page-mode=%d permissions=%d\n", view_prefs, page_layout, page_mode, permissions);
	/* Best fit/Fit page width */
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_FIT_WINDOW) {
		doc_ptr->fit_scale = BESTFIT;
		gtk_combo_box_set_active(GTK_COMBO_BOX(ptr->scale_combo), 0);
	} else {
		doc_ptr->fit_scale = FITPAGEWIDTH;
		gtk_combo_box_set_active(GTK_COMBO_BOX(ptr->scale_combo), 1);
	}

#if ICONVIEW
	/* Thumbnailing */
	if (page_mode != POPPLER_PAGE_MODE_USE_THUMBS)
		gtk_paned_set_position(GTK_PANED(ptr->hpaned), 0);
#endif

	//printf("oppdf-handler: url='%s'\n", url);
	for (i = 0; url[i]; i++)
		if (url[i] == '#')
			break;

	doc_ptr->current_page = 1;
	ptr->vadj_val = 0;
	ptr->hadj_val = 0;
	if (url[i] == '#') {
		/* possibly valid #link inside the PDF */
		i++;
		doc_ptr->dest = poppler_document_find_dest(doc_ptr->doc, url + i);
		printf("oppdf-handler(): dest='%s' (%s %p)\n", url + i, (doc_ptr->dest ? "found" : "not found"), doc_ptr->dest);
		if (doc_ptr->dest != NULL) {
			printf("oppdf-handler(): dest='%s' found on page %d (1-based), left %.2lf top %.2lf right %.2lf bottom %.2lf\n",
				url + i, doc_ptr->dest->page_num,
				doc_ptr->dest->left, doc_ptr->dest->top,
				doc_ptr->dest->right, doc_ptr->dest->bottom);
			doc_ptr->current_page = doc_ptr->dest->page_num;
		}
	}

	//printf("oppdf-handler(): start on page %d\n", ptr->current_page);
}

static void
remove_child_from_hbox(gpointer data, gpointer user_data)
{
	GtkContainer	*container = user_data;
	GtkWidget	*child = data;

	gtk_container_remove(container, child);
}

void create_pdf_browser_contents(pdf_browser_contents *ptr) {
	GList	*orig_layout[PDF_HANDLERS], *new_layout[PDF_HANDLERS], *top_children;
	int	hbox_idx, widget_idx, widget_num, width;

	/*
	 * Check whether the layout has to change because the window size changes too much:
	 * Steps are below.
	 *
	 * 1. Initialize layout GList chains: orig_layout[] has the list of current children
	 *    of hboxes, new_layout[] are empty.
	 */
	for (hbox_idx = 0; hbox_idx < PDF_HANDLERS; hbox_idx++) {
		orig_layout[hbox_idx] = gtk_container_get_children(GTK_CONTAINER(ptr->handler_hbox[hbox_idx]));
		new_layout[hbox_idx] = NULL;
	}

	/*
	 * 2. Add widgets to new_layout[] according to current window size. 
	 */
	hbox_idx = 0;
	widget_idx = 0;
	widget_num = 0;
	width = ptr->w - 8;
	while (hbox_idx < PDF_HANDLERS && widget_idx < PDF_HANDLERS) {
		add_handler_to_layout(ptr, hbox_idx, widget_idx, widget_num);
	}

	/*
	 * 3. Check whether the two layouts are the same. It they are,
	 *    just bail out. Otherwise rearrange the widgets into the hboxes.
	 */
	for (hbox_idx = 0; hbox_idx < PDF_HANDLERS; hbox_idx++) {
		GList	*l1 = orig_layout[hbox_idx];
		GList	*l2 = new_layout[hbox_idx];

		if (l1 == NULL && l2 == NULL)
			continue;
		if (g_list_length(l1) != g_list_length(l2))
			goto rearrange;

		widget_idx = 0;
		while (l1 != NULL && l2 != NULL) {
			if (l1->data != l2->data)
				goto rearrange;
			l1 = g_list_next(l1);
			l2 = g_list_next(l2);
			widget_idx++;
		}
	}

	printf("no change in layout\n");
	return;

rearrange:
	printf("create_pdf_browser_contents rearrange\n");

	/*
	 * Remove all children from all hboxes and remove the hboxes
	 * from the top level containers.
	 */
	top_children = gtk_container_get_children(GTK_CONTAINER(ptr->container));
	for (hbox_idx = 0; hbox_idx < PDF_HANDLERS; hbox_idx++) {
		GList	*children;

		children = gtk_container_get_children(GTK_CONTAINER(ptr->handler_hbox[hbox_idx]));
		g_list_foreach(children, remove_child_from_hbox, ptr->handler_hbox[hbox_idx]);
		g_list_free(children);

		if (g_list_find(top_children, ptr->handler_hbox[hbox_idx]))
			gtk_container_remove(GTK_CONTAINER(ptr->container), ptr->handler_hbox[hbox_idx]);
		ptr->handler_hbox_added[hbox_idx] = 0;
	}
	g_list_free(top_children);

#if ICONVIEW
	gtk_container_remove(GTK_CONTAINER(ptr->container), ptr->hpaned);
#else
	gtk_container_remove(GTK_CONTAINER(ptr->container), ptr->table);
#endif

	hbox_idx = 0;	/* index in ptr->handler_hbox[] */
	widget_idx = 0;	/* index ptr->handlers[] */
	widget_num = 0; /* number of widgets in the current hbox */
	width = ptr->w - 8;
	while (hbox_idx < PDF_HANDLERS && widget_idx < PDF_HANDLERS) {
		add_handler_to_hbox(ptr, hbox_idx, widget_idx, widget_num);
	}

	printf("%d handler widgets shown\n", widget_idx);

	ptr->handlers_shown = 1;
//	gtk_widget_show_all(ptr->window1);
}

error_dialog *
create_error (void)
{
	error_dialog	*dlg = g_new0(error_dialog, 1);

	dlg->error = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dlg->error), _("Error opening PDF"));
	gtk_window_set_position (GTK_WINDOW (dlg->error), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal (GTK_WINDOW (dlg->error), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (dlg->error), FALSE);
	gtk_window_set_type_hint (GTK_WINDOW (dlg->error), GDK_WINDOW_TYPE_HINT_DIALOG);

	dlg->vbox = GTK_DIALOG (dlg->error)->vbox;
	gtk_widget_show (dlg->vbox);

	dlg->label = gtk_label_new ("label");
	gtk_widget_show (dlg->label);
	gtk_box_pack_start (GTK_BOX (dlg->vbox), dlg->label, FALSE, FALSE, 0);

	dlg->action_area = GTK_DIALOG (dlg->error)->action_area;
	gtk_widget_show (dlg->action_area);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dlg->action_area), GTK_BUTTONBOX_END);

	dlg->okbutton = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show (dlg->okbutton);
	gtk_dialog_add_action_widget (GTK_DIALOG (dlg->error), dlg->okbutton, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (dlg->okbutton, GTK_CAN_DEFAULT);

	return dlg;
}

void destroy_error(error_dialog *dlg)
{
	gtk_widget_destroy(dlg->error);
	g_free(dlg);
}

password_dialog*
create_password (void)
{
	password_dialog	*dlg = g_new0(password_dialog, 1);

	dlg->password = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dlg->password), _("Password protected PDF"));
	gtk_window_set_modal (GTK_WINDOW (dlg->password), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (dlg->password), FALSE);
	gtk_window_set_type_hint (GTK_WINDOW (dlg->password), GDK_WINDOW_TYPE_HINT_DIALOG);

	dlg->dialog_vbox2 = GTK_DIALOG (dlg->password)->vbox;
	gtk_widget_show (dlg->dialog_vbox2);

	dlg->hbox2 = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (dlg->hbox2);
	gtk_box_pack_start (GTK_BOX (dlg->dialog_vbox2), dlg->hbox2, FALSE, FALSE, 0);

	dlg->label4 = gtk_label_new (_("Type in the password to unlock the document:"));
	gtk_widget_show (dlg->label4);
	gtk_box_pack_start (GTK_BOX (dlg->hbox2), dlg->label4, FALSE, FALSE, 0);

	dlg->entry1 = gtk_entry_new ();
	gtk_widget_show (dlg->entry1);
	gtk_box_pack_start (GTK_BOX (dlg->hbox2), dlg->entry1, TRUE, TRUE, 0);
	gtk_entry_set_visibility (GTK_ENTRY (dlg->entry1), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY (dlg->entry1), 8226);
	dlg->dialog_action_area2 = GTK_DIALOG (dlg->password)->action_area;
	gtk_widget_show (dlg->dialog_action_area2);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dlg->dialog_action_area2), GTK_BUTTONBOX_END);

	dlg->cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (dlg->cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (dlg->password), dlg->cancelbutton1, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (dlg->cancelbutton1, GTK_CAN_DEFAULT);

	dlg->okbutton2 = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show (dlg->okbutton2);
	gtk_dialog_add_action_widget (GTK_DIALOG (dlg->password), dlg->okbutton2, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (dlg->okbutton2, GTK_CAN_DEFAULT);

	gtk_widget_grab_focus (dlg->entry1);
	gtk_widget_grab_default (dlg->okbutton2);

	return dlg;
}

void
destroy_password(password_dialog *dlg)
{
	gtk_widget_destroy(dlg->password);
	g_free(dlg);
}
