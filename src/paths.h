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

#ifndef PATHS_H
#define PATHS_H

/** Paths to needed files and directories. */
typedef struct s_filepaths {
	/**< Path to config directory. */
	char *config_dir;
	/**< Path to data directory. */
	char *data_dir;

	/**< Path to file with addresses of hosts. */
	char *hosts;
} filepaths_t;

void clear_paths(filepaths_t *filepaths);
int setup_paths(filepaths_t *filepaths);

#endif /* PATHS_H */
