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
#include <sys/types.h>

#include "configuration.h"
#include "log.h"
#include "paths.h"

/**
 * Create a directory, if doesn't exist yet.
 *
 * @param	dir_path	Path to the directory.
 * @param	mode		Directory's permissions.
 *
 * @return	0		Success.
 * @return	1		Failure.
 */
static int create_dir(const char *dir_path, mode_t mode)
{
	struct stat buffer;

	if (stat(dir_path, &buffer)) {
		if (errno == ENOENT) {
			/* create */
			if (mkdir(dir_path, mode)) {
				log_error("Could not create directory %s",
					  dir_path);
				return 1;
			} else {
				log_debug("create_dir - directory %s created",
					  dir_path);
			}
		} else {
			log_error("Could not open directory %s", dir_path);
			return 1;
		}
	}

	return 0;
}

/**
 * Create needed directories.
 *
 * @param	paths	Create directories for dir paths in here.
 *
 * @return	0	Directories created.
 * @return	1	Failure.
 */
int create_dirs(const filepaths_t *paths)
{
	if (create_dir(paths->config_dir, S_IRWXU) ||
	    create_dir(paths->data_dir, S_IRWXU)) {
		log_error("Creating directories");
		return 1;
	}

	return 0;
}
