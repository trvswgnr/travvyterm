#include <gtk/gtk.h>
#include <vte/vte.h>

/**
 * adjust the font size of a widget.
 * @param widget the widget to adjust the font size of.
 * @param increase TRUE if the font size should be increased, FALSE if it should be decreased.
 */
void adjust_font_size(GtkWidget *widget, gboolean increase, PangoFontDescription *font_desc)
{
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gint size = pango_font_description_get_size(font_desc);

    pango_font_description_set_size(font_desc, size + PANGO_SCALE * (increase ? 2 : -2));

    const char *font_family = pango_font_description_get_family(font_desc);
    int font_size = pango_font_description_get_size(font_desc) / PANGO_SCALE;

    gchar *font_css = g_strdup_printf("* { font-family: %s, monospace; font-size: %dpt; }", font_family, font_size);

    gtk_css_provider_load_from_data(css_provider, font_css, -1, NULL);
    g_free(font_css);

    GtkStyleContext *style_context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    pango_font_description_free(font_desc);
    g_object_unref(css_provider);
}

/**
 * called when a key is pressed in the terminal window.
 * @param widget the terminal window.
 * @param event the key press event.
 * @param user_data the user data.
 * @return TRUE if the key press event was handled, FALSE otherwise.
 */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    if (event->type == GDK_KEY_PRESS)
    {
        // if the last key pressed was Ctrl, then check if this key is a + or - (and if so, change the font size)
        if (event->state & GDK_CONTROL_MASK)
        {
            // get the font description from the terminal window
            PangoFontDescription *font_desc = pango_font_description_from_string(pango_font_description_to_string(vte_terminal_get_font(VTE_TERMINAL(widget))));

            if (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal)
            {
                // increase the font size
                adjust_font_size(widget, TRUE, font_desc);

                return TRUE;
            }
            else if (event->keyval == GDK_KEY_minus)
            {
                // decrease the font size
                adjust_font_size(widget, FALSE, font_desc);

                return TRUE;
            }
            // if it's "c" then copy the text
            else if (event->keyval == GDK_KEY_c)
            {
                // get the text that the user selected in the terminal window
                GtkWidget *terminal = widget;
                gchar *text = vte_terminal_get_text(VTE_TERMINAL(terminal), NULL, NULL, NULL);

                // put the text in the clipboard
                GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
                gtk_clipboard_set_text(clipboard, text, -1);

                // free the text
                g_free(text);

                return TRUE;
            }
            // if it's "v" then paste in the text
            else if (event->keyval == GDK_KEY_v)
            {
                // get the text from the clipboard
                GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
                gchar *text = gtk_clipboard_wait_for_text(clipboard);

                // if the text is not NULL, then paste it into the terminal window
                if (text != NULL)
                {
                    vte_terminal_feed_child(VTE_TERMINAL(widget), text, strlen(text));
                }

                // free the text
                g_free(text);

                return TRUE;
            }
        }

        if (event->keyval == GDK_KEY_Return)
        {
            // get the text that the user typed in the terminal window
            GtkWidget *terminal = widget;
            gchar *text = vte_terminal_get_text(VTE_TERMINAL(terminal), NULL, NULL, NULL);

            // free the text
            g_free(text);
        }
    }

    return FALSE;
}

/**
 * checks if the ComicCodeThin Nerd Font is installed.
 *
 * if it is not installed, then show an alert dialog and exit the application.
 */
void check_for_font(GtkWidget *window)
{
    // check if the ComicCodeThin Nerd Font is installed
    PangoFontFamily **families;
    int n_families;
    pango_context_list_families(gtk_widget_get_pango_context(window), &families, &n_families);

    gboolean font_installed = FALSE;
    for (int i = 0; i < n_families; i++)
    {
        if (strcmp(pango_font_family_get_name(families[i]), "ComicCodeThin Nerd Font") == 0)
        {
            font_installed = TRUE;
            break;
        }
    }

    if (!font_installed)
    {
        // alert the user that the ComicCodeThin Nerd Font is not installed
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "The ComicCodeThin Nerd Font is not installed. Please install it and try again.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
}

/**
 * called when the application is activated.
 * @param app The application.
 * @param user_data The user data.
 */
static void on_activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "travvyterm");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

    // check_for_font(window);

    // use ComicCodeThin Nerd Font and make it larger
    // PangoFontDescription *font_desc = pango_font_description_from_string("ComicCodeThin Nerd Font 18");

    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 18");

    GtkWidget *terminal = vte_terminal_new();
    gtk_container_add(GTK_CONTAINER(window), terminal);

    // start a bash shell in the terminal window
    gchar **envp = g_get_environ();
    const gchar *shell = g_environ_getenv(envp, "SHELL");
    // const gchar *shell = "/bin/bash";
    gchar **command = (gchar *[]){g_strdup(shell), NULL};
    g_strfreev(envp);

    vte_terminal_spawn_async(
        VTE_TERMINAL(terminal), VTE_PTY_DEFAULT, NULL, command,
        NULL, 0, NULL, NULL, NULL, -1, NULL, NULL, NULL);

    // set the font
    vte_terminal_set_font(VTE_TERMINAL(terminal), font_desc);

    // connect to the key-press-event signal
    g_signal_connect(terminal, "key-press-event", G_CALLBACK(on_key_press), NULL);

    gtk_widget_show_all(window);
}

/**
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @return The exit status.
 */
int main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("com.techsavvytravvy.travvyterm", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
