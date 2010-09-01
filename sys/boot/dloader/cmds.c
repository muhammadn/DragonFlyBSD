/*
 * Copyright (c) 2010 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stand.h>
#include <string.h>
#include "bootstrap.h"
#include "dloader.h"

static void menu_display(void);
static int menu_execute(int);

/*
 * This is called from common and must reference files to bring
 * library modules into common during linking.
 */
void
dloader_init_cmds(void)
{
}

/*
 * This intercepts lines of the form 'a=b'
 */
COMMAND_SET(local, "local", "List local variables", command_local);
COMMAND_SET(lunset, "lunset", "Unset local variables", command_lunset);
COMMAND_SET(lunsetif, "lunsetif", "Unset local if envvar set to 1 or YES", command_lunsetif);
COMMAND_SET(loadall, "loadall", "Load kernel + modules", command_loadall);
COMMAND_SET(menuclear, "menuclear", "Clear all menus", command_menuclear);
COMMAND_SET(menuitem, "menuitem", "Add menu bullet", command_menuitem);
COMMAND_SET(menuadd, "menuadd", "Add script line for bullet", command_menuadd);
COMMAND_SET(menu, "menu", "Run menu system", command_menu);

static int curitem;
static int curadd;

/*
 * Set local variable.  Sniff "module_path"
 *
 * format a=b (one argument) - in av[0]
 */
static int
command_local(int ac, char **av)
{
	char *name;
	char *data;
	dvar_t dvar;
	int i;
	int j;

	/*
	 * local command executed directly.
	 */
	if (strcmp(av[0], "local") == 0) {
		for (dvar = dvar_first(); dvar; dvar = dvar_next(dvar)) {
			for (j = 1; j < ac; ++j) {
				if (!strncmp(dvar->name, av[j], strlen(av[j])))
					break;
			}
			if (ac > 1 && j == ac)
				continue;

			printf("%s=", dvar->name);
			for (i = 0; i < dvar->count; ++i) {
				if (i)
					printf(",");
				printf("\"%s\"", dvar->data[i]);
			}
			printf("\n");
		}
		return(CMD_OK);
	}

	/*
	 * local command intercept for blah=blah
	 */
	name = av[0];
	data = strchr(name, '=');
	if (data == NULL) {
		sprintf(command_errbuf, "Bad variable syntax");
		return (CMD_ERROR);
	}
	*data++ = 0;

	if (*data)
		dvar_set(name, &data, 1);
	else
		dvar_unset(name);

	/*
	 * These variable have to mirror to kenv because libstand or
	 * other consumers may have hooks into them.
	 */
	if (strchr(name, '.') ||
	    strcmp(name, "kernelname") == 0 ||
	    strcmp(name, "module_path") == 0 ||
	    strcmp(name, "console") == 0 ||
	    strcmp(name, "currdev") == 0 ||
	    strcmp(name, "loaddev") == 0 ||
	    strcmp(name, "bootfile") == 0) {
		if (*data)
			setenv(name, data, 1);
		else
			unsetenv(name);
	}
	return(CMD_OK);
}

/*
 * Unset local variables
 */
static int
command_lunset(int ac, char **av)
{
	int i;

	for (i = 1; i < ac; ++i)
		dvar_unset(av[i]);
	return(0);
}

static int
command_lunsetif(int ac, char **av)
{
	char *envdata;

	if (ac != 3) {
		sprintf(command_errbuf,
			"syntax error use lunsetif lname envname");
		return(CMD_ERROR);
	}
	envdata = getenv(av[2]);
	if (strcmp(envdata, "yes") == 0 ||
	    strcmp(envdata, "YES") == 0 ||
	    strtol(envdata, NULL, 0)) {
		dvar_unset(av[1]);
	}
	return (CMD_OK);
}

/*
 * Load the kernel + all modules specified with
 */
static int
command_loadall(int ac, char **av)
{
	char *argv[4];
	dvar_t dvar;
	int len;
	int argc;
	int res;
	int tmp;

	argv[0] = "unload";
	(void)perform(1, argv);

	/*
	 * Load kernel
	 */
	argv[0] = "load";
	argv[1] = getenv("kernelname");
	if (argv[1] == NULL)
		argv[1] = strdup("kernel");
	res = perform(2, argv);
	free(argv[1]);

	if (res != CMD_OK) {
		printf("Unable to load %s%s\n", DirBase, argv[1]);
		return(res);
	}

	/*
	 * Load modules
	 */
	for (dvar = dvar_first(); dvar; dvar = dvar_next(dvar)) {
		len = strlen(dvar->name);
		if (len <= 5 || strcmp(dvar->name + len - 5, "_load"))
			continue;
		if (strcmp(dvar->data[0], "yes") != 0 &&
		    strcmp(dvar->data[0], "YES") != 0) {
			continue;
		}
		argv[0] = "load";
		argv[1] = strdup(dvar->name);
		argv[1][len - 5] = 0;
		tmp = perform(2, argv);
		free(argv[1]);
		if (tmp != CMD_OK) {
			time_t t = time(NULL);
			printf("Unable to load %s%s\n", DirBase, argv[1]);
			while (time(NULL) == t)
				;
			/* don't kill the boot sequence */
			/* res = tmp; */
		}

	}
	return(res);
}

/*
 * Clear all menus
 */
static int
command_menuclear(int ac, char **av)
{
	dvar_unset("menu_*");
	dvar_unset("item_*");
	curitem = 0;
	curadd = 0;
	return(0);
}

/*
 * Add menu bullet
 */
static int
command_menuitem(int ac, char **av)
{
	char namebuf[32];
	char *cp;

	if (ac != 3) {
		sprintf(command_errbuf, "Bad menuitem syntax");
		return (CMD_ERROR);
	}
	curitem = (unsigned char)av[1][0];
	if (curitem == 0) {
		sprintf(command_errbuf, "Bad menuitem syntax");
		return (CMD_ERROR);
	}
	snprintf(namebuf, sizeof(namebuf), "menu_%c", curitem);
	dvar_set(namebuf, &av[2], 1);
	curadd = 0;

	return(CMD_OK);
}

/*
 * Add execution item
 */
static int
command_menuadd(int ac, char **av)
{
	char namebuf[32];
	int i;

	if (ac == 1)
		return(CMD_OK);
	if (curitem == 0) {
		sprintf(command_errbuf, "Missing menuitem for menuadd");
		return(CMD_ERROR);
	}
	snprintf(namebuf, sizeof(namebuf), "item_%c_%d", curitem, curadd);
	dvar_set(namebuf, &av[1], ac - 1);
	++curadd;
	return (CMD_OK);
}

/*
 * Execute menu system
 */
static int
command_menu(int ac, char **av)
{
	int timeout = -1;
	time_t time_target;
	time_t time_last;
	time_t t;
	char *cp;
	int c;
	int res;
	int counting = 1;

	menu_display();
	if ((cp = getenv("autoboot_delay")) != NULL)
		timeout = strtol(cp, NULL, 0);
	if (timeout <= 0)
		timeout = 10;
	if (timeout > 24 * 60 * 60)
		timeout = 24 * 60 * 60;

	time_target = time(NULL) + timeout;
	time_last = 0;
	c = '1';
	for (;;) {
		if (ischar()) {
			c = getchar();
			if (c == '\r' || c == '\n') {
				c = '1';
				break;
			}
			if (c == ' ') {
				if (counting) {
					printf("\rCountdown halted by "
					       "space   ");
				}
				counting = 0;
				continue;
			}
			if (c == 0x1b) {
				setenv("autoboot_delay", "NO", 1);
				return(CMD_OK);
			}
			res = menu_execute(c);
			if (res >= 0) {
				setenv("autoboot_delay", "NO", 1);
				return(CMD_OK);
			}
			/* else ignore char */
		}
		if (counting) {
			t = time(NULL);
			if (time_last == t)
				continue;
			time_last = t;
			printf("\rBooting in %d second%s... ",
				(int)(time_target - t),
				((time_target - t) == 1 ? "" : "s"));
			if ((int)(time_target - t) <= 0) {
				c = '1';
				break;
			}
		}
	}
	res = menu_execute(c);
	if (res != CMD_OK)
		setenv("autoboot_delay", "NO", 1);
	return (res);
}

static void
menu_display(void)
{
	dvar_t dvar;

	for (dvar = dvar_first(); dvar; dvar = dvar_next(dvar)) {
		if (strncmp(dvar->name, "menu_", 5) == 0) {
			printf("%c. %s\n", dvar->name[5], dvar->data[0]);
		}
	}
	printf("\n");
}

static int
menu_execute(int c)
{
	dvar_t dvar;
	char namebuf[32];
	int res;

	printf("\n");

	snprintf(namebuf, sizeof(namebuf), "item_%c_0", c);

	/*
	 * Does this menu option exist?
	 */
	if (dvar_get(namebuf) == NULL)
		return(-1);

	snprintf(namebuf, sizeof(namebuf), "item_%c", c);
	res = CMD_OK;
	for (dvar = dvar_first(); dvar; dvar = dvar_next(dvar)) {
		if (strncmp(dvar->name, namebuf, 6) == 0) {
			res = perform(dvar->count, dvar->data);
			if (res != CMD_OK) {
				printf("%s: %s\n",
					dvar->data[0], command_errmsg);
				setenv("autoboot_delay", "NO", 1);
				break;
			}
		}
	}
	return(res);
}
