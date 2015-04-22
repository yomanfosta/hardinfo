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

#include <stdio.h>
#include "config.h"
#include "shell.h"

#include "report.h"
#include "hardinfo.h"
#include "iconcache.h"
#include "stock.h"
#include "vendor.h"

#include "binreloc.h"


ProgramParameters params = { 0 };

int main(int argc, char **argv)
{
    DEBUG("HardInfo version " VERSION ". Debug version.");
	DEBUG("  Debugging is enabled.");

	GSList *modules;

    /* initialize the binreloc library, so we can load program data */
    if (!binreloc_init(FALSE))
	{

		//still need to figure out where the fuck g_error is and why I'm not recognizing
		g_error("Failed to find runtime data.\n\n"
						"\342\200\242 Is HardInfo correctly installed?\n"
						"\342\200\242 See if %s and %s exists and you have read permision.",
				PREFIX, LIBPREFIX);
	}

	/* load all modules */
	DEBUG("loading all modules");
	modules = modules_load_all();

    /* initialize vendor database */
    vendor_init();

	//THIS IS THE ELSE IF BLOCK I WANT
	/* generate report */
	gchar *report;
	DEBUG("generating report");
	report = report_create_from_module_list_format(modules,
						       REPORT_FORMAT_HTML); //TODO this looks like the ultimate function I want to look into
	g_print("%s", report);
	g_free(report);
	//END BLOCK

    DEBUG("finished");
    return 0;
}
