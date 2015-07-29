#ifndef STD_BF
#define STD_BF 

#include "hash_function.h"
#include <cstring>
#include <iostream>

using namespace std;

class SBF 
{
public:	
	
	SBF(unsigned int m, unsigned int k)
	{
		bf_m = m;
		bf_k = k;
		bf_n = 0;

		//printf("The size of the bloom filter is: %d\n", bf_m);

		if(bf_k > 26) {
			cerr << "the # of hash functions cannot exceed 18" << endl;
		}

		//cout << "m = " << bf_m << " k = " << bf_k << " type = " << type << endl;

		bf_k64 = new unsigned long long[(bf_m>>6)+1];
		
		memset(bf_k64, 0, sizeof(unsigned long long) * ((bf_m >> 6)+1));

		unsigned int (* tmp_ptr[26])(const unsigned char * str, unsigned int len) = 
		{ 
			JSHash, OCaml, OAAT, PJWHash, RSHash, SBOX, SDBM, Simple, SML, STL,
			APHash, BKDR, BOB, CRC32, DEKHash, DJBHash, FNV32, Hsieh, BOB1, BOB2,
			BOB3, BOB4, BOB5, BOB6, BOB7, BOB8
		};
		
		for(int i = 0; i < 26; i++){
			bf_hfp[i] = tmp_ptr[i];
		}
	}

	~SBF()
	{
		delete [] bf_k64;
	}

	/*
	*	insert_k and query_k
	*
	*	We use a 64-bit array to implement the basic Bloom filter with k hash functions
	*	
	*	For each hash function, we first calculate the slot and offset,
	*	then we operate on one element
	*
	*/

	unsigned int insert_k(const unsigned char * str, unsigned int len)
	{
		unsigned int value = 0;
		unsigned int slot = 0;
		unsigned int offset = 0;
		unsigned long long unit = 0;

		for (uint i = 0; i < bf_k; i++)
		{
			value = bf_hfp[i](str, len)%bf_m;

			slot = (value>>6);
			offset = (value&63);
			
			unit = bf_k64[slot];
			unit = unit | (1i64 << offset);
			bf_k64[slot] = unit;
		}

		bf_n++;
		
		return 1;
	}

	unsigned int query_k(const unsigned char * str, unsigned int len)
	{
		unsigned int value = 0;
		unsigned int slot = 0;
		unsigned int offset = 0;
		unsigned long long unit = 0;

		//cout << str[0] << "	" << str[1] << "	" << str[2] << "	" << str[3]<< endl;

		for (uint i = 0; i < bf_k; i++)
		{
			value = bf_hfp[i](str, len)%bf_m;

			//cout << "The " << i << "th hash value is: " << value << endl;

			slot = (value>>6);
			offset = (value&63);
			
			unit = bf_k64[slot];
			if (!(unit & (1i64 << offset)))
				return 0;
		}

		return 1;
	}
		
	unsigned long long * bf_k64;	// utilize a 64-bit array to implement the Standard Bloom Filter

	unsigned int bf_m; //bloom filter length
	unsigned int bf_k; //hash function numbers;
	unsigned int bf_n; //# of elements inserted

private:
	//pointers to hash function
	unsigned int (*bf_hfp[26])(const unsigned char * str, unsigned int len);
};
#endif