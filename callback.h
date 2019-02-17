#include <gtk/gtk.h>
#include <TFile.h>
#include <TTree.h>

/*Struct to pass data to on_button2_clicked */
typedef struct{
  GtkWidget *window1T;
  GtkWidget *window2T;
}button2_data;

/*Struct to pass data to on_button5_clicked */
typedef struct{
  TFile *file;
  TTree *tree;
  std::ofstream *file_txt;
  uint32_t **read_buf; 
  uint32_t **write_buf;
}button5_data;




void on_button5_clicked(GtkButton *button,gpointer user_data);
void on_button2_clicked (GtkButton *button,gpointer data);
void on_button3_clicked(GtkButton *button,gpointer user_data);
void on_button4_clicked(GtkButton *button,gpointer user_data);
void on_button6_clicked(GtkButton *button,gpointer user_data);



gboolean delete_event( GtkWidget *widget,GdkEvent *event,gpointer data);

/*Configure window*/
void configure_enable_callback(GtkWidget *widget,gpointer data);
void configure_save_callback(GtkWidget *widget,gpointer data);
void configure_load_callback(GtkWidget *widget,gpointer data);
