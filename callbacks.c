#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <poppler/glib/poppler.h>
#include <gtk/gtkprintunixdialog.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ps.h>

#include "callbacks.h"
#include "interface.h"

gboolean
on_window1_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	printf("oppdf-handler: delete event\n");
	gtk_main_quit();
	return FALSE;
}


void
on_firstpage_button_clicked(GtkButton *button, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	gdouble		min, max;

	gtk_spin_button_get_range(GTK_SPIN_BUTTON(ptr->page_spin), &min, &max);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(ptr->page_spin), min);
}


void
on_prevpage_button_clicked(GtkButton *button, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	gdouble		val;

	val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ptr->page_spin));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(ptr->page_spin), val - 1);

	oppdf_compute_scale(ptr);
	oppdf_compute_sizes(ptr);
	oppdf_set_scrollbars(ptr, NULL, NULL);
}


void
on_nextpage_button_clicked(GtkButton *button, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	gdouble		val;

	val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ptr->page_spin));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(ptr->page_spin), val + 1);
}


void
on_lastpage_button_clicked(GtkButton *button, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	gdouble		min, max;

	gtk_spin_button_get_range(GTK_SPIN_BUTTON(ptr->page_spin), &min, &max);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(ptr->page_spin), max);
}

void
on_page_spin_value_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	oppdf_doc	*doc_ptr = ptr->doc_ptr;
	gint		page, imin, imax;
	gdouble		min, max;
	gboolean	s1 = TRUE, s2 = TRUE, s3 = TRUE, s4 = TRUE;

	page = gtk_spin_button_get_value_as_int(spinbutton);

	//printf("%s page %d\n", __FUNCTION__, page);
	gtk_spin_button_get_range(spinbutton, &min, &max);
	imin = min; imax = max;

	if (page == imin)
		s1 = s2 = FALSE;
	if (page == imax)
		s3 = s4 = FALSE;

	gtk_widget_set_sensitive(ptr->firstpage_button, s1);
	gtk_widget_set_sensitive(ptr->prevpage_button, s2);
	gtk_widget_set_sensitive(ptr->nextpage_button, s3);
	gtk_widget_set_sensitive(ptr->lastpage_button, s4);

	doc_ptr->current_page = page;

#if ICONVIEW
	if (!ptr->iconview_change) {
		char		page_str[64];
		GtkTreePath	*path;

		ptr->iconview_change = 1;
		sprintf(page_str, "%d", page - 1);
		path = gtk_tree_path_new_from_string(page_str);
		gtk_icon_view_select_path(GTK_ICON_VIEW(ptr->iconview), path);
		gtk_tree_path_free(path);
		ptr->iconview_change = 0;
	}
#endif

	if (!ptr->render_in_progress) {
		oppdf_compute_scale(ptr);
		oppdf_compute_sizes(ptr);
		oppdf_set_scrollbars(ptr, NULL, NULL);
	}
}

void
on_scale_combo_changed(GtkComboBox *combobox, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	oppdf_doc	*doc_ptr = ptr->doc_ptr;
	gint	value;
	double	vpos, hpos;

	vpos = gtk_adjustment_get_value(ptr->vadj);
	hpos = gtk_adjustment_get_value(ptr->hadj);
	if (doc_ptr->continuous_mode) {
		oppdf_pages	*pageptr = &doc_ptr->pages[doc_ptr->current_page - 1];

		vpos -= pageptr->offset;
	}
	vpos /= doc_ptr->scale;
	hpos /= doc_ptr->scale;

	value = gtk_combo_box_get_active(combobox);
	doc_ptr->fit_scale = scales[value].value;

	oppdf_compute_scale(ptr);
	oppdf_compute_sizes(ptr);

	vpos *= doc_ptr->scale;
	hpos *= doc_ptr->scale;

	oppdf_set_scrollbars(ptr, &hpos, &vpos);
}

static void begin_print(GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data) {
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;

	gtk_print_operation_set_n_pages(operation, ptr->doc_ptr->n_pages);
}

static void draw_page (GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer user_data) {
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	cairo_t *cairo;

	//printf("draw_page called page_nr %d\n", page_nr);
	cairo = gtk_print_context_get_cairo_context(context);
	poppler_page_render_for_printing(ptr->doc_ptr->pages[page_nr].page, cairo);
}

static GtkPrintSettings *saved_settings = NULL;

void
on_print_button_clicked(GtkButton *button, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	GtkPrintSettings *settings;
	GtkPrintOperation *print;
	GtkPrintOperationResult res;

	if (saved_settings)
		settings = saved_settings;
	else
		settings = gtk_print_settings_new();

	print = gtk_print_operation_new();
	gtk_print_operation_set_print_settings (print, settings);
	g_signal_connect (print, "begin_print", G_CALLBACK (begin_print), user_data);
	g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), user_data);

	gtk_print_operation_set_current_page(print, ptr->doc_ptr->current_page - 1); /* 0-based page numbers */

	res = gtk_print_operation_run (print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, NULL);
	//printf("on_print_button_clicked: gtk_print_operation_run result: %d\n", res);
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		g_object_unref(settings);
		saved_settings = g_object_ref (gtk_print_operation_get_print_settings (print));
	}
	g_object_unref (print);
}


void
on_findprev_button_clicked(GtkButton *button, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	int	page;

	if (ptr->search_list)
		page = oppdf_search_prev(ptr);
	else
		page = oppdf_search_new_prev(ptr, ptr->doc_ptr->current_page);

	oppdf_compute_selection(ptr, page);
}


void
on_findnext_button_clicked(GtkButton *button, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	int	page;

	if (ptr->search_list)
		page = oppdf_search_next(ptr);
	else
		page = oppdf_search_new(ptr, ptr->doc_ptr->current_page);

	oppdf_compute_selection(ptr, page);
}


#if ICONVIEW
void
on_iconview_selection_changed(GtkIconView *iconview, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	oppdf_doc *doc_ptr = ptr->doc_ptr;
	GtkTreePath	*path;
	gchar		*str; 
	int		page; 

//	printf("on_iconview_selection_changed\n");

	if (ptr->iconview_change)
		return;

	ptr->iconview_change = 1;

	if (!gtk_icon_view_get_cursor(iconview, &path, NULL))
		return;


	str = gtk_tree_path_to_string(path);
	printf("on_iconview_selection_changed: path '%s'\n", str);
	sscanf(str, "%d", &page);

	if (doc_ptr->continuous_mode)
	{
		oppdf_pages	*pageptr;

		pageptr = &doc_ptr->pages[page];

		gtk_adjustment_set_value(ptr->vadj, pageptr->offset);
	}
	else
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(ptr->page_spin), page + 1);

	g_free(str);
	gtk_tree_path_free(path);

	ptr->iconview_change = 0;
}
#endif


void
on_find_entry_activate(GtkEntry *entry, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;
	int	page;

	if (ptr->search_list)
		page = oppdf_search_next(ptr);
	else
		page = oppdf_search_new(ptr, ptr->doc_ptr->current_page);

	oppdf_compute_selection(ptr, page);
}

void
on_find_entry_delete			(GtkEntry	*entry,
					GtkDeleteType	arg1,
					gint		arg2,
					gpointer	user_data)
{
	pdf_browser_contents *ptr = user_data;

	oppdf_search_free(ptr);
}


void
on_find_entry_insert			(GtkEntry	*entry,
					gchar		*arg1,
					gpointer	user_data)
{
	pdf_browser_contents *ptr = user_data;

	oppdf_search_free(ptr);
}


static void toggle_continuous_mode(gpointer user_data)
{
	pdf_browser_contents *ptr = user_data;
	oppdf_doc *doc_ptr = ptr->doc_ptr;
	oppdf_pages *pageptr = &doc_ptr->pages[doc_ptr->current_page - 1];
	gdouble	xval, yval;

	doc_ptr->continuous_mode = 1 - doc_ptr->continuous_mode;

	oppdf_compute_scale(ptr);
	oppdf_compute_sizes(ptr);

	xval = ptr->hadj_val;
	if (doc_ptr->continuous_mode)
		yval = ptr->vadj_val + pageptr->offset;
	else
		yval = ptr->vadj_val - pageptr->offset;
	oppdf_set_scrollbars(ptr, &xval, &yval);
}

gboolean
on_viewport_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;

	//printf("%s button %d\n", __FUNCTION__, event->button);
	switch (event->button)
	{
		case 1:
			ptr->image_drag = 1;
			ptr->event_x_prev = event->x;
			ptr->event_y_prev = event->y;
			break;
		case 3:
		{
			GtkWidget *menu;
			GtkWidget *menu_item;

			menu = gtk_menu_new();
			menu_item = gtk_check_menu_item_new_with_label ("Continuous");
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), ptr->doc_ptr->continuous_mode);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
			g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
					G_CALLBACK(toggle_continuous_mode),
					user_data);
			gtk_widget_show (menu_item);

			gtk_menu_set_title(GTK_MENU(menu), "Set continuous mode");
			gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
					3, event->time);
			break;
		}
		default:
			printf("on_viewport_button_press_event: button %d pressed\n", event->button);
			break;
	}

	return FALSE;
}

gboolean
on_viewport_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	pdf_browser_contents *ptr = (pdf_browser_contents *)user_data;

	switch (event->button)
	{
		case 1:
			ptr->image_drag = 0;
			break;
		default:
			printf("on_viewport_button_press_event: button %d released\n", event->button);
			break;
	}
	//printf("%s\n", __FUNCTION__);
	return FALSE;
}

gboolean
on_viewport_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	pdf_browser_contents *ptr = user_data;
	//gdouble	oldxval, oldyval;
	gdouble	xdiff, ydiff, xval, yval, xmin, ymin, xmax, ymax;

	if (!ptr->image_drag)
		return FALSE;

	//printf("on_image_motion_notify_event: x=%.0lf oldx=%d y=%.0lf oldy=%d\n", event->x, ptr->event_x_prev, event->y, ptr->event_y_prev);

	ydiff = event->y - ptr->event_y_prev;
	ptr->event_y_prev = event->y;

	xdiff = event->x - ptr->event_x_prev;
	ptr->event_x_prev = event->x;

	//oldxval =
	xval = gtk_adjustment_get_value(ptr->hadj);
	xmin = gtk_adjustment_get_lower(ptr->hadj);
	xmax = gtk_adjustment_get_upper(ptr->hadj) - gtk_adjustment_get_page_size(ptr->hadj);
	xval = xval - xdiff;
	if (xval < xmin)
		xval = xmin;
	if (xval > xmax)
		xval = xmax;

	//oldyval = 
	yval = gtk_adjustment_get_value(ptr->vadj);
	ymin = gtk_adjustment_get_lower(ptr->vadj);
	ymax = gtk_adjustment_get_upper(ptr->vadj) - gtk_adjustment_get_page_size(ptr->vadj);
	yval = yval - ydiff;
	if (yval < ymin)
		yval = ymin;
	if (yval > ymax)
		yval = ymax;

	//printf("on_image_motion_notify_event: xdiff=%.0lf oldx=%.0lf xval=%.0lf xmax=%.0lf ydiff=%.0lf oldy=%.0lf yval=%.0lf ymax=%.0lf\n",
	//		xdiff, oldxval, xval, xmax, ydiff, oldyval, yval, ymax);

	gtk_adjustment_set_value(ptr->hadj, xval);
	gtk_adjustment_set_value(ptr->vadj, yval);

	return FALSE;
}

gboolean on_viewport_scroll_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	pdf_browser_contents *ptr = user_data;
	GdkEventScroll *scroll_event = (GdkEventScroll *)event;
	gdouble	xmin, xmax, xval, xstep, ymin, ymax, yval, ystep;

	xmin = gtk_adjustment_get_lower(ptr->hadj);
	xmax = gtk_adjustment_get_upper(ptr->hadj) - gtk_adjustment_get_page_size(ptr->hadj);
	xval = gtk_adjustment_get_value(ptr->hadj);
	xstep= gtk_adjustment_get_step_increment(ptr->hadj);

	ymin = gtk_adjustment_get_lower(ptr->vadj);
	ymax = gtk_adjustment_get_upper(ptr->vadj) - gtk_adjustment_get_page_size(ptr->vadj);
	yval = gtk_adjustment_get_value(ptr->vadj);
	ystep= gtk_adjustment_get_step_increment(ptr->vadj);

	switch (scroll_event->direction)
	{
		case GDK_SCROLL_UP:
			ystep = -ystep;
			/* fall through */
		case GDK_SCROLL_DOWN:
			yval += ystep;
			if (yval < ymin)
				yval = ymin;
			if (yval > ymax)
				yval = ymax;
			gtk_adjustment_set_value(ptr->vadj, yval);
			break;
		case GDK_SCROLL_LEFT:
			xstep = - xstep;
			/* fall through */
		case GDK_SCROLL_RIGHT:
			xval += xstep;
			if (xval < xmin)
				xval = xmin;
			if (xval > xmax)
				xval = xmax;
			gtk_adjustment_set_value(ptr->hadj, xval);
			break;
		default:
			break;
	}

	return FALSE;
}

static gboolean
idle_redraw(gpointer user_data)
{
	pdf_browser_contents	*ptr = user_data;
	oppdf_doc	*doc_ptr = ptr->doc_ptr;

	if (ptr->pixmap)
		g_object_unref(ptr->pixmap);

	printf("viewport w %d h %d\n", ptr->img_w, ptr->img_h);

	ptr->pixmap = gdk_pixmap_new(ptr->window1->window, ptr->img_w, ptr->img_h, -1);
	oppdf_compute_scale(ptr);
	oppdf_compute_sizes(ptr);
	oppdf_compute_selection(ptr, ptr->search_page);

	/* We have a "dest" pointer, so it must be the first call */
	if (doc_ptr->dest) {
		oppdf_pages	*pageptr = &doc_ptr->pages[doc_ptr->current_page - 1];
		gdouble		xval, yval;

		ptr->render_in_progress = 1;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(ptr->page_spin), doc_ptr->current_page);
		ptr->render_in_progress = 0;

		/* Shortcut scaling to BESTFIT/single page mode */
		xval = doc_ptr->dest->left * doc_ptr->scale;
		yval = (pageptr->ph - doc_ptr->dest->top) * doc_ptr->scale;

#if 0
		printf("oppdf-handler: initial link rendering: "
			"scale %.2lf img_w %d img_h %d top %.2lf left %.2lf "
			"xval %.2lf yval %.2lf\n",
			doc_ptr->scale, pageptr->img_w, pageptr->img_h,
			doc_ptr->dest->top, doc_ptr->dest->left,
			xval, yval);
#endif

		oppdf_set_scrollbars(ptr, &xval, &yval);

		poppler_dest_free(doc_ptr->dest);
		doc_ptr->dest = NULL;
	} else {
		oppdf_set_scrollbars(ptr, NULL, NULL);
	}

	oppdf_display(ptr);

	return FALSE;
}

gboolean on_viewport_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	pdf_browser_contents	*ptr = user_data;

	if (event->count == 0 && GTK_WIDGET_REALIZED(widget)) {
		GdkWindow	*win;
		int		img_w, img_h, dummy;

#if VIEWPORT
		win = GTK_VIEWPORT(ptr->viewport)->view_window;
#else
		win = GTK_BIN(widget)->child->window;
#endif
		gdk_window_get_geometry(win, &dummy, &dummy, &img_w, &img_h, &dummy);
		//printf("on_viewport_expose_event w: %d h: %d\n", img_w, img_h);

		if (ptr->img_w != img_w || ptr->img_h != img_h) {
			printf("on_viewport_expose_event: viewport size different\n");

			ptr->img_w = img_w;
			ptr->img_h = img_h;

#if 0
			g_idle_add(idle_redraw, ptr);
#else
			idle_redraw(ptr);
#endif
		}
	}

	return TRUE;
}

void
on_scrollbar_adj_changed(GtkAdjustment *adjustment, gpointer user_data)
{
	pdf_browser_contents *ptr = user_data;
	oppdf_doc *doc_ptr = ptr->doc_ptr;
	int	i;

	//printf("%s: %.0lf\n", __FUNCTION__, gtk_adjustment_get_value(adjustment));

	if (doc_ptr->continuous_mode) {
		for (i = 0; i < doc_ptr->n_pages; i++)
		{
			oppdf_pages *pageptr = &doc_ptr->pages[i];
			if (pageptr->offset <= adjustment->value &&
				adjustment->value < pageptr->offset + pageptr->img_h)
				break;
		}

		ptr->render_in_progress = 1;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(ptr->page_spin), i + 1);
		ptr->render_in_progress = 0;
	}

	ptr->vadj_val = gtk_adjustment_get_value(ptr->vadj);
	ptr->hadj_val = gtk_adjustment_get_value(ptr->hadj);

	oppdf_display(ptr);
}

gboolean
on_window1_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	pdf_browser_contents	*ptr = user_data;
	int	new_x, new_y, new_w, new_h, new_depth;

#if 0
	/*
	 * Find out the allocated widths of widgets. interface.c::handler_width[]
	 * array must contains these values.
	 */
	if (ptr->handlers_shown) {
		int	i;

		for (i = 0; i < PDF_HANDLERS; i++) {
			GtkAllocation	alloc;
			gtk_widget_get_allocation(ptr->handlers[i], &alloc);
			printf("on_window1_expose_event handler %i width %d\n", i, alloc.width);
		}
	}
#endif

	gdk_window_get_geometry(ptr->window1->window, &new_x, &new_y, &new_w, &new_h, &new_depth);
	if (new_w != ptr->w || new_h != ptr->h) {
		ptr->x = new_x;
		ptr->y = new_y;
		ptr->w = new_w;
		ptr->h = new_h;

		create_pdf_browser_contents(ptr);
	}

	return FALSE;
}
