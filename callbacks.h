#include <gtk/gtk.h>

gboolean
on_window1_delete_event			(GtkWidget	*widget,
					GdkEvent	*event,
					gpointer	user_data);

void
on_firstpage_button_clicked		(GtkButton	*button,
					gpointer	user_data);

void
on_prevpage_button_clicked		(GtkButton	*button,
					gpointer	user_data);

void
on_nextpage_button_clicked		(GtkButton	*button,
					gpointer	user_data);

void
on_lastpage_button_clicked		(GtkButton	*button,
					gpointer	user_data);

void
on_page_spin_value_changed		(GtkSpinButton	*spinbutton,
					gpointer	user_data);

void
on_scale_combo_changed			(GtkComboBox	*combobox,
					gpointer	user_data);

void
on_print_button_clicked			(GtkButton	*button,
					gpointer	user_data);

void
on_findprev_button_clicked		(GtkButton	*button,
					gpointer	user_data);

void
on_findnext_button_clicked		(GtkButton	*button,
					gpointer	user_data);

void
on_iconview_selection_changed		(GtkIconView	*iconview,
					gpointer	user_data);

void
on_find_entry_activate			(GtkEntry	*entry,
					gpointer	user_data);

void
on_find_entry_delete			(GtkEntry	*entry,
					GtkDeleteType	arg1,
					gint		arg2,
					gpointer	user_data);

void
on_find_entry_insert			(GtkEntry	*entry,
					gchar		*arg1,
					gpointer	user_data);

gboolean
on_viewport_button_press_event		(GtkWidget	*widget,
					GdkEventButton	*event,
					gpointer	user_data);

gboolean
on_viewport_button_release_event	(GtkWidget	*widget,
					GdkEventButton	*event,
					gpointer	user_data);

gboolean
on_viewport_motion_notify_event		(GtkWidget	*widget,
					GdkEventMotion	*event,
					gpointer	user_data);

gboolean on_viewport_scroll_event	(GtkWidget	*widget,
					GdkEvent	*event,
					gpointer user_data);

gboolean
on_viewport_expose_event		(GtkWidget	*widget,
					GdkEventExpose	*event,
					gpointer	user_data);

void
on_scrollbar_adj_changed		(GtkAdjustment	*adjustment,
					gpointer	user_data);

gboolean
on_window1_expose_event			(GtkWidget	*widget,
					GdkEventExpose	*event,
					gpointer	user_data);
