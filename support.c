#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include <gtk/gtk.h>

#include "support.h"

GtkWidget* lookup_widget(GtkWidget *widget, const gchar *widget_name)
{
  GtkWidget *parent, *found_widget;
  printf("lookup: ");
  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (!parent)
        parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  if (!found_widget) {
    g_warning ("Widget not found: %s \n", widget_name);
    return 0;
  }
  else{
#ifdef DEBUG 
    printf("Widget found :%s \n",widget_name);
#endif
    return found_widget;
  }
}

long get_time()
{
  long time_ms;
  struct timeval t1;
   struct timezone tz;
   gettimeofday(&t1, &tz);
   time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
   return time_ms;
}
