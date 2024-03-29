#ifndef BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_
#define BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_

#include "server_db.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <thread_pool.h>
#include <utils.h>
#include <server.h>
#include <server_ctrl.h>


/*!
 * @brief Start the main thread loop
 *
 * @param p_db Pointer to the database object
 * @param port_num Port number to bind to
 * @param timeout Timeout of each session with the client
 */
void start_server(db_t * p_db, uint32_t port_num, uint8_t timeout);


#ifdef __cplusplus
}
#endif

#endif //BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_
