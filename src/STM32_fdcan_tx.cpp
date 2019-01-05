#include "STM32_fdcan_tx.hpp"

#include "uart1_printf.hpp"

#include <array>
#include <algorithm>
#include <stdexcept>

namespace
{
uint32_t get_stm32_dlc_from_dlc(const uint8_t dlc)
{
	switch(dlc)
	{
		case 0x0:
			return FDCAN_DLC_BYTES_0;
		case 0x1: 
			return FDCAN_DLC_BYTES_1;
		case 0x2: 
			return FDCAN_DLC_BYTES_2;
		case 0x3:
			return FDCAN_DLC_BYTES_3;
		case 0x4:
			return FDCAN_DLC_BYTES_4;
		case 0x5:
			return FDCAN_DLC_BYTES_5;
		case 0x6:
			return FDCAN_DLC_BYTES_6;
		case 0x7:
			return FDCAN_DLC_BYTES_7;
		case 0x8:
			return FDCAN_DLC_BYTES_8;
		case 0x9:
			return FDCAN_DLC_BYTES_12;
		case 0xA:
			return FDCAN_DLC_BYTES_16;
		case 0xB:
			return FDCAN_DLC_BYTES_20;
		case 0xC:
			return FDCAN_DLC_BYTES_24;
		case 0xD:
			return FDCAN_DLC_BYTES_32;
		case 0xE:
			return FDCAN_DLC_BYTES_48;
		case 0xF:
			return FDCAN_DLC_BYTES_64;
		default:
			throw std::domain_error("dlc not in bounds");
	}

	throw std::domain_error("dlc not in bounds");
}
uint32_t get_size_from_dlc(const uint8_t dlc)
{
	switch(dlc)
	{
		case 0x0:
			return 0;
		case 0x1: 
			return 1;
		case 0x2: 
			return 2;
		case 0x3:
			return 3;
		case 0x4:
			return 4;
		case 0x5:
			return 5;
		case 0x6:
			return 6;
		case 0x7:
			return 7;
		case 0x8:
			return 8;
		case 0x9:
			return 12;
		case 0xA:
			return 16;
		case 0xB:
			return 20;
		case 0xC:
			return 24;
		case 0xD:
			return 32;
		case 0xE:
			return 48;
		case 0xF:
			return 64;
		default:
			throw std::domain_error("dlc not in bounds");
	}

	throw std::domain_error("dlc not in bounds");
}
}

bool STM32_fdcan_tx::init()
{
	HAL_StatusTypeDef ret = HAL_OK;

	*m_fdcan_handle = FDCAN_HandleTypeDef();

	m_fdcan_handle->Instance = m_fdcan;
	m_fdcan_handle->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
	m_fdcan_handle->Init.Mode = FDCAN_MODE_NORMAL;
	m_fdcan_handle->Init.AutoRetransmission = ENABLE;
	m_fdcan_handle->Init.TransmitPause = DISABLE;
	m_fdcan_handle->Init.ProtocolException = ENABLE;

	// 100 MHz CAN Clk
	// tq = NominalPrescaler x (1/fdcan_ker_ck)
	// tq = 5 x (1/100MHz) = 50ns

	m_fdcan_handle->Init.NominalPrescaler = 5;//1-512
	m_fdcan_handle->Init.NominalSyncJumpWidth = 8;//1-128
	// NominalTimeSeg1 = Propagation_segment + Phase_segment_1
	m_fdcan_handle->Init.NominalTimeSeg1 = 139;  //1-256 
	m_fdcan_handle->Init.NominalTimeSeg2 = 20;   //1-128

	m_fdcan_handle->Init.DataPrescaler = 5;//1-32
	m_fdcan_handle->Init.DataSyncJumpWidth = 8;//1-16
	m_fdcan_handle->Init.DataTimeSeg1 = 32;//1-32
	m_fdcan_handle->Init.DataTimeSeg2 = 16;//1-16

	m_fdcan_handle->Init.MessageRAMOffset = 0;//0 - 2560
	m_fdcan_handle->Init.StdFiltersNbr = 1;
	m_fdcan_handle->Init.ExtFiltersNbr = 1;
	// m_fdcan_handle->Init.RxFifo0ElmtsNbr = 16;
	// m_fdcan_handle->Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
	// m_fdcan_handle->Init.RxFifo1ElmtsNbr = 0;
	// m_fdcan_handle->Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
	m_fdcan_handle->Init.RxFifo0ElmtsNbr = 64;
	m_fdcan_handle->Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_64;
	m_fdcan_handle->Init.RxFifo1ElmtsNbr = 0;
	m_fdcan_handle->Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_64;
	m_fdcan_handle->Init.RxBuffersNbr = 0;

	m_fdcan_handle->Init.TxEventsNbr = 0;
	m_fdcan_handle->Init.TxBuffersNbr = 0;
	m_fdcan_handle->Init.TxFifoQueueElmtsNbr = 32;
	m_fdcan_handle->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
	m_fdcan_handle->Init.TxElmtSize = FDCAN_DATA_BYTES_64;

	ret = HAL_FDCAN_Init(m_fdcan_handle);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::init", "HAL_FDCAN_Init failed");
		return false;
	}

	// Configure Rx Std filter
	FDCAN_FilterTypeDef sFilter0;
	sFilter0.IdType = FDCAN_STANDARD_ID;
	sFilter0.FilterIndex = 0;
	sFilter0.FilterType = FDCAN_FILTER_MASK;
	sFilter0.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilter0.FilterID1 = 0x000;//filter
	// sFilter0.FilterID2 = 0x7FF;//mask all
	sFilter0.FilterID2 = 0x000;//mask none, match all
	ret = HAL_FDCAN_ConfigFilter(m_fdcan_handle, &sFilter0);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::init", "HAL_FDCAN_ConfigFilter failed");
		return false;
	}

	// Configure Rx Ext filter
	FDCAN_FilterTypeDef sFilter1;
	sFilter1.IdType = FDCAN_EXTENDED_ID;
	sFilter1.FilterIndex = 0;
	sFilter1.FilterType = FDCAN_FILTER_MASK;
	sFilter1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilter1.FilterID1 = 0x00000000;//filter
	// sFilter1.FilterID2 = 0x1FFFFFFF;//mask
	sFilter1.FilterID2 = 0x00000000;//mask none, match all
	ret = HAL_FDCAN_ConfigFilter(m_fdcan_handle, &sFilter1);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::init", "HAL_FDCAN_ConfigFilter failed");
		return false;
	}

	ret = HAL_FDCAN_ConfigFifoWatermark(m_fdcan_handle, FDCAN_CFG_RX_FIFO0, 16);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_ConfigFifoWatermark for FIFO0 failed");
		return false;
	}

	ret = HAL_FDCAN_ConfigRxFifoOverwrite(m_fdcan_handle, FDCAN_CFG_RX_FIFO0, FDCAN_RX_FIFO_OVERWRITE);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_ConfigRxFifoOverwrite for FIFO0 failed");
		return false;
	}

	ret = HAL_FDCAN_ActivateNotification(m_fdcan_handle, FDCAN_IT_RX_FIFO0_WATERMARK | FDCAN_IT_RX_FIFO0_FULL | FDCAN_IT_RX_FIFO0_MESSAGE_LOST, 0);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_ActivateNotification for FIFO0 failed");
		return false;
	}

	// ret = HAL_FDCAN_ConfigFifoWatermark(m_fdcan_handle, FDCAN_CFG_RX_FIFO1, 2);
	// if(ret != HAL_OK)
	// {
	// 	uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_ConfigFifoWatermark for FIFO1 failed");
	// 	return false;
	// }

	// ret = HAL_FDCAN_ConfigRxFifoOverwrite(m_fdcan_handle, FDCAN_CFG_RX_FIFO1, FDCAN_RX_FIFO_OVERWRITE);
	// if(ret != HAL_OK)
	// {
	// 	uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_ConfigRxFifoOverwrite for FIFO1 failed");
	// 	return false;
	// }

	// ret = HAL_FDCAN_ActivateNotification(m_fdcan_handle, FDCAN_IT_RX_FIFO1_WATERMARK | FDCAN_IT_RX_FIFO1_FULL | FDCAN_IT_RX_FIFO1_MESSAGE_LOST, 0);
	// ret = HAL_FDCAN_ActivateNotification(m_fdcan_handle, FDCAN_IT_RX_FIFO1_WATERMARK, 0);
	// if(ret != HAL_OK)
	// {
	// 	uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_ActivateNotification for FIFO1 failed");
	// 	return false;
	// }

	// ret = HAL_FDCAN_ConfigGlobalFilter(m_fdcan_handle, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO1, DISABLE, DISABLE);
	ret = HAL_FDCAN_ConfigGlobalFilter(m_fdcan_handle, FDCAN_REJECT, FDCAN_REJECT, DISABLE, DISABLE);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_ConfigGlobalFilter failed");
		return false;
	}

	return true;
}

bool STM32_fdcan_tx::open()
{
	HAL_StatusTypeDef ret = HAL_OK;

	if(!init())
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "STM32_fdcan_tx::init failed");
		return false;
	}

	ret = HAL_FDCAN_Start(m_fdcan_handle);
	if(ret != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "HAL_FDCAN_Start failed");
		return false;
	}

	if(HAL_FDCAN_IsRestrictedOperationMode(m_fdcan_handle))
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::open", "FDCAN is in Restricted Mode");
		return false;	
	}

	uart1_log<128>(LOG_LEVEL::INFO, "STM32_fdcan_tx::open", "CAN is open");
	m_is_open = true;

	return true;
}
bool STM32_fdcan_tx::close()
{
	// if(HAL_FDCAN_DeactivateNotification(m_fdcan_handle, FDCAN_IT_RX_FIFO0_WATERMARK | FDCAN_IT_RX_FIFO0_FULL | FDCAN_IT_RX_FIFO0_MESSAGE_LOST) != HAL_OK)
	if(HAL_FDCAN_DeactivateNotification(m_fdcan_handle, FDCAN_IT_RX_FIFO0_WATERMARK) != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::close", "HAL_FDCAN_DeactivateNotification for FIFO0 failed");
		return false;	
	}
/*
	// if(HAL_FDCAN_DeactivateNotification(m_fdcan_handle, FDCAN_IT_RX_FIFO1_WATERMARK | FDCAN_IT_RX_FIFO1_FULL | FDCAN_IT_RX_FIFO1_MESSAGE_LOST) != HAL_OK)
	if(HAL_FDCAN_DeactivateNotification(m_fdcan_handle, FDCAN_IT_RX_FIFO1_WATERMARK) != HAL_OK)
	{
		uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::close", "HAL_FDCAN_DeactivateNotification for FIFO1 failed");
		return false;	
	}
*/
	if(HAL_FDCAN_Stop(m_fdcan_handle) != HAL_OK)
	{
		return false;
	}

	uart1_log<128>(LOG_LEVEL::INFO, "STM32_fdcan_tx::open", "CAN is closed");
	m_is_open = false;

	return true;
}

bool STM32_fdcan_tx::tx_std(const uint32_t id, const uint8_t dlc, const uint8_t* data)
{
	if(!m_is_open)
	{
		uart1_log<128>(LOG_LEVEL::WARN, "STM32_fdcan_tx::tx_std", "Tried to send with closed interface");
		return false;
	}

	FDCAN_TxHeaderTypeDef tx_head;

	tx_head.Identifier = id;
	tx_head.IdType = FDCAN_STANDARD_ID;
	tx_head.TxFrameType = FDCAN_DATA_FRAME;
	tx_head.DataLength = get_stm32_dlc_from_dlc(dlc);
	tx_head.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	tx_head.BitRateSwitch = FDCAN_BRS_OFF;
	tx_head.FDFormat = FDCAN_CLASSIC_CAN;
	tx_head.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	tx_head.MessageMarker = 0;

	std::array<uint8_t, 8> out_data;
	std::copy_n(data, get_size_from_dlc(dlc), out_data.begin());

	return send_packet(tx_head, out_data.data());
}

bool STM32_fdcan_tx::tx_ext(const uint32_t id, const uint8_t dlc, const uint8_t* data)
{
	if(!m_is_open)
	{
		uart1_log<128>(LOG_LEVEL::WARN, "STM32_fdcan_tx::tx_ext", "Tried to send with closed interface");
		return false;
	}

	FDCAN_TxHeaderTypeDef tx_head;

	tx_head.Identifier = id;
	tx_head.IdType = FDCAN_EXTENDED_ID;
	tx_head.TxFrameType = FDCAN_DATA_FRAME;
	tx_head.DataLength = get_stm32_dlc_from_dlc(dlc);
	tx_head.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	tx_head.BitRateSwitch = FDCAN_BRS_OFF;
	tx_head.FDFormat = FDCAN_CLASSIC_CAN;
	tx_head.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	tx_head.MessageMarker = 0;

	std::array<uint8_t, 8> out_data;
	std::copy_n(data, get_size_from_dlc(dlc), out_data.begin());

	return send_packet(tx_head, out_data.data());
}

bool STM32_fdcan_tx::tx_std_rtr(const uint32_t id, const uint8_t dlc, const uint8_t* data)
{
	if(!m_is_open)
	{
		uart1_log<128>(LOG_LEVEL::WARN, "STM32_fdcan_tx::tx_std_rtr", "Tried to send with closed interface");
		return false;
	}

	FDCAN_TxHeaderTypeDef tx_head;

	tx_head.Identifier = id;
	tx_head.IdType = FDCAN_STANDARD_ID;
	tx_head.TxFrameType = FDCAN_REMOTE_FRAME;
	tx_head.DataLength = get_stm32_dlc_from_dlc(dlc);
	tx_head.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	tx_head.BitRateSwitch = FDCAN_BRS_OFF;
	tx_head.FDFormat = FDCAN_CLASSIC_CAN;
	tx_head.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	tx_head.MessageMarker = 0;

	std::array<uint8_t, 8> out_data;
	std::copy_n(data, get_size_from_dlc(dlc), out_data.begin());

	return send_packet(tx_head, out_data.data());
}
bool STM32_fdcan_tx::tx_ext_rtr(const uint32_t id, const uint8_t dlc, const uint8_t* data)
{
	if(!m_is_open)
	{
		uart1_log<128>(LOG_LEVEL::WARN, "STM32_fdcan_tx::tx_ext_rtr", "Tried to send with closed interface");
		return false;
	}

	FDCAN_TxHeaderTypeDef tx_head;

	tx_head.Identifier = id;
	tx_head.IdType = FDCAN_EXTENDED_ID;
	tx_head.TxFrameType = FDCAN_REMOTE_FRAME;
	tx_head.DataLength = get_stm32_dlc_from_dlc(dlc);
	tx_head.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	tx_head.BitRateSwitch = FDCAN_BRS_OFF;
	tx_head.FDFormat = FDCAN_CLASSIC_CAN;
	tx_head.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	tx_head.MessageMarker = 0;

	std::array<uint8_t, 8> out_data;
	std::copy_n(data, get_size_from_dlc(dlc), out_data.begin());

	return send_packet(tx_head, out_data.data());
}

bool STM32_fdcan_tx::send_packet(FDCAN_TxHeaderTypeDef& tx_head, uint8_t* data)
{
	size_t retry_counter = 0;
	HAL_StatusTypeDef ret = HAL_OK;
	do
	{
		ret = HAL_FDCAN_AddMessageToTxFifoQ(m_fdcan_handle, &tx_head, data);

		if(ret != HAL_OK)
		{
			uart1_log<128>(LOG_LEVEL::DEBUG, "STM32_fdcan_tx::send_packet", "HAL_FDCAN_AddMessageToTxFifoQ failed, overflow?");

			if(retry_counter > 2)
			{
				uart1_log<128>(LOG_LEVEL::ERROR, "STM32_fdcan_tx::send_packet", "HAL_FDCAN_AddMessageToTxFifoQ failed, timeout");
				return false;
			}

			retry_counter++;
			vTaskDelay(1);
		}
	} while(ret != HAL_OK);

	return true;
}

extern "C"
{
	// void HAL_FDCAN_ClockCalibrationCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ClkCalibrationITs);
	// void HAL_FDCAN_TxEventFifoCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t TxEventFifoITs);
	// void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);
	// void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs);
	// void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef *hfdcan);
	// void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes);
	// void HAL_FDCAN_TxBufferAbortCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes);
	// void HAL_FDCAN_RxBufferNewMessageCallback(FDCAN_HandleTypeDef *hfdcan);
	// void HAL_FDCAN_HighPriorityMessageCallback(FDCAN_HandleTypeDef *hfdcan);
	// void HAL_FDCAN_TimestampWraparoundCallback(FDCAN_HandleTypeDef *hfdcan);
	// void HAL_FDCAN_TimeoutOccurredCallback(FDCAN_HandleTypeDef *hfdcan);
	// void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan);
}
