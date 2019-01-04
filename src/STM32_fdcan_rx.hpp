#pragma once

#include "USB_tx_buffer_task.hpp"

#include "freertos_cpp_util/Task_static.hpp"
#include "freertos_cpp_util/BSema_static.hpp"
#include "freertos_cpp_util/Queue_static_pod.hpp"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_fdcan.h"

#include <atomic>

class STM32_fdcan_rx : public Task_static<1024>
{
public:

	STM32_fdcan_rx()
	{
		m_usb_tx_buffer = nullptr;
	}

	void set_usb_tx(USB_tx_buffer_task* const usb_tx_buffer)
	{
		m_usb_tx_buffer = usb_tx_buffer;
	}
	
	void set_can_instance(FDCAN_GlobalTypeDef* const can)
	{
		m_fdcan = can;
	}

	void set_can_handle(FDCAN_HandleTypeDef* const can_handle)
	{
		m_fdcan_handle = can_handle;
	}

	void work();

	struct CAN_fd_packet
	{
		FDCAN_RxHeaderTypeDef rxheader;
		std::array<uint8_t, 64> data;
	};

	bool insert_packet_isr(CAN_fd_packet& pk);

	void can_fifo0_callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);
	void can_fifo1_callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs);

protected:

	static bool append_packet_type(const FDCAN_RxHeaderTypeDef& rxheader, std::string* const s);
	static bool append_packet_id(const FDCAN_RxHeaderTypeDef& rxheader, std::string* const s);
	static bool append_packet_data(const CAN_fd_packet& rxheader, std::string* const s);

	FDCAN_GlobalTypeDef* m_fdcan;
	FDCAN_HandleTypeDef* m_fdcan_handle;

	Queue_static_pod<CAN_fd_packet, 64> m_can_fd_queue;

	USB_tx_buffer_task* m_usb_tx_buffer;

	std::atomic<bool> m_can_fifo0_full;
	std::atomic<bool> m_can_fifo0_msg_lost;

	std::atomic<bool> m_can_fifo1_full;
	std::atomic<bool> m_can_fifo1_msg_lost;

};

extern STM32_fdcan_rx stm32_fdcan_rx_task;