/*
 *  Coincer
 *  Copyright (C) 2017-2018  Coincer Developers
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "autoconfig.h"
#include "log.h"
#include "paths.h"

#define TRADES_DIR_NAME		"trades"
#define TRADES_BASIC_DIR_NAME	"basic"

#define HOSTS_FILE_NAME "hosts"

static int set_dir_path(const char *location,
			const char *dir_name,
			char	   **dir_path);

/**
 * Set path to home directory.
 *
 * @param	homedir		Path to home directory.
 *
 * @return	0		Path successfully set.
 * @return	1		Home directory retrieval failure.
 */
static int set_homedir(char **homedir)
{
	*homedir = getenv("HOME");
	if (!*homedir || *homedir[0] == '\0') {
		log_error("Can not find home directory");
		return 1;
	}
	return 0;
}

/**
 * Set path to config directory.
 *
 * @param	config_dir	Path to config directory.
 *
 * @return	0		Successfully set.
 * @return	1		Failure.
 */
static int set_config_dir_path(char **config_dir)
{
	char *homedir = NULL,
	     *tmpchar = NULL;

	tmpchar = getenv("XDG_CONFIG_HOME");
	if (!tmpchar || tmpchar[0] == '\0') {
		if (set_homedir(&homedir)) {
			return 1;
		}
		return set_dir_path(homedir,
				    "/.config/" PACKAGE,
				    config_dir);
	}

	return set_dir_path(tmpchar, "/" PACKAGE, config_dir);
}

/**
 * Set path to data directory.
 *
 * @param	data_dir	Path to data directory.
 *
 * @return	0		Successfully set.
 * @return	1		Failure.
 */
static int set_data_dir_path(char **data_dir)
{
	char *homedir = NULL,
	     *tmpchar = NULL;

	tmpchar = getenv("XDG_DATA_HOME");
	if (!tmpchar || tmpchar[0] == '\0') {
		if (set_homedir(&homedir)) {
			return 1;
		}
		return set_dir_path(homedir,
				    "/.local/share/" PACKAGE,
				    data_dir);
	}

	return set_dir_path(tmpchar, "/" PACKAGE, data_dir);
}

/**
 * Sets path to hosts file.
 *
 * @param	data_dir	Path to data dir.
 * @param	hosts		The file path of hosts.
 *
 * @return	0		Path successfully set.
 * @return	1		Allocation failure.
 */
static int set_hosts_path(char *data_dir, char **hosts)
{
	/* size of data_dir + HOSTS_FILE_NAME */
	*hosts = (char *) malloc(strlen(data_dir) +
				 sizeof(HOSTS_FILE_NAME));
	if (!*hosts) {
		log_error("Setting hosts path");
		return 1;
	}

	strcpy(*hosts, data_dir);
	strcat(*hosts, HOSTS_FILE_NAME);

	return 0;
}

/**
 * Set path to a directory.
 *
 * @param	location	Directory's location.
 * @param	dir_name	Directory's name.
 * @param	dir_path	Path to the directory.
 *
 * @return	0		Successfully set.
 * @return	1		Failure.
 */
static int set_dir_path(const char *location,
			const char *dir_name,
			char	   **dir_path)
{
	*dir_path = (char *) malloc((strlen(location) + strlen(dir_name)) *
				    sizeof(char) + sizeof("/"));

	if (!*dir_path) {
		log_error("Setting directory path for %s", dir_name);
		return 1;
	}

	strcpy(*dir_path, location);

	strcat(*dir_path, dir_name);
	strcat(*dir_path, "/");

	return 0;
}

/**
 * Initializes a filepaths_t instance.
 *
 * @param	filepaths	filepaths_t to be initialized.
 *
 * @return	0		Successfully initialized.
 * @return	1		Setup failure.
 */
int setup_paths(filepaths_t *filepaths)
{
	if (set_config_dir_path(&filepaths->config_dir) ||
	    set_data_dir_path(&filepaths->data_dir) ||
	    set_dir_path(filepaths->data_dir,
			 TRADES_DIR_NAME,
			 &filepaths->trades_dir) ||
	    set_dir_path(filepaths->trades_dir,
			 TRADES_BASIC_DIR_NAME,
			 &filepaths->trades_basic_dir)) {
		log_error("Setting directory paths failed");
		return 1;
	}

	if (set_hosts_path(filepaths->data_dir, &filepaths->hosts)) {
		return 1;
	}

	return 0;
}

/**
 * Clear allocated path strings.
 *
 * @param	filepaths	filepaths_t to be cleared.
 */
void clear_paths(filepaths_t *filepaths)
{
	free(filepaths->config_dir);
	free(filepaths->data_dir);
	free(filepaths->trades_dir);
	free(filepaths->trades_basic_dir);

	free(filepaths->hosts);
}
