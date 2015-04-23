/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * Functions h_strdup_cprintf and h_strconcat are based on GLib version 2.4.6
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

#include <config.h>

#include <report.h>
#include <string.h>
#include <shell.h>
#include <iconcache.h>
#include "hardinfo.h"
#include <gtk/gtk.h>

#include <binreloc.h>

#include <sys/stat.h>
#include <sys/types.h>

#define KiB 1024
#define MiB 1048576
#define GiB 1073741824

gchar *find_program(gchar *program_name)
{
    int i;
    char *temp;
    static GHashTable *cache = NULL;
    const char *path[] = { "/bin", "/sbin",
		           "/usr/bin", "/usr/sbin",
		           "/usr/local/bin", "/usr/local/sbin",
		           NULL };
    
    /* we don't need to call stat() every time: cache the results */
    if (!cache) {
    	cache = g_hash_table_new(g_str_hash, g_str_equal);
    } else if ((temp = g_hash_table_lookup(cache, program_name))) {
    	return g_strdup(temp);
    }
    
    for (i = 0; path[i]; i++) {
    	temp = g_build_filename(path[i], program_name, NULL);
    	
    	if (g_file_test(temp, G_FILE_TEST_IS_EXECUTABLE)) {
    		g_hash_table_insert(cache, program_name, g_strdup(temp));
		return temp;
    	}
    	
    	g_free(temp);
    }
    
    /* our search has failed; use GLib's search (which uses $PATH env var) */
    if ((temp = g_find_program_in_path(program_name))) {
    	g_hash_table_insert(cache, program_name, g_strdup(temp));
    	return temp;
    }

    return NULL;
}

gchar *seconds_to_string(unsigned int seconds)
{
    unsigned int hours, minutes, days;

    minutes = seconds / 60;
    hours = minutes / 60;
    minutes %= 60;
    days = hours / 24;
    hours %= 24;

#define plural(x) ((x > 1) ? "s" : "")

    if (days < 1) {
	if (hours < 1) {
	    return g_strdup_printf("%d minute%s", minutes,
				   plural(minutes));
	} else {
	    return g_strdup_printf("%d hour%s, %d minute%s",
				   hours,
				   plural(hours), minutes,
				   plural(minutes));
	}
    }

    return g_strdup_printf("%d day%s, %d hour%s and %d minute%s",
			   days, plural(days), hours,
			   plural(hours), minutes, plural(minutes));
}

inline gchar *size_human_readable(gfloat size)
{
    if (size < KiB)
	return g_strdup_printf("%.1f B", size);
    if (size < MiB)
	return g_strdup_printf("%.1f KiB", size / KiB);
    if (size < GiB)
	return g_strdup_printf("%.1f MiB", size / MiB);

    return g_strdup_printf("%.1f GiB", size / GiB);
}

inline char *strend(gchar * str, gchar chr)
{
    if (!str)
	return NULL;

    char *p;
    if ((p = strchr(str, chr)))
	*p = 0;

    return str;
}

inline void remove_quotes(gchar * str)
{
    if (!str)
	return;

    while (*str == '"')
	*(str++) = ' ';

    strend(str, '"');
}

inline void remove_linefeed(gchar * str)
{
    strend(str, '\n');
}

void widget_set_cursor(GtkWidget * widget, GdkCursorType cursor_type)
{
    GdkCursor *cursor;

    if ((cursor = gdk_cursor_new(cursor_type))) {
        gdk_window_set_cursor(GDK_WINDOW(widget->window), cursor);
        gdk_display_flush(gtk_widget_get_display(widget));
        gdk_cursor_unref(cursor);
    }

    while (gtk_events_pending())
	gtk_main_iteration();
}

static gboolean __nonblock_cb(gpointer data)
{
    gtk_main_quit();
    return FALSE;
}

void nonblock_sleep(guint msec)
{
    g_timeout_add(msec, (GSourceFunc) __nonblock_cb, NULL);
    gtk_main();
}

static void __expand_cb(GtkWidget * widget, gpointer data)
{
    if (GTK_IS_EXPANDER(widget)) {
	gtk_expander_set_expanded(GTK_EXPANDER(widget), TRUE);
    } else if (GTK_IS_CONTAINER(widget)) {
	gtk_container_foreach(GTK_CONTAINER(widget),
			      (GtkCallback) __expand_cb, NULL);
    }
}

void file_chooser_open_expander(GtkWidget * chooser)
{
    gtk_container_foreach(GTK_CONTAINER(chooser),
			  (GtkCallback) __expand_cb, NULL);
}

void file_chooser_add_filters(GtkWidget * chooser, FileTypes * filters)
{
    GtkFileFilter *filter;
    gint i;

    for (i = 0; filters[i].name; i++) {
	filter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(filter, filters[i].mime_type);
	gtk_file_filter_set_name(filter, filters[i].name);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
    }
}

gchar *file_chooser_get_extension(GtkWidget * chooser, FileTypes * filters)
{
    GtkFileFilter *filter;
    const gchar *filter_name;
    gint i;

    filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(chooser));
    filter_name = gtk_file_filter_get_name(filter);
    for (i = 0; filters[i].name; i++) {
	if (g_str_equal(filter_name, filters[i].name)) {
	    return filters[i].extension;
	}
    }

    return NULL;
}

gpointer file_types_get_data_by_name(FileTypes * filters, gchar * filename)
{
    gint i;

    for (i = 0; filters[i].name; i++) {
	if (g_str_has_suffix(filename, filters[i].extension)) {
	    return filters[i].data;
	}
    }

    return NULL;
}

gchar *file_chooser_build_filename(GtkWidget * chooser, gchar * extension)
{
    gchar *filename =
	gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    gchar *retval;

    if (g_str_has_suffix(filename, extension)) {
	return filename;
    }

    retval = g_strconcat(filename, extension, NULL);
    g_free(filename);

    return retval;
}
void parameters_init(int *argc, char ***argv, ProgramParameters * param)
{
	if (*argc >= 2) {
		g_print("Unrecognized arguments.\n"
						"Try ``%s --help'' for more information.\n", *(argv)[0]);
		exit(1);
	}

	param->argv0 = *(argv)[0];

	gchar *confdir = g_build_filename(g_get_home_dir(), ".hardinfo", NULL);
	if (!g_file_test(confdir, G_FILE_TEST_EXISTS)) {
		mkdir(confdir, 0744);
	}
	g_free(confdir);
}
gboolean binreloc_init(gboolean try_hardcoded)
{
    GError *error = NULL;
    gchar *tmp;

    DEBUG("initializing binreloc (hardcoded = %d)", try_hardcoded);

    /* If the runtime data directories we previously found, don't even try
       to find them again. */
    if (params.path_data && params.path_lib) {
	DEBUG("data and lib path already found.");
	return TRUE;
    }

    if (try_hardcoded || !gbr_init(&error)) {
	/* We were asked to try hardcoded paths or BinReloc failed to initialize. */
	params.path_data = g_strdup(PREFIX);
	params.path_lib = g_strdup(LIBPREFIX);

	if (error) {
	    g_error_free(error);
	}

	DEBUG("%strying hardcoded paths.",
	      try_hardcoded ? "" : "binreloc init failed. ");
    } else {
	/* If we were able to initialize BinReloc, build the default data
	   and library paths. */
	DEBUG("done, trying to use binreloc paths.");

	tmp = gbr_find_data_dir(PREFIX);
	params.path_data = g_build_filename(tmp, "hardinfo", NULL);
	g_free(tmp);

	tmp = gbr_find_lib_dir(PREFIX);
	params.path_lib = g_build_filename(tmp, "hardinfo", NULL);
	g_free(tmp);
    }

    DEBUG("searching for runtime data on these locations:");
    DEBUG("  lib: %s", params.path_lib);
    DEBUG(" data: %s", params.path_data);

    /* Try to see if the uidefs.xml file isn't missing. This isn't the
       definitive test, but it should do okay for most situations. */
    tmp = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_test(tmp, G_FILE_TEST_EXISTS)) {
	DEBUG("runtime data not found");

	g_free(params.path_data);
	g_free(params.path_lib);
	g_free(tmp);

	params.path_data = params.path_lib = NULL;

	if (try_hardcoded) {
	    /* We tried the hardcoded paths, but still was unable to find the
	       runtime data. Give up. */
	    DEBUG("giving up");
	    return FALSE;
	} else {
	    /* Even though BinReloc worked OK, the runtime data was not found.
	       Try the hardcoded paths. */
	    DEBUG("trying to find elsewhere");
	    return binreloc_init(TRUE);
	}
    }
    g_free(tmp);

    DEBUG("runtime data found!");
    /* We found the runtime data; hope everything is fine */
    return TRUE;
}

static void
log_handler(const gchar * log_domain,
	    GLogLevelFlags log_level,
	    const gchar * message, gpointer user_data)
{

	/* No GUI running: spit the message to the terminal */
	g_print("\n\n*** %s: %s\n\n",
		(log_level & G_LOG_FLAG_FATAL) ? "Error" : "Warning",
		message);

}

void open_url(gchar * url)
{
    const gchar *browsers[] = {
	"xdg-open", "gnome-open", "kfmclient openURL",
	"sensible-browser", "firefox", "epiphany",
	"iceweasel", "seamonkey", "galeon", "mozilla",
	"opera", "konqueror", "netscape", "links -g",
	NULL
    };
    gint i = 0;
    gchar *browser = (gchar *)g_getenv("BROWSER");
    
    if (!browser || *browser == '\0') {
    	browser = (gchar *)browsers[i++];
    }
    
    do {
	gchar *cmdline = g_strdup_printf("%s '%s'", browser, url);

	if (g_spawn_command_line_async(cmdline, NULL)) {
	    g_free(cmdline);
	    return;
	}

	g_free(cmdline);
    	
    	browser = (gchar *)browsers[i++];
    } while (browser);

    g_warning("Couldn't find a Web browser to open URL %s.", url);
}

/* Copyright: Jens Låås, SLU 2002 */
gchar *strreplace(gchar * string, gchar * replace, gchar new_char)
{
    gchar *s;
    for (s = string; *s; s++)
	if (strchr(replace, *s))
	    *s = new_char;

    return string;
}

static GHashTable *__module_methods = NULL;

static void module_register_methods(ShellModule * module)
{
    ShellModuleMethod *(*get_methods) (void);
    gchar *method_name;

    if (__module_methods == NULL) {
	__module_methods = g_hash_table_new(g_str_hash, g_str_equal);
    }

    if (g_module_symbol
	(module->dll, "hi_exported_methods", (gpointer) & get_methods)) {
	ShellModuleMethod *methods;
	
	for (methods = get_methods(); methods->name; methods++) {
	    ShellModuleMethod method = *methods;
	    gchar *name = g_path_get_basename(g_module_name(module->dll));

	    strend(name, '.');

	    method_name = g_strdup_printf("%s::%s", name, method.name);
	    g_hash_table_insert(__module_methods, method_name,
				method.function);
	    g_free(name);
	}
    }

}

gchar *module_call_method(gchar * method)
{
    gchar *(*function) (void);

    if (__module_methods == NULL) {
	return NULL;
    }

    function = g_hash_table_lookup(__module_methods, method);
    return function ? g_strdup(function()) :
	g_strdup_printf("{Unknown method: \"%s\"}", method);
}

/* FIXME: varargs? */
gchar *module_call_method_param(gchar * method, gchar * parameter)
{
    gchar *(*function) (gchar *param);

    if (__module_methods == NULL) {
	return NULL;
    }

    function = g_hash_table_lookup(__module_methods, method);
    return function ? g_strdup(function(parameter)) :
	g_strdup_printf("{Unknown method: \"%s\"}", method);
}

static ShellModule *module_load(gchar * filename)
{
    ShellModule *module;
    gchar *tmp;

    module = g_new0(ShellModule, 1);

    tmp = g_build_filename(params.path_lib, "modules", filename, NULL);
    module->dll = g_module_open(tmp, G_MODULE_BIND_LAZY);
    g_free(tmp);

    if (module->dll) {
	void (*init) (void);
	ModuleEntry *(*get_module_entries) (void);
	gint(*weight_func) (void);
	gchar *(*name_func) (void);
	ModuleEntry *entries;
	gint i = 0;

	if (!g_module_symbol
	    (module->dll, "hi_module_get_entries",
	     (gpointer) & get_module_entries)
	    || !g_module_symbol(module->dll, "hi_module_get_name",
				(gpointer) & name_func)) {
	    goto failed;
	}

	if (g_module_symbol
	    (module->dll, "hi_module_init", (gpointer) & init)) {
	    init();
	}

	g_module_symbol(module->dll, "hi_module_get_weight",
			(gpointer) & weight_func);

	module->weight = weight_func ? weight_func() : 0;
	module->name = name_func();

	entries = get_module_entries();
	while (entries[i].name) {
	    ShellModuleEntry *entry = g_new0(ShellModuleEntry, 1);

	    g_module_symbol(module->dll, "hi_more_info",
			    (gpointer) & (entry->morefunc));
	    g_module_symbol(module->dll, "hi_get_field",
			    (gpointer) & (entry->fieldfunc));
	    g_module_symbol(module->dll, "hi_note_func",
			    (gpointer) & (entry->notefunc));

	    entry->name = entries[i].name;
	    entry->scan_func = entries[i].scan_callback;
	    entry->func = entries[i].callback;
	    entry->number = i;

	    module->entries = g_slist_append(module->entries, entry);

	    i++;
	}

	module_register_methods(module);
    } else {
      failed:
	DEBUG("loading module %s failed", filename);

	g_free(module->name);
	g_free(module);
	module = NULL;
    }

    return module;
}

static gboolean module_in_module_list(gchar * module, gchar ** module_list)
{
    int i = 0;

    if (!module_list)
	return TRUE;

    for (; module_list[i]; i++) {
	if (g_str_equal(module_list[i], module))
	    return TRUE;
    }

    return FALSE;
}

static gint module_cmp(gconstpointer m1, gconstpointer m2)
{
    ShellModule *a = (ShellModule *) m1;
    ShellModule *b = (ShellModule *) m2;

    return a->weight - b->weight;
}

static void module_entry_free(gpointer data, gpointer user_data)
{
    ShellModuleEntry *entry = (ShellModuleEntry *) data;

    if (entry) {
	g_free(entry->name);
	g_object_unref(entry->icon);

	g_free(entry);
    }
}

void module_free(ShellModule * module)
{
    g_free(module->name);
    g_object_unref(module->icon);
    g_module_close(module->dll);

    DEBUG("module_free: module->entries, %p\n", module->entries);
    g_slist_foreach(module->entries, (GFunc) module_entry_free, NULL);
    g_slist_free(module->entries);

    g_free(module);
}

ModuleAbout *module_get_about(ShellModule * module)
{
    ModuleAbout *(*get_about) (void);

    if (g_module_symbol(module->dll, "hi_module_get_about",
			(gpointer) & get_about)) {
	return get_about();
    }

    return NULL;
}

static GSList *modules_check_deps(GSList * modules)
{
    GSList *mm;
    ShellModule *module;

    for (mm = modules; mm; mm = mm->next) {
	gchar **(*get_deps) (void);
	gchar **deps;
	gint i;

	module = (ShellModule *) mm->data;

	if (g_module_symbol(module->dll, "hi_module_get_dependencies",
			    (gpointer) & get_deps)) {
	    for (i = 0, deps = get_deps(); deps[i]; i++) {
		GSList *l;
		ShellModule *m;
		gboolean found = FALSE;

		for (l = modules; l && !found; l = l->next) {
		    m = (ShellModule *) l->data;
		    gchar *name = g_path_get_basename(g_module_name(m->dll));

		    found = g_str_equal(name, deps[i]);
		    g_free(name);
		}

		if (!found) {

			g_error("Module \"%s\" depends on module \"%s\".",
				module->name, deps[i]);

		}
	    }
	}
    }

    return modules;
}

static GSList *modules_load(gchar ** module_list)
{
    GDir *dir;
    GSList *modules = NULL;
    ShellModule *module;
    gchar *filename;

    filename = g_build_filename(params.path_lib, "modules", NULL);
    dir = g_dir_open(filename, 0, NULL);
    g_free(filename);

    if (dir) {
	while ((filename = (gchar *) g_dir_read_name(dir))) {
	    if (g_strrstr(filename, "." G_MODULE_SUFFIX) &&
		module_in_module_list(filename, module_list) &&
		((module = module_load(filename)))) {
		modules = g_slist_prepend(modules, module);
	    }
	}

	g_dir_close(dir);
    }

    modules = modules_check_deps(modules);

    if (g_slist_length(modules) == 0) {
	if (params.use_modules == NULL) {
	    g_error
		("No module could be loaded. Check permissions on \"%s\" and try again.",
		 params.path_lib);
	} else {
	    g_error
		("No module could be loaded. Please use hardinfo -l to list all avai"
		 "lable modules and try again with a valid module list.");

	}
    }

    return g_slist_sort(modules, module_cmp);
}

GSList *modules_load_selected(void)
{
    return modules_load(params.use_modules);
}

GSList *modules_load_all(void)
{
    return modules_load(NULL);
}

gint tree_view_get_visible_height(GtkTreeView * tv)
{
    GtkTreePath *path;
    GdkRectangle rect;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tv);
    gint nrows = 1;

    path = gtk_tree_path_new_first();
    gtk_tree_view_get_cell_area(GTK_TREE_VIEW(tv), path, NULL, &rect);

    /* FIXME: isn't there any easier way to tell the number of rows? */
    gtk_tree_model_get_iter_first(model, &iter);
    do {
	nrows++;
    } while (gtk_tree_model_iter_next(model, &iter));

    gtk_tree_path_free(path);

    return nrows * rect.height;
}

static gboolean __idle_free_do(gpointer ptr)
{
    if (ptr) {
	g_free(ptr);
    }

    return FALSE;
}

#if RELEASE == 1
gpointer idle_free(gpointer ptr)
#else
gpointer __idle_free(gpointer ptr, gchar * f, gint l)
#endif
{
    DEBUG("file: %s, line: %d, ptr %p", f, l, ptr);

    if (ptr) {
	g_timeout_add(10000, __idle_free_do, ptr);
    }

    return ptr;
}

void module_entry_scan_all_except(ModuleEntry * entries, gint except_entry)
{
    ModuleEntry entry;
    gint i = 0;
    void (*scan_callback) (gboolean reload);
    gchar *text;


    for (entry = entries[0]; entry.name; entry = entries[++i]) {
	if (i == except_entry)
	    continue;

	text = g_strdup_printf("Scanning: %s...", entry.name);
	g_free(text);

	if ((scan_callback = entry.scan_callback)) {
	    scan_callback(FALSE);
	}
    }
}

void module_entry_scan_all(ModuleEntry * entries)
{
    module_entry_scan_all_except(entries, -1);
}

void module_entry_reload(ShellModuleEntry * module_entry)
{
    if (module_entry->scan_func) {
	module_entry->scan_func(TRUE);
    }
}

void module_entry_scan(ShellModuleEntry * module_entry)
{
    if (module_entry->scan_func) {
	module_entry->scan_func(FALSE);
    }
}

gchar *module_entry_function(ShellModuleEntry * module_entry)
{
    if (module_entry->func) {
	return g_strdup(module_entry->func());
    }

    return g_strdup("[Error]\n" "Invalid module=");
}

const gchar *module_entry_get_note(ShellModuleEntry * module_entry)
{
    return module_entry->notefunc(module_entry->number);
}

gchar *h_strdup_cprintf(const gchar * format, gchar * source, ...)
{
    gchar *buffer, *retn;
    va_list args;

    va_start(args, source);
    buffer = g_strdup_vprintf(format, args);
    va_end(args);

    if (source) {
	retn = g_strconcat(source, buffer, NULL);
	g_free(buffer);
        g_free(source);
    } else {
	retn = buffer;
    }

    return retn;
}

gchar *h_strconcat(gchar * string1, ...)
{
    gsize l;
    va_list args;
    gchar *s;
    gchar *concat;
    gchar *ptr;

    if (!string1)
	return NULL;

    l = 1 + strlen(string1);
    va_start(args, string1);
    s = va_arg(args, gchar *);
    while (s) {
	l += strlen(s);
	s = va_arg(args, gchar *);
    }
    va_end(args);

    concat = g_new(gchar, l);
    ptr = concat;

    ptr = g_stpcpy(ptr, string1);
    va_start(args, string1);
    s = va_arg(args, gchar *);
    while (s) {
	ptr = g_stpcpy(ptr, s);
	s = va_arg(args, gchar *);
    }
    va_end(args);

    g_free(string1);

    return concat;
}

static gboolean h_hash_table_remove_all_true(gpointer key, gpointer data, gpointer user_data)
{
    return TRUE;
}

void
h_hash_table_remove_all(GHashTable *hash_table)
{
    g_hash_table_foreach_remove(hash_table,
				h_hash_table_remove_all_true,
				NULL);
}

gfloat
h_sysfs_read_float(gchar *endpoint, gchar *entry)
{
	gchar *tmp, *buffer;
	gfloat return_value = 0.0f;
	
	tmp = g_build_filename(endpoint, entry, NULL);
	if (g_file_get_contents(tmp, &buffer, NULL, NULL))
		return_value = atof(buffer);
	
	g_free(tmp);
	g_free(buffer);
	
	return return_value;
}

gint
h_sysfs_read_int(gchar *endpoint, gchar *entry)
{
	gchar *tmp, *buffer;
	gint return_value = 0;
	
	tmp = g_build_filename(endpoint, entry, NULL);
	if (g_file_get_contents(tmp, &buffer, NULL, NULL))
		return_value = atoi(buffer);
	
	g_free(tmp);
	g_free(buffer);
	
	return return_value;
}

gchar *
h_sysfs_read_string(gchar *endpoint, gchar *entry)
{
	gchar *tmp, *return_value;
	
	tmp = g_build_filename(endpoint, entry, NULL);
	if (!g_file_get_contents(tmp, &return_value, NULL, NULL)) {
		g_free(return_value);
		
		return_value = NULL;
	} else {
		return_value = g_strstrip(return_value);
	}
	
	g_free(tmp);
	
	return return_value;
}

