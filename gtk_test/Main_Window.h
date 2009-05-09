/* 
 * File:   main_window.h
 * Author: hcy
 *
 */

#ifndef _MAIN_WINDOW_H
#define	_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include <stdlib.h>
#include "db_connection.h"

namespace Main_Window {
    
    void main_window(int argc, char *argv[]);

    void callback(GtkWidget *widget,
            gpointer data);
    gboolean delete_event(GtkWidget *widget,
            GdkEvent *event,
            gpointer data);

    void show();
}
#endif	/* _MAIN_WINDOW_H */

