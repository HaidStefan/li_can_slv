/****************************************************************************/
/*                                                                          */
/*                     Copyright (c) 2018, Liebherr PME1                    */
/*                         ALL RIGHTS RESERVED                              */
/*                                                                          */
/* This file is part of li_can_slv stack which is free software: you can    */
/* redistribute it and/or modify it under the terms of the GNU General      */
/* Public License as published by the Free Software Foundation, either      */
/* version 3 of the License, or (at your option) any later version.         */
/*                                                                          */
/* The li_can_slv stack is distributed in the hope that it will be useful,  */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General */
/* Public License for more details.                                         */
/*                                                                          */
/* You should have received a copy of the GNU General Public License which  */
/* should be located in the the root of the Stack. If not, contact Liebherr */
/* to obtain a copy.                                                        */
/****************************************************************************/

/**
 * @file io_can.c
 * @brief initialization of can module
 * @addtogroup lcs_core
 * @{
 */
/*--------------------------------------------------------------------------*/
/* include files                                                            */
/*--------------------------------------------------------------------------*/
#include "io_can.h"
#include "io_can_hw.h"
#include "io_can_types.h"

#include "io_can_errno.h"

#ifdef CAN_SMP
#include "io_smp.h"
#endif // #ifdef CAN_SMP

#ifdef LI_CAN_SLV_RECONNECT
#include "io_can_reconnect.h"
#endif // #ifdef LI_CAN_SLV_RECONNECT

#include "io_can_main.h"
#include "io_can_main_hw.h"
#include "io_can_main_hw_handler.h"

#if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
#include "io_can_mon.h"
#include "io_can_mon_hw.h"
#endif // #if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)

#include "io_can_sys.h"

#ifndef LI_CAN_SLV_BOOT
#include "io_can_sync.h"
#endif // #ifndef LI_CAN_SLV_BOOT

#ifdef LI_CAN_SLV_ASYNC
#include "io_can_async.h"
#endif // #ifdef LI_CAN_SLV_ASYNC

#include "io_can_port.h"

#ifdef LI_CAN_SLV_DEBUG
#include "li_can_slv_debug.h"
#endif // #ifdef LI_CAN_SLV_DEBUG

/*--------------------------------------------------------------------------*/
/* general definitions (private/not exported)                               */
/*--------------------------------------------------------------------------*/
#ifdef CAN_RANDOM_STATUS_ACKNOWLEDGE
/* randomize the status acknowledge response time */
/* delay time needs only to be in arbitration area; (13 Bit: 1 Start Bit, 11 Identifier Bits, 1 RTR Bit) */
/* => lowest baudrate 125k => 104us */
/* => the random range should be 0us...104us */
#define CAN_MAX_STATUS_ACKN_DELAY_NS		104000L /*!< maximum time delay of status acknowledge in ns */
#ifdef XE167
#define CAN_NOP_NS							12.5 /*!< time of one NOP in ns */
#else // #ifdef XE167
#define CAN_NOP_NS							25L /*!< time of one NOP in ns */
#endif // #ifdef XE167
#define CAN_MAX_STATUS_ACKN_DELAY_NOPS		(uint16_t)(CAN_MAX_STATUS_ACKN_DELAY_NS/CAN_NOP_NS)	/*!<  maximum number NOP for time delays of status acknowledge */
#endif // #ifdef CAN_RANDOM_STATUS_ACKNOWLEDGE

static lcsa_state_t li_can_slv_state = LI_CAN_SLV_STATE_INIT;
static li_can_slv_mode_t li_can_slv_mode = LI_CAN_SLV_MODE_INIT;

/*--------------------------------------------------------------------------*/
/* structure/type definitions (private/not exported)                        */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* global variables (public/exported)                                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* global variables (private/not exported)                                  */
/*--------------------------------------------------------------------------*/
can_mainmon_type_t can_mainmon_type = CAN_MAINMON_TYPE_UNDEF;

/*--------------------------------------------------------------------------*/
/* function prototypes (private/not exported)                               */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* function definition (public/exported)                                    */
/*--------------------------------------------------------------------------*/
/**
 * @param baudrate baud rate used for initialization
 * @return #li_can_slv_errorcode_t or #LI_CAN_SLV_ERR_OK if successful
 */
li_can_slv_errorcode_t li_can_slv_init(can_config_bdr_t baudrate)
{
	li_can_slv_errorcode_t err = LI_CAN_SLV_ERR_OK;
#if defined(OUTER) || defined(OUTER_APP)
	uint16_t msg_obj;
#endif // #if defined(OUTER) || defined(OUTER_APP)
	uint16_t i;

#if defined(OUTER) || defined(OUTER_APP)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_config_init();
	}
#endif // #if defined(OUTER) || defined(OUTER_APP)

	// save the baud rate on startup
	can_config_set_baudrate_startup(baudrate);

	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_config_set_baudrate_table();
	}

	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_hw_init();
	}

#ifdef LI_CAN_SLV_MAIN_MON
	if (err == LI_CAN_SLV_ERR_OK)
	{
		can_mainmon_type = can_hw_get_mainmon_type();
#ifdef LI_CAN_SLV_DEBUG_CAN_MAIN_MON
		if (can_mainmon_type == CAN_MAINMON_TYPE_MAIN)
		{
			LI_CAN_SLV_DEBUG_PRINT("\ncan_mainmon => main");
			//main_mon_uart_log("\r\n hello from main_mon: main");
		}

		if (can_mainmon_type == CAN_MAINMON_TYPE_MON)
		{
			LI_CAN_SLV_DEBUG_PRINT("\ncan_mainmon => mon");
			//main_mon_uart_log("\r\n hello from main_mon: mon");
		}
#endif // #ifdef LI_CAN_SLV_DEBUG_CAN_MAIN_MON
	}
#endif // #ifdef LI_CAN_SLV_MAIN_MON

	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_main_init();
	}

#ifdef LI_CAN_SLV_MAIN_MON
	if (can_mainmon_type == CAN_MAINMON_TYPE_MAIN)
	{
#endif // #ifdef LI_CAN_SLV_MAIN_MON
#if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
		if (err == LI_CAN_SLV_ERR_OK)
		{
			err = can_mon_init();
		}
#endif // #if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
#ifdef LI_CAN_SLV_MAIN_MON
	}
#endif // #ifdef LI_CAN_SLV_MAIN_MON

	// initialization of the message object memory
	for (i = 0; i < LI_CAN_SLV_MAIN_NODE_MAX_NOF_MSG_OBJ; i++)
	{
		if (err == LI_CAN_SLV_ERR_OK)
		{
			err = can_main_hw_msg_obj_init(i);
		}
	}

#if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
	for (i = 0; i < LI_CAN_SLV_MON_NODE_MAX_NOF_MSG_OBJ; i++)
	{
		if (err == LI_CAN_SLV_ERR_OK)
		{
			err = can_mon_hw_msg_obj_init(i);
		}
	}
#endif // #if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)

#if defined(OUTER) || defined(OUTER_APP)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define message objects for process request on main CAN-controller
		err = can_main_define_msg_obj(CAN_CONFIG_MSG_MAIN_OBJ_RX_PROCESS, CAN_CONFIG_PROCESS_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_PROCESS_DLC, CAN_CONFIG_DIR_RX, CAN_MAIN_SERVICE_ID_RX, CAN_OBJECT_NOT_SYNC);
	}
#endif // #if defined(OUTER) || defined(OUTER_APP)

#if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY_PROCESS_REQ)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define message objects for process request on monitor CAN-controller
		err = can_mon_define_msg_obj(CAN_CONFIG_MSG_MON_OBJ_RX_PROCESS, CAN_CONFIG_PROCESS_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_PROCESS_DLC, CAN_CONFIG_DIR_RX, CAN_MON_ISR_ID_RX, CAN_OBJECT_NOT_SYNC);
	}
#endif // #if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY_PROCESS_REQ)

#if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY_SYSTEM_REQ)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define message objects for system request on monitor CAN-controller
		err = can_mon_define_msg_obj(CAN_CONFIG_MSG_MON_OBJ_RX_SYS1, CAN_CONFIG_SYS_MSG_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_SYS_MSG_DLC, CAN_CONFIG_DIR_RX, CAN_MAIN_SERVICE_ID_RX, CAN_OBJECT_NOT_SYNC);
	}
#endif // #if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY_SYSTEM_REQ)

	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define two message object(s) for receiving system messages
		err = can_main_define_msg_obj(CAN_CONFIG_MSG_MAIN_OBJ_RX_SYS1, CAN_CONFIG_SYS_MSG_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_SYS_MSG_DLC, CAN_CONFIG_DIR_RX, CAN_MAIN_SERVICE_ID_RX, CAN_OBJECT_NOT_SYNC);
#if defined(OUTER) || defined(OUTER_APP)
#if 1
		if (err == LI_CAN_SLV_ERR_OK)
		{
			err = can_main_define_msg_obj(CAN_CONFIG_MSG_MAIN_OBJ_RX_SYS2, CAN_CONFIG_SYS_MSG_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_SYS_MSG_DLC, CAN_CONFIG_DIR_RX, CAN_MAIN_SERVICE_ID_RX, CAN_OBJECT_NOT_SYNC);
		}
		if (err == LI_CAN_SLV_ERR_OK)
		{
			err = can_hw_combine_msg_obj_to_two_stage_fifo(CAN_CONFIG_MSG_MAIN_OBJ_RX_SYS1, CAN_CONFIG_MSG_MAIN_OBJ_RX_SYS2);
		}
#endif
#endif // #if defined(OUTER) || defined(OUTER_APP)
	}

	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define message object for transmitting system messages
		err = can_main_define_msg_obj(CAN_CONFIG_MSG_MAIN_OBJ_TX_SYS, CAN_CONFIG_ONLINE_ARB_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_ONLINE_DLC, CAN_CONFIG_DIR_TX, CAN_MAIN_SERVICE_ID_TX, CAN_OBJECT_NOT_SYNC);
	}

#ifdef LI_CAN_SLV_ASYNC
	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define message object for transmitting asynchronous control and asynchronous data
		err = can_main_define_msg_obj(CAN_CONFIG_MSG_MAIN_OBJ_ASYNC_TX, CAN_CONFIG_ONLINE_ARB_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_ONLINE_DLC, CAN_CONFIG_DIR_TX, CAN_MAIN_SERVICE_ID_TX, CAN_OBJECT_NOT_SYNC);
	}

	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define message object for receiving asynchronous control data
#ifdef LI_CAN_SLV_BOOT
		err = can_main_define_msg_obj(CAN_CONFIG_MSG_MAIN_OBJ_ASYNC_CTRL1_RX, CAN_CONFIG_ASYNC_CTRL_RX_SLAVE_ID,  CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_ASYNC_CTRL_RX_DLC, CAN_CONFIG_DIR_RX, CAN_MAIN_SERVICE_ID_RX, CAN_OBJECT_NOT_SYNC);
#endif // #ifdef LI_CAN_SLV_BOOT
	}

#if defined(OUTER) || defined(OUTER_APP)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		// define message object for receiving asynchronous data
		err = can_main_define_msg_obj(CAN_CONFIG_MSG_MAIN_OBJ_ASYNC_DATA_RX, CAN_CONFIG_ASYNC_CTRL_RX_SLAVE_ID,  CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_ASYNC_DATA_RX_DLC, CAN_CONFIG_DIR_RX, CAN_MAIN_ASYNC_SERVICE_ID_RX, CAN_OBJECT_NOT_SYNC);
	}
#endif // #if defined(OUTER) || defined(OUTER_APP)
#endif // #ifdef LI_CAN_SLV_ASYNC

#if defined(OUTER) || defined(OUTER_APP)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		/*------------------------------------------------------------------*/
		/* definition of message object for synchronous data transmission   */
		/*------------------------------------------------------------------*/
		for (i = 0; i < CAN_CONFIG_MSG_MAIN_OBJ_TX_NR; i++)
		{
			err = can_main_get_next_free_msg_obj(&msg_obj);
			if (err == LI_CAN_SLV_ERR_OK)
			{
				// define message object for transmitting synchronous data
				err = can_main_define_msg_obj(msg_obj, CAN_CONFIG_ONLINE_ARB_ID, CAN_CONFIG_ACCEPTANCE_ONE_ID, CAN_CONFIG_ONLINE_DLC, CAN_CONFIG_DIR_TX, CAN_MAIN_SERVICE_ID_TX, CAN_OBJECT_IS_SYNC);
				if (err == LI_CAN_SLV_ERR_OK)
				{
					// add CAN_CONFIG_MSG_MAIN_OBJ_TX_NR of TX objects
					err = can_main_msg_obj_tx_data_cnfg(msg_obj);
				}
			}
		}
	}
#endif	// #if defined(OUTER) || defined(OUTER_APP)

#if defined(OUTER) || defined(OUTER_APP)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_sys_init();
	}
#endif // #if defined(OUTER) || defined(OUTER_APP)

#ifdef CAN_ASYNC_CTRL_RX_QUEUE
	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_async_init();
	}
#endif // #ifdef CAN_ASYNC_CTRL_RX_QUEUE

#if defined(OUTER) || defined(OUTER_APP)
	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_sync_init();
	}
#endif // #if defined(OUTER) || defined(OUTER_APP)

#ifdef LI_CAN_SLV_RECONNECT
#ifdef LI_CAN_SLV_MAIN_MON
	if (can_mainmon_type == CAN_MAINMON_TYPE_MAIN)
	{
#endif // #ifdef LI_CAN_SLV_MAIN_MON
		if (err == LI_CAN_SLV_ERR_OK)
		{
			err = li_can_slv_reconnect_init();
			if (err == LI_CAN_SLV_ERR_OK)
			{
				if (lcsa_get_state() == LI_CAN_SLV_STATE_DOWNLOAD)
				{
					li_can_slv_reconnect_set_state(CAN_RECONNECT_STATE_OFF);
					// in case a download is pending switch off the reconnect feature after reset
					err = li_can_slv_reconnect_download();
					if (err == LI_CAN_SLV_ERR_OK)
					{
						err = can_config_set_baudrate(baudrate);
					}

				}
				else
				{
					li_can_slv_reconnect_set_state(CAN_RECONNECT_STATE_STARTUP);
					// diagnostic connection to CAN bus
					err = li_can_slv_reconnect_startup();
					if (err == LI_CAN_SLV_ERR_OK)
					{
						err = can_config_set_baudrate_listen_only(baudrate);
					}
				}
			}
			else
			{
				// directly connection to CAN bus set normal baud rate
				if (err == LI_CAN_SLV_ERR_OK)
				{
					err = can_config_set_baudrate(baudrate);
				}
				can_main_enable();
#if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
				can_mon_enable();
#endif // #if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
			}
		}
#ifdef LI_CAN_SLV_MAIN_MON
	}
	else
	{
		// go online on monitor
		if (err == LI_CAN_SLV_ERR_OK)
		{
			err = can_config_set_baudrate(baudrate);
		}
	}
#endif // #ifdef LI_CAN_SLV_MAIN_MON

#ifdef LI_CAN_SLV_MAIN_MON
	if (can_mainmon_type == CAN_MAINMON_TYPE_MON)
	{
		// on the monitor CPU enable the main node normal
		can_main_enable();
	}
#endif // #ifdef LI_CAN_SLV_MAIN_MON

#else // #ifdef LI_CAN_SLV_RECONNECT

	lcsa_set_state(LI_CAN_SLV_STATE_RUNNING);

	if (err == LI_CAN_SLV_ERR_OK)
	{
		err = can_config_set_baudrate(baudrate);
	}

	can_main_enable();
#if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
	can_mon_enable();
#endif // #if defined (LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
#endif // #ifdef LI_CAN_SLV_RECONNECT

#ifdef CAN_SMP
	if ((err == LI_CAN_SLV_ERR_OK) && (can_state != LI_CAN_SLV_STATE_DOWNLOAD))
	{
		// activate CAN sample trace not in case of CAN download mode of outer core -> performance problems
		smp_cnfg_load();
		smp_trigger_extern();
	}
#endif // #ifdef CAN_SMP

#ifdef CAN_RANDOM_STATUS_ACKNOWLEDGE
	modhw_info.status_ackn_delay_nops = util_rand(CAN_MAX_STATUS_ACKN_DELAY_NOPS);
#endif // #ifdef CAN_RANDOM_STATUS_ACKNOWLEDGE

#ifdef LI_CAN_SLV_MAIN_MON
	if (can_mainmon_type == CAN_MAINMON_TYPE_MON)
	{
#endif // #ifdef LI_CAN_SLV_MAIN_MON
		err = can_hw_transceiver_enable();
#ifdef LI_CAN_SLV_MAIN_MON
	}
#endif // #ifdef LI_CAN_SLV_MAIN_MON
	return (err);
}

li_can_slv_errorcode_t li_can_slv_deinit(void)
{
	can_main_hw_deinit();
#if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
	can_mon_hw_deinit();
#endif // #if defined(LI_CAN_SLV_MON) || defined(CAN_NODE_B_USED_FOR_RECONNECT_ONLY)
	return LI_CAN_SLV_ERR_OK;
}

/*!
 * \brief can Hardware enable routine
 * \return li_can_slv_errorcode_t or LI_CAN_SLV_ERR_OK if successful
 */
li_can_slv_errorcode_t can_transceiver_disable(void)
{
	can_hw_transceiver_disable();
	return (LI_CAN_SLV_ERR_OK);
}

/**
* @todo rename can_dump_msg_objects
*/
#ifdef CAN_HW_DUMP_MSG_OBJECTS
/**
 * @param start_idx
 * @param end_idx
 * @return #li_can_slv_errorcode_t or #LI_CAN_SLV_ERR_OK if successful
 */
li_can_slv_errorcode_t can_dump_msg_objects(uint16_t start_idx, uint16_t end_idx)
{
	uint16_t i;
	LI_CAN_SLV_DEBUG_PRINT("\n");
	LI_CAN_SLV_DEBUG_PRINT("MSG ID   MASK    DLC DIR IDE MIDE D0   D1   D2   D3   D4   D5   D6   D7\n");

	for (i = start_idx; i < end_idx; i++)
	{
		can_hw_dump_msg_obj(i);
	}
	return (LI_CAN_SLV_ERR_OK);
}
#endif // #ifdef CAN_HW_DUMP_MSG_OBJECTS

/**
 * @return LI_CAN_SLV_ERR_OK
 */
li_can_slv_errorcode_t li_can_slv_process(void)
{
#ifdef DEBUG_LI_CAN_SLV_PRCOCESS
#define	NEXT_PRINT (10000)
	static uint32_t next_print_tick = can_port_msec_2_ticks(NEXT_PRINT);
	uint32_t tick;
#endif // #ifdef DEBUG_LI_CAN_SLV_PRCOCESS

#ifndef LI_CAN_SLV_BOOT
	/*----------------------------------------------------------------------*/
	/* Trigger process period update in case of missing PRs				    */
	/*----------------------------------------------------------------------*/
	li_can_slv_sync_trigger_process_periode();
#endif // #ifndef LI_CAN_SLV_BOOT

	can_main_hw_handler_error();

#ifdef DEBUG_LI_CAN_SLV_PRCOCESS
	tick = can_port_get_system_ticks();
	if (tick >= next_print_tick)
	{
		LI_CAN_SLV_DEBUG_PRINT("\nlcs proc(tick): %ld", tick);
		LI_CAN_SLV_DEBUG_PRINT("\nlcs proc(ms): %ld", can_port_msec_2_ticks(tick));
		next_print_tick = can_port_get_system_ticks() + can_port_msec_2_ticks(NEXT_PRINT);
	}
#endif // #ifdef DEBUG_LI_CAN_SLV_PRCOCESS

#if defined(OUTER) || defined(OUTER_APP)
#ifdef LI_CAN_SLV_ASYNC
#if defined(LI_CAN_SLV_ASYNC_TUNNEL)
	// process partly filled async tx data objects for a timed sending
	can_async_tunnel_process_tx_data();
#endif // #if defined(LI_CAN_SLV_ASYNC_TUNNEL)
#endif // #ifdef LI_CAN_SLV_ASYNC
#endif // #if defined(OUTER) || defined(OUTER_APP)
	return LI_CAN_SLV_ERR_OK;
}

void li_can_slv_set_mode(li_can_slv_mode_t new_mode)
{
	li_can_slv_mode = new_mode;
}

li_can_slv_mode_t li_can_slv_get_mode(void)
{
	return li_can_slv_mode;
}

void lcsa_set_state(lcsa_state_t new_state)
{
	li_can_slv_state = new_state;
}

lcsa_state_t lcsa_get_state(void)
{
	return li_can_slv_state;
}

/*--------------------------------------------------------------------------*/
/* function definition (private/not exported)                               */
/*--------------------------------------------------------------------------*/
/** @} */

