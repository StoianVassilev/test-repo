/*
 * DS18B20 Temperature Sensor Reader with GTK GUI and History Graph
 * Reads temperature from DS18B20 sensor via 1-Wire interface.
 *
 * Setup:
 * 1. Enable 1-Wire: sudo raspi-config -> Interface Options -> 1-Wire -> Enable
 * 2. Or add to /boot/config.txt: dtoverlay=w1-gpio
 * 3. Connect DS18B20: VCC->3.3V, GND->GND, DATA->GPIO4 (with 4.7k pull-up resistor)
 * 4. Reboot the Pi
 *
 * Compile: gcc -o ds18b20_temp_gui ds18b20_temp_gui.c `pkg-config --cflags --libs gtk+-3.0` -lm
 * Run: ./ds18b20_temp_gui
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <time.h>

#define BASE_DIR "/sys/bus/w1/devices/"
#define HISTORY_SIZE 40000

static GtkWidget *temp_label;
static GtkWidget *temp_f_label;
static GtkWidget *status_label;
static GtkWidget *graph_area;
static GtkWidget *minmax_label;
static GtkWidget *indicator_area;
static char sensor_path[256] = "";

/* Temperature history */
static float temp_history[HISTORY_SIZE];
static time_t time_history[HISTORY_SIZE];
static int history_index = 0;
static int history_count = 0;
static float temp_min = 15.0f;
static float temp_max = 35.0f;

/* Recorded min/max temperatures */
static float recorded_min = 1000.0f;
static float recorded_max = -1000.0f;
static time_t recorded_min_time = 0;
static time_t recorded_max_time = 0;

/* Indicator state (alternates between 0 and 1) */
static int indicator_state = 0;

/* Find the DS18B20 sensor device path */
int find_sensor(void) {
    DIR *dir;
    struct dirent *entry;
    
    dir = opendir(BASE_DIR);
    if (dir == NULL) {
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "28", 2) == 0) {
            snprintf(sensor_path, sizeof(sensor_path), "%s%s/w1_slave", BASE_DIR, entry->d_name);
            closedir(dir);
            return 1;
        }
    }
    
    closedir(dir);
    return 0;
}

/* Read temperature from sensor */
int read_temperature(float *temp_c, float *temp_f) {
    FILE *fp;
    char line[256];
    char *temp_str;
    int temp_raw;
    
    if (strlen(sensor_path) == 0) {
        return 0;
    }
    
    fp = fopen(sensor_path, "r");
    if (fp == NULL) {
        return 0;
    }
    
    /* Read first line and check CRC */
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return 0;
    }
    
    if (strstr(line, "YES") == NULL) {
        fclose(fp);
        return 0;
    }
    
    /* Read second line with temperature */
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    
    /* Find t= and parse temperature */
    temp_str = strstr(line, "t=");
    if (temp_str == NULL) {
        return 0;
    }
    
    temp_raw = atoi(temp_str + 2);
    *temp_c = temp_raw / 1000.0f;
    *temp_f = (*temp_c * 9.0f / 5.0f) + 32.0f;
    
    return 1;
}

/* Add temperature to history */
void add_to_history(float temp_c) {
    temp_history[history_index] = temp_c;
    time_history[history_index] = time(NULL);
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
    
    /* Update min/max with some margin */
    float current_min = temp_c;
    float current_max = temp_c;
    for (int i = 0; i < history_count; i++) {
        if (temp_history[i] < current_min) current_min = temp_history[i];
        if (temp_history[i] > current_max) current_max = temp_history[i];
    }
    temp_min = floorf(current_min) - 1.0f;
    temp_max = ceilf(current_max) + 1.0f;
    if (temp_max - temp_min < 2.0f) {
        temp_max = temp_min + 2.0f;
    }
}

/* Get color based on temperature */
void get_temp_color_rgb(float temp_c, double *r, double *g, double *b) {
    if (temp_c < 20.0f) {
        *r = 0.20; *g = 0.60; *b = 0.86;  /* Blue - cold */
    } else if (temp_c < 31.0f) {
        *r = 0.18; *g = 0.80; *b = 0.44;  /* Green - normal */
    } else {
        *r = 0.91; *g = 0.30; *b = 0.24;  /* Red - hot */
    }
}

const char* get_temp_color(float temp_c) {
    if (temp_c < 20.0f) {
        return "#3498db";  /* Blue - cold */
    } else if (temp_c < 30.0f) {
        return "#2ecc71";  /* Green - normal */
    } else {
        return "#e74c3c";  /* Red - hot */
    }
}

/* Draw the reading indicator */
gboolean draw_indicator(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    
    /* Background */
    cairo_set_source_rgb(cr, 0.17, 0.24, 0.31);  /* #2c3e50 */
    cairo_paint(cr);
    
    /* Draw circle - alternate between light and dark green */
    if (indicator_state) {
        cairo_set_source_rgb(cr, 0.18, 0.80, 0.44);  /* Light green #2ecc71 */
    } else {
        cairo_set_source_rgb(cr, 0.10, 0.50, 0.28);  /* Dark green */
    }
    
    cairo_arc(cr, width / 2, height / 2, 5, 0, 2 * G_PI);
    cairo_fill(cr);
    
    return FALSE;
}

/* Draw the temperature history graph */
gboolean draw_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int margin_left = 40;
    int margin_top = 30;  /* Moved 10 pixels up (was 40) */
    int margin_right = 40;
    int margin_bottom = 50;
    int graph_width = width - margin_left - margin_right;
    int graph_height = height - margin_top - margin_bottom;
    
    /* Background */
    cairo_set_source_rgb(cr, 0.17, 0.24, 0.31);  /* #2c3e50 */
    cairo_paint(cr);
    
    /* Graph background */
    cairo_set_source_rgb(cr, 0.10, 0.15, 0.20);
    cairo_rectangle(cr, margin_left, margin_top, graph_width, graph_height);
    cairo_fill(cr);
    
    /* Draw grid lines */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  /* White */
    cairo_set_line_width(cr, 1.0);
    double dashes[] = {4.0, 4.0};  /* Dotted line pattern */
    cairo_set_dash(cr, dashes, 2, 0);
    
    /* Horizontal grid lines and labels */
    int num_lines = 5;
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);
    
    for (int i = 0; i <= num_lines; i++) {
        float temp = temp_min + (temp_max - temp_min) * (num_lines - i) / num_lines;
        int y = margin_top + (graph_height * i / num_lines);
        
        cairo_move_to(cr, margin_left, y);
        cairo_line_to(cr, margin_left + graph_width, y);
        cairo_stroke(cr);
        
        /* Temperature label */
        char label[16];
        snprintf(label, sizeof(label), "%.0f°C", temp);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  /* White */
        cairo_move_to(cr, 5, y + 4);
        cairo_show_text(cr, label);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  /* White */
    }
    
    /* Draw vertical grid lines with time labels at 10-minute intervals */
    if (history_count > 1) {
        int start_idx = (history_index - history_count + HISTORY_SIZE) % HISTORY_SIZE;
        time_t last_10min_mark = 0;  /* Track last 10-min boundary drawn */
        
        for (int i = 0; i < history_count; i++) {
            int idx = (start_idx + i) % HISTORY_SIZE;
            time_t t = time_history[idx];
            
            /* Check if this timestamp falls on a 10-minute boundary */
            struct tm *tm_info = localtime(&t);
            int minutes = tm_info->tm_min;
            int seconds = tm_info->tm_sec;
            
            /* Check if we're at an even 10-minute mark (within 1 second tolerance) */
            if (minutes % 10 == 0 && seconds < 2) {
                /* Calculate the 10-minute mark (floor to exact 10-min) */
                time_t this_10min_mark = t - seconds - (minutes % 10) * 60;
                
                /* Only draw if this is a new 10-minute boundary */
                if (this_10min_mark != last_10min_mark) {
                    double x = margin_left + (graph_width * i / (HISTORY_SIZE - 1));
                    
                    /* Draw vertical dotted line */
                    cairo_move_to(cr, x, margin_top);
                    cairo_line_to(cr, x, margin_top + graph_height);
                    cairo_stroke(cr);
                    
                    /* Draw time label (rotated 90 degrees) */
                    char time_label[16];
                    strftime(time_label, sizeof(time_label), "%H:%M", tm_info);
                    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  /* White */
                    cairo_save(cr);
                    cairo_translate(cr, x + 4, margin_top + graph_height + 45);
                    cairo_rotate(cr, -G_PI / 2);  /* Rotate 90 degrees counter-clockwise */
                    cairo_move_to(cr, 0, 0);
                    cairo_show_text(cr, time_label);
                    cairo_restore(cr);
                    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  /* White */
                    
                    last_10min_mark = this_10min_mark;
                }
            }
        }
    }
    
    /* Reset dash to solid for other drawing */
    cairo_set_dash(cr, NULL, 0, 0);
    
    /* Draw temperature line */
    if (history_count > 1) {
        cairo_set_line_width(cr, 2.0);
        
        int start_idx = (history_index - history_count + HISTORY_SIZE) % HISTORY_SIZE;
        
        for (int i = 0; i < history_count - 1; i++) {
            int idx1 = (start_idx + i) % HISTORY_SIZE;
            int idx2 = (start_idx + i + 1) % HISTORY_SIZE;
            
            float t1 = temp_history[idx1];
            float t2 = temp_history[idx2];
            
            double x1 = margin_left + (graph_width * i / (HISTORY_SIZE - 1));
            double y1 = margin_top + graph_height - (graph_height * (t1 - temp_min) / (temp_max - temp_min));
            double x2 = margin_left + (graph_width * (i + 1) / (HISTORY_SIZE - 1));
            double y2 = margin_top + graph_height - (graph_height * (t2 - temp_min) / (temp_max - temp_min));
            
            /* Get color for this segment */
            double r, g, b;
            get_temp_color_rgb(t2, &r, &g, &b);
            cairo_set_source_rgb(cr, r, g, b);
            
            cairo_move_to(cr, x1, y1);
            cairo_line_to(cr, x2, y2);
            cairo_stroke(cr);
        }
        
        /* Draw current point */
        int last_idx = (history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        float last_temp = temp_history[last_idx];
        double x = margin_left + (graph_width * (history_count - 1) / (HISTORY_SIZE - 1));
        double y = margin_top + graph_height - (graph_height * (last_temp - temp_min) / (temp_max - temp_min));
        
        double r, g, b;
        get_temp_color_rgb(last_temp, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_arc(cr, x, y, 1, 0, 2 * G_PI);
        cairo_fill(cr);
    }
    
    /* Draw border */
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, margin_left, margin_top, graph_width, graph_height);
    cairo_stroke(cr);
    
    return FALSE;
}

/* Update temperature display - called every second */
gboolean update_temperature(gpointer data) {
    float temp_c, temp_f;
    char temp_str[64];
    char markup[256];
    
    if (read_temperature(&temp_c, &temp_f)) {
        const char *color = get_temp_color(temp_c);
        
        snprintf(markup, sizeof(markup), 
                 "<span font='48' weight='bold' foreground='%s'>%.1f°C</span>", 
                 color, temp_c);
        gtk_label_set_markup(GTK_LABEL(temp_label), markup);
        
        snprintf(temp_str, sizeof(temp_str), "%.1f°F", temp_f);
        gtk_label_set_text(GTK_LABEL(temp_f_label), temp_str);
        
        /* Update recorded min/max */
        if (temp_c < recorded_min) {
            recorded_min = temp_c;
            recorded_min_time = time(NULL);
        }
        if (temp_c > recorded_max) {
            recorded_max = temp_c;
            recorded_max_time = time(NULL);
        }
        
        /* Update min/max label with times */
        char minmax_markup[512];
        char min_time_str[16] = "--:--:--";
        char max_time_str[16] = "--:--:--";
        if (recorded_min_time > 0) {
            struct tm *tm_min = localtime(&recorded_min_time);
            strftime(min_time_str, sizeof(min_time_str), "%H:%M:%S", tm_min);
        }
        if (recorded_max_time > 0) {
            struct tm *tm_max = localtime(&recorded_max_time);
            strftime(max_time_str, sizeof(max_time_str), "%H:%M:%S", tm_max);
        }
        snprintf(minmax_markup, sizeof(minmax_markup),
                 "<span font='12' foreground='#e74c3c'>Max: %.1f°C @ %s</span>\n"
                 "<span font='12' foreground='#3498db'>Min: %.1f°C @ %s</span>",
                 recorded_max, max_time_str, recorded_min, min_time_str);
        gtk_label_set_markup(GTK_LABEL(minmax_label), minmax_markup);
        
        gtk_label_set_text(GTK_LABEL(status_label), "");
        
        /* Add to history and redraw graph */
        add_to_history(temp_c);
        gtk_widget_queue_draw(graph_area);
        
        /* Toggle indicator and redraw */
        indicator_state = !indicator_state;
        gtk_widget_queue_draw(indicator_area);
    } else {
        gtk_label_set_text(GTK_LABEL(status_label), "Read error");
    }
    
    return TRUE;  /* Continue calling */
}

/* Apply CSS styling */
void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        "window { background-color: #2c3e50; }"
        ".title { color: white; font-size: 14px; font-weight: bold; }"
        ".temp-f { color: #95a5a6; font-size: 18px; }"
        ".status { color: #e74c3c; font-size: 10px; }";
    
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *title_label;
    
    gtk_init(&argc, &argv);
    
    /* Initialize history */
    memset(temp_history, 0, sizeof(temp_history));
    
    /* Find sensor */
    if (!find_sensor()) {
        fprintf(stderr, "No DS18B20 sensor found! Check wiring and 1-Wire is enabled.\n");
    }
    
    /* Apply CSS */
    apply_css();
    
    /* Create window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "DS18B20 Temperature Monitor");
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    if (monitor == NULL) {
        monitor = gdk_display_get_monitor(display, 0);
    }
    int screen_width = 800;  /* Default fallback */
    if (monitor != NULL) {
        GdkRectangle geometry;
        gdk_monitor_get_geometry(monitor, &geometry);
        screen_width = geometry.width;
    }
    gtk_window_set_default_size(GTK_WINDOW(window), screen_width, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    /* Create vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    /* Title label */
    title_label = gtk_label_new("DS18B20 Temperature");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "title");
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 5);
    
    /* Horizontal box for temperature and min/max */
    GtkWidget *temp_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(vbox), temp_hbox, FALSE, FALSE, 5);
    
    /* Sub-box for indicator + temperature (tightly spaced) */
    GtkWidget *indicator_temp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(temp_hbox), indicator_temp_box, TRUE, FALSE, 0);
    
    /* Reading indicator (left of temperature) */
    indicator_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(indicator_area, 15, 15);
    gtk_widget_set_valign(indicator_area, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(indicator_area, 20);
    g_signal_connect(indicator_area, "draw", G_CALLBACK(draw_indicator), NULL);
    gtk_box_pack_start(GTK_BOX(indicator_temp_box), indicator_area, FALSE, FALSE, 0);
    
    /* Temperature label (Celsius) */
    temp_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(temp_label), 
        "<span font='48' weight='bold' foreground='#3498db'>--.-°C</span>");
    gtk_box_pack_start(GTK_BOX(indicator_temp_box), temp_label, FALSE, FALSE, 0);
    
    /* Min/Max label */
    minmax_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(minmax_label),
        "<span font='12' foreground='#e74c3c'>Max: --.-°C @ --:--:--</span>\n"
        "<span font='12' foreground='#3498db'>Min: --.-°C @ --:--:--</span>");
    gtk_widget_set_valign(minmax_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(minmax_label, 0);
    gtk_box_pack_start(GTK_BOX(indicator_temp_box), minmax_label, FALSE, FALSE, 0);
    
    /* Temperature label (Fahrenheit) */
    temp_f_label = gtk_label_new("--.-°F");
    gtk_style_context_add_class(gtk_widget_get_style_context(temp_f_label), "temp-f");
    gtk_box_pack_start(GTK_BOX(vbox), temp_f_label, FALSE, FALSE, 0);
    
    /* Status label */
    status_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(status_label), "status");
    if (strlen(sensor_path) == 0) {
        gtk_label_set_text(GTK_LABEL(status_label), "No sensor found! Check wiring.");
    }
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 2);
    
    /* Graph drawing area */
    graph_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(graph_area, 400, 150);
    g_signal_connect(graph_area, "draw", G_CALLBACK(draw_graph), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), graph_area, TRUE, TRUE, 10);
    
    /* Update as fast as possible */
    g_idle_add(update_temperature, NULL);
    
    /* Show window */
    gtk_widget_show_all(window);
    
    gtk_main();
    
    return 0;
}
