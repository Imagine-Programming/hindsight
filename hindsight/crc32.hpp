#pragma once

#ifndef checksum_crc32_h
#define checksum_crc32_h
	#include <cstdint>

	namespace Hindsight {
		namespace Checksum {
			/// <summary>
			/// A compile-time generated polynomial lookup table
			/// </summary>
			struct LookupTable {
				uint32_t data[256];

				/// <summary>
				/// Construct a new polynomial lookup table for the CRC32 checksum implementation below. The constructor 
				/// is constexpr and does nothing special, so the construction of this struct is constexpr as well. This 
				/// allows for describing how the table is built up and for an easy way of using a new polynomial without
				/// having to generate new tables yourself. 
				/// </summary>
				/// <param name="polynomial"></param>
				constexpr LookupTable(const uint32_t polynomial = 0xEDB88320) : data() {
					for (uint32_t i = 0; i < 256; i++) {
						auto c = i;

						for (size_t j = 0; j < 8; j++) {
							if (c & 1) {
								c = polynomial ^ (c >> 1);
							} else {
								c >>= 1;
							}
						}

						data[i] = c;
					}
				}
			};

			/// <summary>
			/// A very basic CRC32 implementation to be used in the hindsight binary log files. This CRC 
			/// implementation is not written towards optimization, but merely to having a simple checksum.
			/// </summary>
			struct Crc32 {
				/// <summary>
				/// A static default lookup table, but any lookup table may be used.
				/// </summary>
				static constexpr LookupTable Default = LookupTable();

				/// <summary>
				/// Update the checksum <paramref name="initial"/> with <paramref name="data"/> using the lookup table 
				/// <paramref name="table"/>.
				/// </summary>
				/// <param name="buf">The data to update the checksum with.</param>
				/// <param name="len">The size of the data pointed to by <paramref name="data"/>.</param>
				/// <param name="table">The lookup table.</param>
				/// <param name="initial">The initial checksum or previous iteration.</param>
				/// <returns>The updated checksum.</returns>
				static uint32_t Update(const void* buf, size_t len, const LookupTable& table, uint32_t initial) {
					auto c = initial ^ 0xFFFFFFFF;
					auto u = static_cast<const uint8_t*>(buf);
					for (size_t i = 0; i < len; ++i)
						c = table.data[(c ^ u[i]) & 0xFF] ^ (c >> 8);
					return c ^ 0xFFFFFFFF;
				}

				/// <summary>
				/// Update the checksum <paramref name="initial"/> with <paramref name="data"/> using the default lookup table 
				/// <see cref="::Hindsight::Checksum::Crc32::Default"/>.
				/// </summary>
				/// <param name="buf">The data to update the checksum with.</param>
				/// <param name="len">The size of the data pointed to by <paramref name="data"/>.</param>
				/// <param name="initial">The initial checksum or previous iteration.</param>
				/// <returns>The updated checksum.</returns>
				static uint32_t Update(const void* buf, size_t len, uint32_t initial) {
					return Update(buf, len, Default, initial);
				}
			};
		}
	}
#endif 