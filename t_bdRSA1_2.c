/* $Id: t_bdRSA1.c $ */

/******************** SHORT COPYRIGHT NOTICE**************************
This source code is part of the BigDigits multiple-precision
arithmetic library Version 2.4 originally written by David Ireland,
copyright (c) 2001-13 D.I. Management Services Pty Limited, all rights
reserved. It is provided "as is" with no warranties. You may use
this software under the terms of the full copyright notice
"bigdigitsCopyright.txt" that should have been included with this
library or can be obtained from <www.di-mgt.com.au/bigdigits.html>.
This notice must always be retained in any copy.
******************* END OF COPYRIGHT NOTICE***************************/
/*
	Last updated:
	$Date: 2013-04-27 17:19:00 $
	$Revision: 2.4.0 $
	$Author: dai $
*/

/* Test BigDigits "bd" functions using a new RSA key and user input plaintext 

   NOTE: this uses a different algorithm to generate the random number (a better one,
   still not quite cryptographically secure, but much better than using rand()).
   It also uses a more convenient key length of 1024 which is an exact multiple of 8.
   The "message" to be encrypted is accepted from the command-line or defaults to "abc".
*/

#if _MSC_VER >= 1100
	/* Detect memory leaks in MSVC++ */ 
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#else
	#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "bigd.h"
#include "bigdRand.h"

int RSA_driver();
void encrypt(int klen, unsigned char *pmsg, BIGD n, BIGD e, BIGD c, BIGD m, unsigned char *outMsg);
void Decryptor(unsigned char *outMsg, BIGD m1, BIGD d, BIGD n, BIGD c, int klen);

static void pr_bytesmsg(char *msg, unsigned char *bytes, size_t len)
{
	size_t i;
	printf("%s", msg);
	for (i = 0; i < len; i++)
		printf("%02x", bytes[i]);
	printf("\n");
}

int generateRSAKey(BIGD n, BIGD e, BIGD d, BIGD p, BIGD q, BIGD dP, BIGD dQ, BIGD qInv,
	size_t nbits, bdigit_t ee, size_t ntests, unsigned char *seed, size_t seedlen, 
	BD_RANDFUNC randFunc)
{
	BIGD g, p1, q1, phi;
	size_t np, nq;
	unsigned char *myseed = NULL;
//	clock_t start, finish;
//	double duration, tmake;
	int res;

	/* Initialise */
	g = bdNew();
	p1 = bdNew();
	q1 = bdNew();
	phi = bdNew();

	printf("Generating a %d-bit RSA key...\n", nbits);
	
	/* Set e as a BigDigit from short value ee */
	bdSetShort(e, ee);
	bdPrintHex("e=", e, "\n");

	/* We add an extra byte to the user-supplied seed */
	myseed = malloc(seedlen + 1);
	if (!myseed) return -1;
	memcpy(myseed, seed, seedlen);
	/* Make sure seeds are slightly different for p and q */
	myseed[seedlen] = 0x01;

	/* Do (p, q) in two halves, approx equal */
	nq = nbits / 2 ;
	np = nbits - nq;

	/* Compute two primes of required length with p mod e > 1 and *second* highest bit set */
	// start = clock();
	do {
		bdGeneratePrime(p, np, ntests, myseed, seedlen+1, randFunc);
		bdPrintHex("Try p=", p, "\n");
	} while ((bdShortMod(g, p, ee) == 1) || bdGetBit(p, np-2) == 0);
	// finish = clock();
	// duration = (double)(finish - start) / CLOCKS_PER_SEC;
	// tmake = duration;
	// printf("p is %d bits, bit(%d) is %d\n", bdBitLength(p), np-2, bdGetBit(p,np-2));

	myseed[seedlen] = 0xff;
	// start = clock();
	do {
		bdGeneratePrime(q, nq, ntests, myseed, seedlen+1, randFunc);
		bdPrintHex("Try q=", q, "\n");
	} while ((bdShortMod(g, q, ee) == 1) || bdGetBit(q, nq-2) == 0);

	// finish = clock();
	// duration = (double)(finish - start) / CLOCKS_PER_SEC;
	// tmake += duration;
	// printf("q is %d bits, bit(%d) is %d\n", bdBitLength(q), nq-2, bdGetBit(q,nq-2));
	// printf("Prime generation took %.3f secs\n", duration); 

	/* Compute n = pq */
	bdMultiply(n, p, q);
	bdPrintHex("n=\n", n, "\n");
	printf("n is %d bits\n", bdBitLength(n));
	assert(bdBitLength(n) == nbits);

	/* Check that p != q (if so, RNG is faulty!) */
	assert(!bdIsEqual(p, q));

	/* If q > p swap p and q so p > q */
	if (bdCompare(p, q) < 1)
	{	
		printf("Swopping p and q so p > q...\n");
		bdSetEqual(g, p);
		bdSetEqual(p, q);
		bdSetEqual(q, g);
	}
	bdPrintHex("p=", p, "\n");
	bdPrintHex("q=", q, "\n");

	/* Calc p-1 and q-1 */
	bdSetEqual(p1, p);
	bdDecrement(p1);
	bdPrintHex("p-1=\n", p1, "\n");
	bdSetEqual(q1, q);
	bdDecrement(q1);
	bdPrintHex("q-1=\n", q1, "\n");

	/* Compute phi = (p-1)(q-1) */
	bdMultiply(phi, p1, q1);
	bdPrintHex("phi=\n", phi, "\n");

	/* Check gcd(phi, e) == 1 */
	bdGcd(g, phi, e);
	bdPrintHex("gcd(phi,e)=", g, "\n");
	assert(bdShortCmp(g, 1) == 0);

	/* Compute inverse of e modulo phi: d = 1/e mod (p-1)(q-1) */
	res = bdModInv(d, e, phi);
	assert(res == 0);
	bdPrintHex("d=\n", d, "\n");

	/* Check ed = 1 mod phi */
	bdModMult(g, e, d, phi);
	bdPrintHex("ed mod phi=", g, "\n");
	assert(bdShortCmp(g, 1) == 0);

	/* Calculate CRT key values */
	printf("CRT values:\n");
	bdModInv(dP, e, p1);
	bdModInv(dQ, e, q1);
	bdModInv(qInv, q, p);
	bdPrintHex("dP=", dP, "\n");
	bdPrintHex("dQ=", dQ, "\n");
	bdPrintHex("qInv=", qInv, "\n");

	printf("n is %d bits\n", bdBitLength(n));

	/* Clean up */
	if (myseed) free(myseed);
	bdFree(&g);
	bdFree(&p1);
	bdFree(&q1);
	bdFree(&phi);

	return 0;
}

#define KEYSIZE 1024

static int debug = 0;


int main()
{


	RSA_driver();

}

int RSA_driver()
{
	size_t nbits = KEYSIZE;	/* NB a multiple of 8 here */
	unsigned char *pmsg = "Hi how are you ?.. !!";
	unsigned char *outMsg = "";
	
	unsigned ee = 0x3;
	size_t ntests = 50;
	unsigned char *seed = NULL;
	size_t seedlen = 0;
	
	BIGD n, e, d, p, q, dP, dQ, qInv;
	BIGD m, c, s, m1; 
	int res;
	/* How big is the key in octets (8-bit bytes)? */
	int klen = (nbits + 7) / 8;
	//printf("Test BIGDIGITS with a new %d-bit RSA key and given message data.\n", nbits);

	/* Initialise */
	p = bdNew();
	q = bdNew();
	n = bdNew();
	e = bdNew();
	d = bdNew();
	dP= bdNew();
	dQ= bdNew();
	qInv= bdNew();
	m = bdNew();
	c = bdNew();
	s = bdNew();
	m1 = bdNew();

	/* Create RSA key pair (n, e),(d, p, q, dP, dQ, qInv) */
	/* NB you should use a proper cryptographically-secure RNG */
	res = generateRSAKey(n, e, d, p, q, dP, dQ, qInv, nbits, ee, ntests, seed, seedlen, bdRandomOctets);
	
	// main loop <<======--------<<<<<<<<<<<<
	// while(1)
	// {
encrypt(klen, pmsg, n, e, c, m, outMsg);
Decryptor(outMsg, m1, d, n, c, klen);
// if (resp == 1)
// break;

	// }

	//clean
	if (res != 0)
	{
		printf("Failed to generate RSA key!\n");
		
		bdFree(&n);
		bdFree(&e);
		bdFree(&d);
		bdFree(&p);
		bdFree(&q);
		bdFree(&dP);
		bdFree(&dQ);
		bdFree(&qInv);
		bdFree(&m);
		bdFree(&c);
		bdFree(&s);
		bdFree(&m1);
	}

	return 0;
}

void encrypt(int klen, unsigned char *pmsg, BIGD n, BIGD e, BIGD c, BIGD m, unsigned char *outMsg)
{
	/* Create a PKCS#1 v1.5 EME message block in octet format */
		/*
		|<-----------------(klen bytes)--------------->|
		+--+--+-------+--+-----------------------------+
		|00|02|PADDING|00|      DATA TO ENCRYPT        |
		+--+--+-------+--+-----------------------------+
		The padding is made up of _at least_ eight non-zero random bytes.
		*/
	int npad, i, mlen;
	unsigned char block[(KEYSIZE+7)/8];
	unsigned char rb;
	
	/* CAUTION: make sure the block is at least klen bytes long */
	memset(block, 0, klen);
	mlen = strlen( (char*)pmsg );
	npad = klen - mlen - 3;
	if (npad < 8)	/* Note npad is a signed int, not a size_t */
	{
		printf("Message is too long\n");
		exit(1);
	}
	/* Display */
	//printf("Message='%s' ", pmsg);
	//pr_bytesmsg("0x", pmsg, strlen((char*)pmsg));

	/* Create encryption block */
	block[0] = 0x00;
	block[1] = 0x02;
	/* Generate npad non-zero padding bytes - rand() is OK */
	srand((unsigned)time(NULL));
	for (i = 0; i < npad; i++)
	{
		while ((rb = (rand() & 0xFF)) == 0)
			;/* loop until non-zero*/
		block[i+2] = rb;
	}
	block[npad+2] = 0x00;
	memcpy(&block[npad+3], pmsg, mlen);

	/* Convert to BIGD format */
	bdConvFromOctets(m, block, klen);

	bdPrintHex("m=\n", m, "\n");

	/* Encrypt c = m^e mod n */
	bdModExp(c, m, e, n);
	bdPrintHex("c=\n", c, "\n");
	//bdConvToDecimal(c, outMsg,1000*sizeof(int)/*buffer size*/);
	//return outMsg - our message
}

void Decryptor(unsigned char *outMsg, BIGD m1, BIGD d, BIGD n, BIGD c, int klen)
{
	unsigned char block[(KEYSIZE+7)/8];
	char msgstr[sizeof(block)];
	//int res; 
	int i;
	int nchars;
//	char msgstr[sizeof(block)];

	//we take outMsg and conv it to bd

	//bdConvFromDecimal(c, (const char*)outMsg);
	/* Check decrypt m1 = c^d mod n */
	
	bdModExp(m1, c, d, n);

	bdPrintHex("m'=\n", m1, "\n");
	//res = bdCompare(m1, m);
	//printf("Decryption %s\n", (res == 0 ? "OK" : "FAILED!"));
	//assert(res == 0);
	//printf("Decrypt by inversion took %.3f secs\n", tinv);

	/* Extract the message bytes from the decrypted block */
	memset(block, 0, klen);
	bdConvToOctets(m1, block, klen);
	assert(block[0] == 0x00);
	assert(block[1] == 0x02);
	for (i = 2; i < klen; i++)
	{	/* Look for zero separating byte */
		if (block[i] == 0x00)
			break;
	}
	if (i >= klen)
		printf("ERROR: failed to find message in decrypted block\n");
	else
	{
		nchars = klen - i - 1;
		memcpy(msgstr, &block[i+1], nchars);
		msgstr[nchars] = '\0';
		printf("Decrypted message is '%s'\n", msgstr);
	}
}