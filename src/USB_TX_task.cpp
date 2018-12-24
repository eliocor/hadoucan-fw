#include "USB_TX_task.hpp"

#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#include "hal_inst.hpp"
#include "uart1_printf.hpp"

#include <algorithm>

USB_TX_task::USB_TX_task()
{

}

void USB_TX_task::handle_init_callback()
{
	m_init_complete.give_from_isr();
}

void USB_TX_task::work()
{
	m_init_complete.take();
	
	for(;;)
	{
		// uart1_print<64>("tx wait buf\r\n");

		USB_buf* usb_buffer = nullptr;
		if(!m_pending_tx_buffers.pop_front(&usb_buffer, pdMS_TO_TICKS(50)))
		{
			// uart1_print<64>("tx pend is empty for 50 ms\r\n");
			// uart1_print<64>("tx send null buf\r\n");
			send_buffer(nullptr);
			continue;
		}

		// uart1_print<64>("tx send buf\r\n");

		send_buffer(usb_buffer);
		
		// uart1_print<64>("tx free buf\r\n");

		Object_pool_base<USB_buf>::free(usb_buffer);
	}
}

void USB_TX_task::wait_tx_finish()
{
	USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceHS.pClassData;
	while(hcdc->TxState != 0)
	{
		vTaskDelay(1);
	}
}

size_t USB_TX_task::queue_buffer_blocking(const uint8_t* buf, const size_t len)
{
	size_t num_queued = 0;

	while(num_queued < len)
	{
		USB_buf* usb_buf = nullptr;
		do
		{
			usb_buf = tx_buf_pool.try_allocate_for_ticks(portMAX_DELAY);
		} while(usb_buf == nullptr);
		
		const size_t num_to_copy = std::min(len - num_queued, usb_buf->buf.size());
		std::copy_n(buf + num_queued, num_to_copy, usb_buf->buf.data() + num_queued);
		usb_buf->len = num_to_copy;
	
		m_pending_tx_buffers.push_back(usb_buf);
	
		num_queued += num_to_copy;		
	}

	return num_queued;
}

size_t USB_TX_task::queue_buffer(const uint8_t* buf, const size_t len, const TickType_t xTicksToWait)
{
	size_t num_queued = 0;

	while(num_queued < len)
	{
		USB_buf* usb_buf = tx_buf_pool.try_allocate_for_ticks(xTicksToWait);
		if(!usb_buf)
		{
			uart1_print<64>("tx could not alloc\r\n");
			return num_queued;
		}
		
		const size_t num_to_copy = std::min(len - num_queued, usb_buf->buf.size());
		std::copy_n(buf + num_queued, num_to_copy, usb_buf->buf.data() + num_queued);
		usb_buf->len = num_to_copy;
	
		m_pending_tx_buffers.push_back(usb_buf);
	
		num_queued += num_to_copy;		
	}

	return num_queued;
}

uint8_t USB_TX_task::send_buffer(USB_buf* const buf)
{
	wait_tx_finish();
    
    if(buf)
    {
		USBD_CDC_SetTxBuffer(&hUsbDeviceHS, buf->buf.data(), buf->len);
    }
    else
    {
		USBD_CDC_SetTxBuffer(&hUsbDeviceHS, nullptr, 0);
    }
	
	uint8_t result = USBD_CDC_TransmitPacket(&hUsbDeviceHS);

	return result;
}