#include "Lawicel_parser.hpp"

#include <array>
#include <algorithm>

#include <cstring>
#include <cstdio>

bool Lawicel_parser::write_bell()
{
	return write_string("\b");
}
bool Lawicel_parser::write_cr()
{
	return write_string("\r");
}

bool Lawicel_parser::parse_std_id(const char* in_str, uint32_t* const id)
{
	//tiiildd
	std::array<char, 4> id_str;
	id_str.fill(0);
	std::copy_n(in_str+1, 3, id_str.data());

	unsigned int temp_id = 0;
	const int ret = sscanf(id_str.data(), "%x", &temp_id);
	if(ret != 1)
	{
		return false;
	}

	if(temp_id > 0x7FF)
	{
		return false;
	}

	*id = temp_id;

	return true;
}
bool Lawicel_parser::parse_ext_id(const char* in_str, uint32_t* const id)
{
	//Tiiiiiiiildd
	std::array<char, 9> id_str;
	id_str.fill(0);
	std::copy_n(in_str+1, 8, id_str.data());

	unsigned int temp_id = 0;
	const int ret = sscanf(id_str.data(), "%x", &temp_id);
	if(ret != 1)
	{
		return false;
	}

	if(temp_id > 0x1FFFFFFF)
	{
		return false;
	}

	*id = temp_id;

	return true;
}

bool Lawicel_parser::parse_std_dlc(const char* dlc_str, uint8_t* const dlc)
{
	const size_t dlc_str_len = strnlen(dlc_str, 1);
	if(dlc_str_len < 1)
	{
		return false;
	}

	unsigned int temp_dlc = 0;
	const int ret = sscanf(dlc_str, "%u", &temp_dlc);
	if(ret != 1)
	{
		return false;
	}

	if(temp_dlc > 8)
	{
		return false;
	}

	*dlc = temp_dlc;

	return true;
}
bool Lawicel_parser::parse_std_data(const char* data_str, const uint8_t dlc, std::array<uint8_t, 8>* const data)
{
	const size_t data_str_len = strnlen(data_str, 16);
	if(data_str_len < (dlc*2))
	{
		return false;
	}

	for(size_t i = 0; i < dlc; i++)
	{
		//tiiildd
		std::array<char, 3> data_i_str;
		data_i_str.fill(0);
		std::copy_n(data_str+2*i, 2, data_i_str.data());

		unsigned int d = 0;
		const int ret = sscanf(data_i_str.data(), "%x", &d);
		if(ret != 1)
		{
			return false;
		}

		(*data)[i] = d;
	}

	return true;
}

bool Lawicel_parser::parse_string(const char* in_str)
{
	if(in_str == nullptr)
	{
		return false;
	}

	bool ret = false;
	switch(in_str[0])
	{
		case 'S':
		{
			ret = parse_std_baud(in_str);
			break;
		}
		case 's':
		{
			ret = parse_cust_baud(in_str);
			break;
		}
		case 'O':
		{
			ret = parse_open(in_str);
			break;
		}
		case 'C':
		{
			ret = parse_close(in_str);
			break;
		}
		case 't':
		{
			ret = parse_tx_std(in_str);
			break;
		}
		case 'T':
		{
			ret = parse_tx_ext(in_str);
			break;
		}
		case 'r':
		{
			ret = parse_tx_rtr_std(in_str);
			break;
		}
		case 'R':
		{
			ret = parse_tx_rtr_ext(in_str);
			break;
		}
		case 'F':
		{
			ret = parse_get_flags(in_str);
			break;
		}
		case 'M':
		{
			ret = parse_set_accept_code(in_str);
			break;
		}
		case 'm':
		{
			ret = parse_set_accept_mask(in_str);
			break;
		}
		case 'V':
		{
			ret = parse_get_version(in_str);
			break;
		}
		case 'N':
		{
			ret = parse_get_serial(in_str);
			break;
		}
		case 'Z':
		{
			ret = parse_set_timestamp(in_str);
			break;
		}
		default:
		{
			ret = false;
			break;
		}
	}

	return ret;
}

bool Lawicel_parser::parse_std_baud(const char* in_str)
{
	const size_t in_str_len = strnlen(in_str, 3);

	if(in_str_len != 3)
	{
		write_string("\b");
		return false;
	}
	
	if(in_str[0] != 'S')
	{
		write_string("\b");
		return false;
	}

	if(in_str[2] != '\r')
	{
		write_string("\b");
		return false;
	}

	unsigned int baud = 0;
{
	int ret = sscanf(in_str, "S%u\r", &baud);
	if(ret != 1)
	{
		write_string("\b");
		return false;
	}
}

	if(baud > 8)
	{
		write_string("\b");
		return false;
	}

	if(!handle_std_baud(baud))
	{
		write_string("\b");
		return false;
	}

	write_string("\r");
	return true;
}
bool Lawicel_parser::parse_cust_baud(const char* in_str)
{
	const size_t in_str_len = strnlen(in_str, 6);

	if(in_str_len != 6)
	{
		write_string("\b");
		return false;
	}
	
	if(in_str[0] != 's')
	{
		write_string("\b");
		return false;
	}

	if(in_str[5] != '\r')
	{
		write_string("\b");
		return false;
	}

	unsigned int b0 = 0;
	unsigned int b1 = 0;

{
	std::array<char, 3> b0_str;
	b0_str.fill(0);
	
	std::copy_n(in_str+1, 2, b0_str.data());

	const int ret = sscanf(b0_str.data(), "%x", &b0);
	if(ret != 1)
	{
		write_string("\b");
		return false;
	}
}

{
	std::array<char, 3> b1_str;
	b1_str.fill(0);
	std::copy_n(in_str+3, 2, b1_str.data());

	const int ret = sscanf(b1_str.data(), "%x", &b1);
	if(ret != 1)
	{
		write_string("\b");
		return false;
	}
}
	if(!handle_cust_baud(b0, b1))
	{
		write_string("\b");
		return false;
	}

	write_string("\r");
	return true;
}

bool Lawicel_parser::parse_open(const char* in_str)
{
	const int ret = strncmp("O\r", in_str, 2);
	if(ret != 0)
	{
		write_string("\b");
		return false;
	}

	if(!handle_open())
	{
		write_string("\b");
		return false;
	}

	write_string("\r");
	return true;
}
bool Lawicel_parser::parse_close(const char* in_str)
{
	int ret = strncmp("C\r", in_str, 2);
	if(ret != 0)
	{
		write_string("\b");
		return false;
	}

	if(!handle_close())
	{
		write_string("\b");
		return false;
	}

	write_string("\r");
	return true;
}

bool Lawicel_parser::parse_tx_std(const char* in_str)
{
	const size_t in_str_len = strlen(in_str);

	if(in_str_len < 6)
	{
		write_string("\b");
		return false;
	}
	
	if(in_str[0] != 't')
	{
		write_string("\b");
		return false;
	}

	uint32_t id = 0;
	if(!parse_std_id(in_str, &id))
	{
		write_string("\b");
		return false;
	}

	uint8_t dlc = 0;
{
	//tiiildd
	std::array<char, 2> dlc_str;
	dlc_str.fill(0);
	std::copy_n(in_str+4, 1, dlc_str.data());

	if(!parse_std_dlc(dlc_str.data(), &dlc))
	{
		write_string("\b");
		return false;
	}
}

	//verify len
	if(in_str_len != (1U+3U+1U+2U*dlc+1U))
	{
		write_string("\b");
		return false;
	}

	std::array<uint8_t, 8> data;
	if(!parse_std_data(in_str+5, dlc, &data))
	{
		write_string("\b");
		return false;
	}

	if(!handle_tx_std(id, dlc, data.data()))
	{
		write_string("\b");
		return false;
	}

	return true;
}
bool Lawicel_parser::parse_tx_ext(const char* in_str)
{
	const size_t in_str_len = strlen(in_str);

	if(in_str_len < 6)
	{
		write_string("\b");
		return false;
	}
	
	if(in_str[0] != 't')
	{
		write_string("\b");
		return false;
	}

	uint32_t id = 0;
	if(!parse_ext_id(in_str, &id))
	{
		write_string("\b");
		return false;
	}

	uint8_t dlc = 0;
{
	std::array<char, 2> dlc_str;
	dlc_str.fill(0);
	std::copy_n(in_str+9, 1, dlc_str.data());

	if(!parse_std_dlc(dlc_str.data(), &dlc))
	{
		write_string("\b");
		return false;
	}
}

	//verify len
	if(in_str_len != (1U+8U+1U+2U*dlc+1U))
	{
		write_string("\b");
		return false;
	}

	std::array<uint8_t, 8> data;
	if(!parse_std_data(in_str+10, dlc, &data))
	{
		write_string("\b");
		return false;
	}

	if(!handle_tx_ext(id, dlc, data.data()))
	{
		write_string("\b");
		return false;
	}

	return true;
}

bool Lawicel_parser::parse_tx_rtr_std(const char* in_str)
{
	write_string("\b");
	return false;
}
bool Lawicel_parser::parse_tx_rtr_ext(const char* in_str)
{
	write_string("\b");
	return false;
}

bool Lawicel_parser::parse_get_flags(const char* in_str)
{
	write_string("\b");
	return false;
}

bool Lawicel_parser::parse_set_accept_code(const char* in_str)
{
	write_string("\b");
	return false;
}
bool Lawicel_parser::parse_set_accept_mask(const char* in_str)
{
	write_string("\b");
	return false;
}

bool Lawicel_parser::parse_get_version(const char* in_str)
{
	const int ret = strncmp("V\r", in_str, 2);
	if(ret != 0)
	{
		write_string("\b");
		return false;
	}

	std::array<uint8_t, 4> ver;
	if(!handle_get_version(&ver))
	{
		write_string("\b");
		return false;
	}

	std::array<uint8_t, 7> resp;
	resp[0] = 'V';
	std::copy_n(ver.data(), 4, resp.data()+1);
	resp[5] = '\r';
	resp[6] = '\0';

	write_string((char*)resp.data());

	return true;
}
bool Lawicel_parser::parse_get_serial(const char* in_str)
{
	const int ret = strncmp("N\r", in_str, 2);
	if(ret != 0)
	{
		write_string("\b");
		return false;
	}

	std::array<uint8_t, 4> sn;
	if(!handle_get_serial(&sn))
	{
		write_string("\b");
		return false;
	}

	std::array<uint8_t, 7> resp;
	resp[0] = 'N';
	std::copy_n(sn.data(), 4, resp.data()+1);
	resp[5] = '\r';
	resp[6] = '\0';

	write_string((char*)resp.data());

	return true;
}
bool Lawicel_parser::parse_set_timestamp(const char* in_str)
{
	write_string("\b");
	return false;
}