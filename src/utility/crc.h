/***************************************************************************
 *      Mechanized Assault and Exploration Reloaded Projectfile            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef utility_crcH
#define utility_crcH

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <forward_list>

#include "SDL_endian.h"


uint32_t calcCheckSum(const char* data, size_t dataSize, uint32_t checksum);

uint32_t calcCheckSum(bool data, uint32_t checksum);
uint32_t calcCheckSum(char data, uint32_t checksum);
uint32_t calcCheckSum(signed char data, uint32_t checksum);
uint32_t calcCheckSum(unsigned char data, uint32_t checksum);
uint32_t calcCheckSum(signed short data, uint32_t checksum);
uint32_t calcCheckSum(unsigned short data, uint32_t checksum);
uint32_t calcCheckSum(signed int data, uint32_t checksum);
uint32_t calcCheckSum(unsigned int data, uint32_t checksum);
uint32_t calcCheckSum(signed long data, uint32_t checksum);
uint32_t calcCheckSum(unsigned long data, uint32_t checksum);
uint32_t calcCheckSum(signed long long data, uint32_t checksum);
uint32_t calcCheckSum(unsigned long long data, uint32_t checksum);
uint32_t calcCheckSum(float data, uint32_t checksum);
uint32_t calcCheckSum(double data, uint32_t checksum);

uint32_t calcCheckSum(const std::string& data, uint32_t checksum);

struct sCrcEnum
{
	template <typename T>
	static uint32_t getChecksum(T data, uint32_t crc)
	{
		int value = static_cast<int>(data);
		SDL_SwapLE32(value);
		return calcCheckSum(reinterpret_cast<char*> (&value), sizeof(value), crc);
	}
};

struct sCrcClass
{
	template <typename T>
	static uint32_t getChecksum(const T& data, uint32_t crc)
	{
		return data.getChecksum(crc);
	}
};

template<typename T>
uint32_t calcCheckSum(const T& data, uint32_t crc)
{
	typedef typename std::conditional
		<
		std::is_enum<T>::value, 
		sCrcEnum, 
		sCrcClass
		>::type crcWrapper;
	
	return crcWrapper::getChecksum(data, crc);
};

template<typename T>
uint32_t calcCheckSum(const std::vector<T>& data, uint32_t checksum)
{
	for (const auto& x : data)
		checksum = calcCheckSum(x, checksum);

	return checksum;
};

template<typename T>
uint32_t calcCheckSum(const std::forward_list<T>& data, uint32_t checksum)
{
	for (const auto& x : data)
		checksum = calcCheckSum(x, checksum);

	return checksum;
};

#endif // utility_crcH
