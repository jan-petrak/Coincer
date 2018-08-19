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

#include "configuration.h"
#include "log.h"
#include "paths.h"

#define HOSTS_FILE_NAME "hosts"

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
	if (*hosts == NULL) {
		log_error("Setting hosts path");
		return 1;
	}

	strcpy(*hosts, data_dir);
	strcat(*hosts, HOSTS_FILE_NAME);

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
	if (setup_directories(&filepaths->config_dir,
			      &filepaths->data_dir)) {
		return 1;
	}

	set_hosts_path(filepaths->data_dir, &filepaths->hosts);

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

	free(filepaths->hosts);
}
