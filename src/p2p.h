#ifndef P2P_H
#define P2P_H

#include <event2/event.h>

/**
 * @brief initialize listening and set up callbacks
 * @param base event loop
 * @return 1 if an error occured
 * @return 0 if successfully initialized
 */
int listen_init(struct event_base **base);
#endif
