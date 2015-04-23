/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include <report.h>
#include <stdio.h>
#include <string.h>
#include <shell.h>
#include <iconcache.h>
#include <hardinfo.h>
#include <config.h>


static ReportDialog *report_dialog_new(GtkTreeModel * model,
				       GtkWidget * parent);
static void set_all_active(ReportDialog * rd, gboolean setting);

static FileTypes file_types[] = {
    {"HTML (*.html)", "text/html", ".html", report_context_html_new},
    {NULL, NULL, NULL, NULL}
};

void report_header(ReportContext * ctx)
{
    ctx->header(ctx);
}

void report_footer(ReportContext * ctx)
{
    ctx->footer(ctx);
}

void report_title(ReportContext * ctx, gchar * text)
{
    ctx->title(ctx, text);
}

void report_subtitle(ReportContext * ctx, gchar * text)
{
    ctx->subtitle(ctx, text);
}

void report_subsubtitle(ReportContext * ctx, gchar * text)
{
    ctx->subsubtitle(ctx, text);
}

void report_key_value(ReportContext * ctx, gchar * key, gchar * value)
{
    ctx->keyvalue(ctx, key, value);
}

gint report_get_visible_columns(ReportContext *ctx)
{
    gint columns;
    
    /* Column count starts at two, since we always have at least
       two columns visible. */
    columns = 2;

    /* Either the Progress column or the Value column is available at
       the same time. So we don't count them. */

    if (ctx->columns & REPORT_COL_EXTRA1)
      columns++;
    
    if (ctx->columns & REPORT_COL_EXTRA2)
      columns++;
    
    return columns;
}

void report_context_configure(ReportContext * ctx, GKeyFile * keyfile)
{
    gchar **keys;
    const gchar *group = "$ShellParam$";
    
    /* FIXME: sometime in the future we'll save images in the report. this
       flag will be set if we should support that.

       so i don't forget how to encode the images inside the html files:
       http://en.wikipedia.org/wiki/Data:_URI_scheme */

    
    keys = g_key_file_get_keys(keyfile, group, NULL, NULL);
    if (keys) {
      gint i = 0;
      
      for (; keys[i]; i++) {
        gchar *key = keys[i];
        
        if (g_str_equal(key, "ShowColumnHeaders")) {
          ctx->show_column_headers = g_key_file_get_boolean(keyfile, group, key, NULL);
        } else if (g_str_has_prefix(key, "ColumnTitle")) {
          gchar *value, *title = strchr(key, '$') + 1;
          
          value = g_key_file_get_value(keyfile, group, key, NULL);
          if (g_str_equal(title, "Extra1")) {
                  ctx->columns |= REPORT_COL_EXTRA1;
          } else if (g_str_equal(title, "Extra2")) {
                  ctx->columns |= REPORT_COL_EXTRA2;
          } else if (g_str_equal(title, "Value")) {
                  ctx->columns |= REPORT_COL_VALUE;
          } else if (g_str_equal(title, "TextValue")) {
                  ctx->columns |= REPORT_COL_TEXTVALUE;
          } else if (g_str_equal(title, "Progress")) {
                  ctx->columns |= REPORT_COL_PROGRESS;
          }
          
          g_hash_table_replace(ctx->column_titles, title, g_strdup(value));
          
          g_free(value);
        } else if (g_str_equal(key, "ViewType")) {
          if (g_key_file_get_integer(keyfile, group, "ViewType", NULL) == SHELL_VIEW_PROGRESS) {
            ctx->columns &= ~REPORT_COL_VALUE;
            ctx->columns |= REPORT_COL_PROGRESS;
          }     
        }
      }

      g_strfreev(keys);    
    }
    
}

void report_table(ReportContext * ctx, gchar * text)
{
    GKeyFile *key_file = g_key_file_new();
    gchar **groups;
    gint i;

    /* make only "Value" column visible ("Key" column is always visible) */
    ctx->columns = REPORT_COL_VALUE;
    ctx->show_column_headers = FALSE;

    /**/
    g_key_file_load_from_data(key_file, text, strlen(text), 0, NULL);
    groups = g_key_file_get_groups(key_file, NULL);
    
    for (i = 0; groups[i]; i++) {
	if (groups[i][0] == '$') {
	    report_context_configure(ctx, key_file);
	    break;
	}
    }

    for (i = 0; groups[i]; i++) {
	gchar *group, *tmpgroup;
	gchar **keys;
	gint j;

	if (groups[i][0] == '$') {
	    continue;
	}

	group = groups[i];

	tmpgroup = g_strdup(group);
	strend(group, '#');

	report_subsubtitle(ctx, group);

#if 0
	if (ctx->is_image_enabled) {
	    report_embed_image(ctx, key_file, group);
	} else {
#endif
	    keys = g_key_file_get_keys(key_file, tmpgroup, NULL, NULL);
	    for (j = 0; keys[j]; j++) {
		gchar *key = keys[j];
		gchar *value;

		value = g_key_file_get_value(key_file, tmpgroup, key, NULL);

		if (g_utf8_validate(key, -1, NULL) && g_utf8_validate(value, -1, NULL)) {
		    strend(key, '#');

		    if (g_str_equal(value, "...")) {
			g_free(value);
			if (!(value = ctx->entry->fieldfunc(key))) {
			    value = g_strdup("...");
			}
		    }
		    
		    if (*key == '$') {
			report_key_value(ctx, strchr(key + 1, '$') + 1,
					 value);
		    } else {
			report_key_value(ctx, key, value);
		    }

		}

		g_free(value);
	    }

	    g_strfreev(keys);
#if 0
	}
#endif
	g_free(tmpgroup);
    }

    g_strfreev(groups);
    g_key_file_free(key_file);
}

static void report_html_header(ReportContext * ctx)
{
    if (ctx->output)
	g_free(ctx->output);

    ctx->output =
	g_strdup_printf
	("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Final//EN\">\n"
	 "<html><head>\n" "<title>HardInfo (%s) System Report</title>\n"
	 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
	 "<style>\n" "    body    { background: #fff }\n"
	 "    .title  { font: bold 130%% serif; color: #0066FF; padding: 30px 0 10px 0 }\n"
	 "    .stitle { font: bold 100%% sans-serif; color: #0044DD; padding: 30px 0 10px 0 }\n"
	 "    .sstitle{ font: bold 80%% serif; color: #000000; background: #efefef }\n"
	 "    .field  { font: 80%% sans-serif; color: #000000; padding: 2px; padding-left: 50px }\n"
	 "    .value  { font: 80%% sans-serif; color: #505050 }\n"
	 "</style>\n" "</head><body>\n",
	 VERSION);
}

static void report_html_footer(ReportContext * ctx)
{
    ctx->output = h_strconcat(ctx->output,
			      "</table></html>", NULL);
}

static void report_html_title(ReportContext * ctx, gchar * text)
{
    if (!ctx->first_table) {
      ctx->output = h_strdup_cprintf("</table>", ctx->output);
    }

    ctx->output = h_strdup_cprintf("<h1 class=\"title\">%s</h1>", ctx->output, text);
}

static void report_html_subtitle(ReportContext * ctx, gchar * text)
{
    gint columns = report_get_visible_columns(ctx);

    if (!ctx->first_table) {
      ctx->output = h_strdup_cprintf("</table>", ctx->output);
    } else {
      ctx->first_table = FALSE;
    }

    ctx->output = h_strdup_cprintf("<table><tr><td colspan=\"%d\" class=\"stit"
				  "le\">%s</td></tr>\n",
				  ctx->output,
				  columns,
				  text);
}

static void report_html_subsubtitle(ReportContext * ctx, gchar * text)
{
    gint columns = report_get_visible_columns(ctx);

    ctx->output = h_strdup_cprintf("<tr><td colspan=\"%d\" class=\"ssti"
				  "tle\">%s</td></tr>\n",
				  ctx->output,
				  columns,
				  text);
}

static void
report_html_key_value(ReportContext * ctx, gchar * key, gchar * value)
{
    gint columns = report_get_visible_columns(ctx);
    gchar **values;
    gint i;
    
    if (columns == 2) {
      ctx->output = h_strdup_cprintf("<tr><td class=\"field\">%s</td>"
                                    "<td class=\"value\">%s</td></tr>\n",
                                    ctx->output,
                                    key, value);
    } else {
      values = g_strsplit(value, "|", columns);
      
      ctx->output = h_strdup_cprintf("\n<tr>\n<td class=\"field\">%s</td>", ctx->output, key);
      
      for (i = columns - 2; i >= 0; i--) {
        ctx->output = h_strdup_cprintf("<td class=\"value\">%s</td>",
                                       ctx->output,
                                       values[i]);
      }

      ctx->output = h_strdup_cprintf("</tr>\n", ctx->output);
      
      g_strfreev(values);
    }
}

static GSList *report_create_module_list_from_dialog(ReportDialog * rd)
{
    ShellModule *module;
    GSList *modules = NULL;
    GtkTreeModel *model = rd->model;
    GtkTreeIter iter;

    gtk_tree_model_get_iter_first(model, &iter);
    do {
	gboolean selected;
	gchar *name;

	gtk_tree_model_get(model, &iter, TREE_COL_SEL, &selected, -1);
	if (!selected)
	    continue;

	module = g_new0(ShellModule, 1);

	gtk_tree_model_get(model, &iter, TREE_COL_NAME, &name, -1);
	module->name = name;
	module->entries = NULL;

	if (gtk_tree_model_iter_has_child(model, &iter)) {
	    ShellModuleEntry *entry;

	    gint children = gtk_tree_model_iter_n_children(model, &iter);
	    gint i;

	    for (i = 0; i < children; i++) {
		GtkTreeIter child;

		gtk_tree_model_iter_nth_child(model, &child, &iter, i);

		gtk_tree_model_get(model, &child, TREE_COL_SEL, &selected,
				   -1);
		if (!selected)
		    continue;

		gtk_tree_model_get(model, &child, TREE_COL_DATA, &entry,
				   -1);
		module->entries = g_slist_append(module->entries, entry);
	    }
	}

	modules = g_slist_append(modules, module);
    } while (gtk_tree_model_iter_next(rd->model, &iter));

    return modules;
}

static void
report_create_inner_from_module_list(ReportContext * ctx, GSList * modules)
{
    for (; modules; modules = modules->next) {
	ShellModule *module = (ShellModule *) modules->data;
	GSList *entries;


	fprintf(stderr, "\033[40;32m%s\033[0m\n", module->name);

	report_title(ctx, module->name);

	for (entries = module->entries; entries; entries = entries->next) {
	    ShellModuleEntry *entry = (ShellModuleEntry *) entries->data;


		fprintf(stderr, "\033[2K\033[40;32;1m %s\033[0m\n",
			entry->name);

	    ctx->entry = entry;
	    report_subtitle(ctx, entry->name);
	    module_entry_scan(entry);
	    report_table(ctx, module_entry_function(entry));
	}
    }
}

void report_module_list_free(GSList * modules)
{
    GSList *m;

    for (m = modules; m; m = m->next) {
	ShellModule *module = (ShellModule *) m->data;

	g_slist_free(module->entries);
    }

    g_slist_free(modules);
}

static gchar *report_get_filename(void)
{
    GtkWidget *dialog;
    gchar *filename = NULL;

    dialog = gtk_file_chooser_dialog_new("Save File",
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_SAVE,
					 GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
				      "hardinfo_report");

    file_chooser_add_filters(dialog, file_types);
    file_chooser_open_expander(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	gchar *ext = file_chooser_get_extension(dialog, file_types);
	filename = file_chooser_build_filename(dialog, ext);
    }
    gtk_widget_destroy(dialog);
    return filename;
}

ReportContext *report_context_html_new()
{
    ReportContext *ctx;

    ctx = g_new0(ReportContext, 1);
    ctx->header = report_html_header;
    ctx->footer = report_html_footer;
    ctx->title = report_html_title;
    ctx->subtitle = report_html_subtitle;
    ctx->subsubtitle = report_html_subsubtitle;
    ctx->keyvalue = report_html_key_value;

    ctx->output = g_strdup("");
    ctx->format = REPORT_FORMAT_HTML;

    ctx->column_titles = g_hash_table_new(g_str_hash, g_str_equal);
    ctx->first_table = TRUE;

    return ctx;
}

void report_context_free(ReportContext * ctx)
{
    g_hash_table_destroy(ctx->column_titles);
    g_free(ctx->output);
    g_free(ctx);
}

void report_create_from_module_list(ReportContext * ctx, GSList * modules)
{
    report_header(ctx);

    report_create_inner_from_module_list(ctx, modules);
    report_module_list_free(modules);

    report_footer(ctx);
}


gchar *report_create_from_module_list_format(GSList * modules,
					     ReportFormat format)
{
    ReportContext *(*create_context) ();
    ReportContext *ctx;
    gchar *retval;

    if (format >= N_REPORT_FORMAT)
	return NULL;

    create_context = file_types[format].data;
    if (!create_context)
	return NULL;

    ctx = create_context();

    report_create_from_module_list(ctx, modules);
    retval = g_strdup(ctx->output);

    report_context_free(ctx);

    return retval;
}

static gboolean report_generate(ReportDialog * rd)
{
    GSList *modules;
    ReportContext *ctx;
    ReportContext *(*create_context) ();
    gchar *file;
    FILE *stream;

    if (!(file = report_get_filename()))
	return FALSE;

    if (!(stream = fopen(file, "w+"))) {
	g_free(file);
	return FALSE;
    }

    create_context = file_types_get_data_by_name(file_types, file);

    if (!create_context) {
	g_warning("Cannot create ReportContext. Programming bug?");
	g_free(file);
	return FALSE;
    }

    ctx = create_context();
    modules = report_create_module_list_from_dialog(rd);

    report_create_from_module_list(ctx, modules);
    fputs(ctx->output, stream);
    fclose(stream);

    if (ctx->format == REPORT_FORMAT_HTML) {
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_NONE,
					"Open the report with your web browser?");
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			       GTK_STOCK_NO, GTK_RESPONSE_REJECT,
			       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	    open_url(file);

	gtk_widget_destroy(dialog);
    }

    report_context_free(ctx);
    g_free(file);

    return TRUE;
}