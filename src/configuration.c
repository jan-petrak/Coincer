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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "autoconfig.h"
#include "configuration.h"
#include "log.h"

/**
 * Set home directory.
 *
 * @param	homedir		Path to homedir.
 *
 * @return	0		Path successfully set.
 * @return	1		Home directory retrieval failure.
 */
static int set_homedir(char **homedir)
{
	*homedir = getenv("HOME");
	if (*homedir == NULL || *homedir[0] == '\0') {
		log_error("Can not find home directory");
		return 1;
	}
	return 0;
}

/**
 * Set paths to config and data directories.
 *
 * @param	config_dir	Config directory.
 * @param	data_dir	Data directory.
 *
 * @return	0		Paths successfully set.
 * @return	1		Allocation failure.
 */
static int set_directories(char **config_dir, char **data_dir)
{
	char *homedir = NULL,
	     *tmpchar = NULL;
	size_t tmpsize;

	/* configuration directory */
	tmpchar = getenv("XDG_CONFIG_HOME");
	if (tmpchar == NULL || tmpchar[0] == '\0') {
		if (set_homedir(&homedir)) {
			return 1;
		}
		tmpsize = strlen(homedir);
		*config_dir = (char *) malloc(tmpsize +
					     sizeof("/.config/" PACKAGE_NAME
						    "/"));
		if (*config_dir == NULL) {
			log_error("Setting configdir");
			return 1;
		}
		strcpy(*config_dir, homedir);
		strcpy(*config_dir + tmpsize, "/.config/" PACKAGE_NAME "/");
	} else {
		tmpsize = strlen(tmpchar);
		*config_dir = (char *) malloc(tmpsize +
					     sizeof("/" PACKAGE_NAME "/"));
		if (*config_dir == NULL) {
			log_error("Setting configdir");
			return 1;
		}
		strcpy(*config_dir, tmpchar);
		strcpy(*config_dir + tmpsize, "/" PACKAGE_NAME "/");
	}

	/* data directory */
	tmpchar = getenv("XDG_DATA_HOME");
	if (tmpchar == NULL || tmpchar[0] == '\0') {
		if (homedir == NULL && set_homedir(&homedir)) {
			free(config_dir);
			return 1;
		}
		tmpsize = strlen(homedir);
		*data_dir = (char *) malloc(tmpsize +
					   sizeof("/.local/share/" PACKAGE_NAME
						  "/"));
		if (*data_dir == NULL) {
			log_error("Setting datadir");
			free(config_dir);
			return 1;
		}
		strcpy(*data_dir, homedir);
		strcpy(*data_dir + tmpsize, "/.local/share/" PACKAGE_NAME "/");
	} else {
		tmpsize = strlen(tmpchar);
		*data_dir = (char *) malloc(tmpsize +
					   sizeof("/" PACKAGE_NAME "/"));
		if (*data_dir == NULL) {
			log_error("Setting datadir");
			free(config_dir);
			return 1;
		}
		strcpy(*data_dir, tmpchar);
		strcpy(*data_dir + tmpsize, "/" PACKAGE_NAME "/");
	}

	return 0;
}

/**
 * Creates directories if they don't exist yet.
 *
 * @param	config_dir	Config directory.
 * @param	data_dir	Data directory.
 *
 * @return	0		Directories created.
 * @return	1		Creating directories failed.
 */
static int create_dirs(const char *config_dir, const char *data_dir)
{
	struct stat buffer;

	/* configuration directory */
	if (stat(config_dir, &buffer)) {
		if (errno == ENOENT) {
			/* create */
			if (mkdir(config_dir, S_IRWXU)) {
				log_error("Could not create configuration "
					  "directory %s", config_dir);
				return 1;
			} else {
				log_debug("create_dirs - created configuration "
					  "directory %s", config_dir);
			}
		} else {
			log_error("Could not open configuration "
				  "directory %s", config_dir);
			return 1;
		}
	}

	/* data directory */
	if (stat(data_dir, &buffer)) {
		if (errno == ENOENT) {
			/* create */
			if (mkdir(data_dir, S_IRWXU)) {
				log_error("Could not create "
					  "data directory %s", data_dir);
				return 1;
			} else {
				log_debug("create_dirs - created data "
					  "directory %s", data_dir);
			}
		} else {
			log_error("Could not open data directory %s", data_dir);
			return 1;
		}
	}

	return 0;
}

/**
 * Create needed directories and fetch their paths into params.
 *
 * @param	config_dir	Config directory.
 * @param	data_dir	Data directory.
 *
 * @return	0		Directories created.
 * @return	1		Directories setup failure.
 */
int setup_directories(char **config_dir, char **data_dir)
{
	if (set_directories(config_dir, data_dir) ||
	    create_dirs(*config_dir, *data_dir)) {
		return 1;
	}

	return 0;
}
