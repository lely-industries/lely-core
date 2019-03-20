/**@file
 * This header file is part of the I/O library; it contains the CAN bus
 * declarations for Linux.
 *
 * This implementation uses the SocketCAN interface.
 *
 * @copyright 2018-2019 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LELY_IO2_LINUX_CAN_H_
#define LELY_IO2_LINUX_CAN_H_

#include <lely/io2/can.h>
#include <lely/io2/sys/io.h>

#ifdef __cplusplus
extern "C" {
#endif

void *io_can_ctrl_alloc(void);
void io_can_ctrl_free(void *ptr);
io_can_ctrl_t *io_can_ctrl_init(
		io_can_ctrl_t *ctrl, unsigned int index, size_t txlen);
void io_can_ctrl_fini(io_can_ctrl_t *ctrl);

/**
 * Creates a new CAN controller from an interface name.
 *
 * @param name  the name of a SocketCAN network interface.
 * @param txlen the transmission queue length (in number of frames) of the
 *              network interface. If <b>txlen</b> is 0, the default value
 *              #LELY_IO_CAN_TXLEN is used.
 *
 * @returns a pointer to a new CAN controller, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_can_ctrl_create_from_index()
 */
io_can_ctrl_t *io_can_ctrl_create_from_name(const char *name, size_t txlen);

/**
 * Creates a new CAN controller from an interface index.
 *
 * @param index the index of a SocketCAN network interface.
 * @param txlen the transmission queue length (in number of frames) of the
 *              network interface. If <b>txlen</b> is 0, the default value
 *              #LELY_IO_CAN_TXLEN is used.
 *
 * @returns a pointer to a new CAN controller, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_can_ctrl_create_from_name()
 */
io_can_ctrl_t *io_can_ctrl_create_from_index(unsigned int index, size_t txlen);

/**
 * Destroys a CAN controller.
 *
 * @see io_can_ctrl_create_from_name(), io_can_ctrl_create_from_index()
 */
void io_can_ctrl_destroy(io_can_ctrl_t *ctrl);

/// Returns the interface name of a CAN controller.
const char *io_can_ctrl_get_name(const io_can_ctrl_t *ctrl);

/// Returns the interface index of a CAN controller.
unsigned int io_can_ctrl_get_index(const io_can_ctrl_t *ctrl);

/**
 * Returns the flags specifying which CAN bus features are enabled.
 *
 * @returns any combination of IO_CAN_BUS_FLAG_ERR, #IO_CAN_BUS_FLAG_FDF and
 * #IO_CAN_BUS_FLAG_BRS.
 */
int io_can_ctrl_get_flags(const io_can_ctrl_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_LINUX_CAN_H_
