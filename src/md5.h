/* md5.c - Functions to compute MD5 message digest of files or memory blocks
 * according to the definition of MD5 in RFC 1321 from April 1992.
 *
 * Thanks to Ulrich Drepper for the md5sum example code
 * 
 * Copyright (C) 2002 by the Widelands Development Team
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __S__MD5_H
#define __S__MD5_H

#include <iostream.h>

/** 
 *
 * This class is responsible of creating a streamind md5 checksum.
 * You simply pass it the data (using pass_data()) and if you want
 * to read the checksum, you do a finish_chksum() and a get_chksum()
 *
 * Pros:	Checksum can be generated by streaming: simply call pass_data()
 * with every junk of data you've read from a file
 *
 * Cons: 16 byte checksum is to big for network transport
 */
class ChkSum {
		  ChkSum(const ChkSum&);
		  ChkSum operator=(const ChkSum&);
		  
		  public:
					 ChkSum(void);
					 ~ChkSum(void);
					 void pass_data(const void*, uint);
					 void finish_chksum(void);
					 ulong* get_chksum(void) const { if(can_handle_data) return 0; return (ulong*)sum; }


		  private:
					 static const unsigned int BLOCKSIZE=4096; 
					 static const unsigned char fillbuf[64]  = { 0x80, 0 /* , 0, 0, ...  */ };

					 /* Structure to save state of computation between the single steps.  */
					 struct md5_ctx
					 {
								ulong A;
								ulong B;
								ulong C;
								ulong D;

								ulong total[2];
								ulong buflen;
								char buffer[128];
					 } ctx;

					 void md5_process_block (const void*, ulong, md5_ctx*);
					 void md5_process_bytes (const void*, ulong, md5_ctx*);
					 void* md5_finish_ctx (md5_ctx*, void*);

					 char buf[BLOCKSIZE+72];
					 uint nread;
					 char sum[16];
					 bool can_handle_data;
					
					 
};
					 
// operator overloading
bool operator==(ChkSum&, ChkSum&);
bool operator==(ChkSum&, const void*);
ostream& operator<<(ostream&, ChkSum&);

#endif /* __S__MD5_H */
