/*
 *  Coincer
 *  Copyright (C) 2017  Coincer Developers
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

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* chmod */

#include "configuration.h"
#include "filing.h"
#include "log.h"

static const char *PEERS_FILE_NAME = "peers";

/**
 * Sets path to peers file.
 *
 * @param	data_dir	Path to data dir.
 * @param	peers		The file path of peers.
 *
 * @return	0		Path successfully set.
 * @return	1		Allocation failure.
 */
static int set_peers_path(char *data_dir, char **peers)
{
	FILE *peers_file;

	/* size of data_dir + PEERS_FILE_NAME + '\0' */
	*peers = (char *) malloc(strlen(data_dir) +
				 strlen(PEERS_FILE_NAME) + 1);
	if (*peers == NULL) {
		log_error("set_peers_path - peers file malloc");
		return 1;
	}

	strcpy(*peers, data_dir);
	strcat(*peers, PEERS_FILE_NAME);

	/* create file if doesn't exist */
	peers_file = fopen(*peers, "a");
	fclose(peers_file);

	/* change permissions */
	chmod(*peers, strtol("0600", 0, 8));

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

	set_peers_path(filepaths->data_dir, &filepaths->peers);

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

	free(filepaths->peers);
}

