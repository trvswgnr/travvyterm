#include <gtk/gtk.h>
#include <vte/vte.h>

/**
 * recursively modify the font of a widget and all of its children.
 * @param widget the widget to modify the font of.
 * @param font_desc the font description to use.
 */
void widget_modify_font(GtkWidget *widget, PangoFontDescription *font_desc) {
    gtk_widget_override_font(widget, font_desc);
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(widget));
    if (child && GTK_IS_WIDGET(child)) {
        widget_modify_font(child, font_desc);
    }
}

/**
 * called when a key is pressed in the terminal window.
 * @param widget the terminal window.
 * @param event the key press event.
 * @param user_data the user data.
 * @return TRUE if the key press event was handled, FALSE otherwise.
 */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->type == GDK_KEY_PRESS) {
        // if the last key pressed was Ctrl, then check if this key is a + or - (and if so, change the font size)
        if (event->state & GDK_CONTROL_MASK) {
            if (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal) {
                // increment the font size by 2
                PangoFontDescription *font_desc = gtk_widget_get_style(widget)->font_desc;
                gint size = pango_font_description_get_size(font_desc);
                pango_font_description_set_size(font_desc, size + PANGO_SCALE * 2);
                widget_modify_font(widget, font_desc);

                return TRUE;
            } else if (event->keyval == GDK_KEY_minus) {
                // decrement the font size by 2
                PangoFontDescription *font_desc = gtk_widget_get_style(widget)->font_desc;
                gint size = pango_font_description_get_size(font_desc);
                pango_font_description_set_size(font_desc, size - PANGO_SCALE * 2);
                widget_modify_font(widget, font_desc);

                return TRUE;
            }
        }



        if (event->keyval == GDK_KEY_Return) {
            // get the text that the user typed in the terminal window
            GtkWidget *terminal = widget;
            gchar *text = vte_terminal_get_text(VTE_TERMINAL(terminal), NULL, NULL, NULL);

            // print the text to stdout
            g_print("%s", text);

            // free the text
            g_free(text);
        }
    }

    return FALSE;
}

/**
 * called when the application is activated.
 * @param app The application.
 * @param user_data The user data.
 */
static void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "travvyterm");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

    // make the font size larger
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 16");
    widget_modify_font(GTK_WIDGET(window), font_desc);

    GtkWidget *terminal = vte_terminal_new();
    gtk_container_add(GTK_CONTAINER(window), terminal);

    // start a bash shell in the terminal window
    gchar **envp = g_get_environ();
    gchar **command = (gchar *[]){g_strdup(g_environ_getenv(envp, "SHELL")), NULL};
    g_strfreev(envp);
    vte_terminal_spawn_async(VTE_TERMINAL(terminal), VTE_PTY_DEFAULT, NULL, command, NULL, 0, NULL, NULL, NULL, -1, NULL, NULL, NULL);

    g_signal_connect(terminal, "key-press-event", G_CALLBACK(on_key_press), NULL);

    gtk_widget_show_all(window);
}

/**
 * entry point
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @return The exit status.
 */
int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("com.techsavvytravvy.travvyterm", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
