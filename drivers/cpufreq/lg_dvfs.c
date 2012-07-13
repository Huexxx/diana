/*
 *	linux/drivers/cpufreq/lg_dvfs.c
 *
 *	Copyright (C) 2010 Sookyoung Kim <sookyoung.kim@lge.com>
 *      Modified by Huexxx from xda forum <-huexxx@gmail.com>
 */

/* LKM START ***********************/
//#define __KERNEL__
#define MODULE

//#include <linux/modversions.h>
#include <linux/module.h>
#include <linux/init.h>
//#include <linux/kernel.h>
/* LKM END *************************/

#include "lg_dvfs.h"

/* LKM START ***********************/
#define DRIVER_AUTHOR	"Sookyoung Kim <sookyoung.kim@lge.com>"
#define DRIVER_DESC	"LG-DVFS"
/* LKM END *************************/

/****************************************************************************************
 * Variables and data structures
 ****************************************************************************************/

/****************************************************************************************
 * Function definitions
 ****************************************************************************************/

/*====================================================================
	The functions involved with U(20,12) fixed point format arithmetic.
	====================================================================*/

/* The unsigned multiplication function for U(20,12) format
	 fixed point fractional numbers.
	 Multiplicand and multiplier should be converted to
	 U(20,12) format fixed point number by << 12 before using the function.
	 Keep in mind that the calculation result should not exceed
	 0xffffffff or 1048575.999755859375, the maximum value U(20,12) can represent.
	 Otherwise, this function will cause segmentation error.
 */
void ds_fpmul12(
unsigned long multiplicand,
unsigned long multiplier,
unsigned long *multiplied)
{

/* umull	r4, r3, %1, %2		%1 = multiplicand, %2 = multiplier, 
 *								r3 = higher 32 bits, r4 = lower 32 bits.
 * mov		r4, r4, lsr #12		Logical right shift r4 by 12 bits
 * mov		r3, r3, lsl #20		Logical left shift r3 by 20 bits
 * orr		%0, r3, r4			Logical OR r3 and r4 and save it to *multimpied.
 */

//	printk(KERN_INFO "0x%lx * 0x%lx = ", multiplicand, multiplier);

	__asm__ __volatile__("umull	r4, r3, %1, %2\n\t"
						 "mov	r4, r4, lsr #12\n\t"
						 "mov	r3, r3, lsl #20\n\t"
						 "orr	%0, r3, r4"
						 : "=r" (*multiplied)
						 : "r" (multiplicand), "r" (multiplier)
						 : "r4", "r3");

//	printk(KERN_INFO "0x%lx\n", *multiplied);

	return;
}

/* The unsigned division function for U(20,12) format fixed point fractional numbers.
	 Dividend and divisor should be converted to U(20,12) format fixed point number
	 by << 12 before using the function.
	 Keep in mind that the calculation result should not exceed 0xffffffff or
	 1048575.999755859375, the maximum value U(20,12) can represent.
	 Otherwise, this function will cause segmentation error.
 */
void ds_fpdiv12(
unsigned long dividend,
unsigned long divisor,
unsigned long *quotient,
unsigned long *remainder)
{

	unsigned long long lc_dividend;

	lc_dividend = (unsigned long long)dividend;
	lc_dividend = lc_dividend << 12;

	//printk(KERN_INFO "0x%lx / 0x%lx = ", dividend, divisor);

	do_div(lc_dividend, divisor);

	*quotient = (unsigned long)lc_dividend;
	*remainder = 0;

	//printk(KERN_INFO "0x%lx + 0x%lx\n", *quotient, *remainder);

 return;
}

/* Normal 32-bit division.
	With this function, we can obtain both of the quotient and remainder at a time.
 */
void ds_div12(
unsigned long dividend,
unsigned long divisor,
unsigned long *quotient,
unsigned long *remainder)
{

	unsigned long long lc_dividend;

	lc_dividend = (unsigned long long)dividend;

//	printk(KERN_INFO "0x%lx / 0x%lx = ", dividend, divisor);

	do_div(lc_dividend, divisor);

	*quotient = (unsigned long)lc_dividend;
	*remainder = 0;

//	printk(KERN_INFO "0x%lx + 0x%lx\n", *quotient, *remainder);

 return;
}

int ds_find_first1_in_integer_part(unsigned long target_value){

	if(target_value == 0){
		return(0);
	}
	else{
		switch(target_value & 0xf0000000){
			case 0xf0000000: return(32);
			case 0xe0000000: return(32);
			case 0xd0000000: return(32);
			case 0xc0000000: return(32);
			case 0xb0000000: return(32);
			case 0xa0000000: return(32);
			case 0x90000000: return(32);
			case 0x80000000: return(32);
			case 0x70000000: return(31);
			case 0x60000000: return(31);
			case 0x50000000: return(31);
			case 0x40000000: return(31);
			case 0x30000000: return(30);
			case 0x20000000: return(30);
			case 0x10000000: return(29);
		}
		switch(target_value & 0xf000000){
			case 0xf000000: return(28);
			case 0xe000000: return(28);
			case 0xd000000: return(28);
			case 0xc000000: return(28);
			case 0xb000000: return(28);
			case 0xa000000: return(28);
			case 0x9000000: return(28);
			case 0x8000000: return(28);
			case 0x7000000: return(27);
			case 0x6000000: return(27);
			case 0x5000000: return(27);
			case 0x4000000: return(27);
			case 0x3000000: return(26);
			case 0x2000000: return(26);
			case 0x1000000: return(25);
		}
		switch(target_value & 0xf00000){
			case 0xf00000: return(24);
			case 0xe00000: return(24);
			case 0xd00000: return(24);
			case 0xc00000: return(24);
			case 0xb00000: return(24);
			case 0xa00000: return(24);
			case 0x900000: return(24);
			case 0x800000: return(24);
			case 0x700000: return(23);
			case 0x600000: return(23);
			case 0x500000: return(23);
			case 0x400000: return(23);
			case 0x300000: return(22);
			case 0x200000: return(22);
			case 0x100000: return(21);
		}
		switch(target_value & 0xf0000){
			case 0xf0000: return(20);
			case 0xe0000: return(20);
			case 0xd0000: return(20);
			case 0xc0000: return(20);
			case 0xb0000: return(20);
			case 0xa0000: return(20);
			case 0x90000: return(20);
			case 0x80000: return(20);
			case 0x70000: return(19);
			case 0x60000: return(19);
			case 0x50000: return(19);
			case 0x40000: return(19);
			case 0x30000: return(18);
			case 0x20000: return(18);
			case 0x10000: return(17);
		}
		switch(target_value & 0xf000){
			case 0xf000: return(16);
			case 0xe000: return(16);
			case 0xd000: return(16);
			case 0xc000: return(16);
			case 0xb000: return(16);
			case 0xa000: return(16);
			case 0x9000: return(16);
			case 0x8000: return(16);
			case 0x7000: return(15);
			case 0x6000: return(15);
			case 0x5000: return(15);
			case 0x4000: return(15);
			case 0x3000: return(14);
			case 0x2000: return(14);
			case 0x1000: return(13);
		}
		switch(target_value & 0xf00){
			case 0xf00: return(12);
			case 0xe00: return(12);
			case 0xd00: return(12);
			case 0xc00: return(12);
			case 0xb00: return(12);
			case 0xa00: return(12);
			case 0x900: return(12);
			case 0x800: return(12);
			case 0x700: return(11);
			case 0x600: return(11);
			case 0x500: return(11);
			case 0x400: return(11);
			case 0x300: return(10);
			case 0x200: return(10);
			case 0x100: return(9);
		}
		switch(target_value & 0xf0){
			case 0xf0: return(8);
			case 0xe0: return(8);
			case 0xd0: return(8);
			case 0xc0: return(8);
			case 0xb0: return(8);
			case 0xa0: return(8);
			case 0x90: return(8);
			case 0x80: return(8);
			case 0x70: return(7);
			case 0x60: return(7);
			case 0x50: return(7);
			case 0x40: return(7);
			case 0x30: return(6);
			case 0x20: return(6);
			case 0x10: return(5);
		}
		switch(target_value & 0xf){
			case 0xf: return(4);
			case 0xe: return(4);
			case 0xd: return(4);
			case 0xc: return(4);
			case 0xb: return(4);
			case 0xa: return(4);
			case 0x9: return(4);
			case 0x8: return(4);
			case 0x7: return(3);
			case 0x6: return(3);
			case 0x5: return(3);
			case 0x4: return(3);
			case 0x3: return(2);
			case 0x2: return(2);
			case 0x1: return(1);
		}
	}

	return(0);
}

int ds_find_first1_in_fraction_part(unsigned long target_value){

	if(target_value == 0){
		return(0);
	}
	else{
		switch(target_value & 0xf00){
			case 0xf00: return(1);
			case 0xe00: return(1);
			case 0xd00: return(1);
			case 0xc00: return(1);
			case 0xb00: return(1);
			case 0xa00: return(1);
			case 0x900: return(1);
			case 0x800: return(1);
			case 0x700: return(2);
			case 0x600: return(2);
			case 0x500: return(2);
			case 0x400: return(2);
			case 0x300: return(3);
			case 0x200: return(3);
			case 0x100: return(4);
		}
		switch(target_value & 0xf0){
			case 0xf0: return(5);
			case 0xe0: return(5);
			case 0xd0: return(5);
			case 0xc0: return(5);
			case 0xb0: return(5);
			case 0xa0: return(5);
			case 0x90: return(5);
			case 0x80: return(5);
			case 0x70: return(6);
			case 0x60: return(6);
			case 0x50: return(6);
			case 0x40: return(6);
			case 0x30: return(7);
			case 0x20: return(7);
			case 0x10: return(8);
		}
		switch(target_value & 0xf){
			case 0xf: return(9);
			case 0xe: return(9);
			case 0xd: return(9);
			case 0xc: return(9);
			case 0xb: return(9);
			case 0xa: return(9);
			case 0x9: return(9);
			case 0x8: return(9);
			case 0x7: return(10);
			case 0x6: return(10);
			case 0x5: return(10);
			case 0x4: return(10);
			case 0x3: return(11);
			case 0x2: return(11);
			case 0x1: return(12);
		}
	}

	return(0);
}

/* A sub function for ds_fpdiv().
	 This function compares (1 bit carry + 32 bits integer part + 12 bits fraction part)
	 of operand and operator.
 */
int ds_compare45bits(
int operand_carry, unsigned long operand_int_ulong, unsigned long operand_fra_fp12,
int operator_carry, unsigned long operator_int_ulong, unsigned long operator_fra_fp12)
{
	int lc_result = 0;

	lc_result = (operand_carry > operator_carry ? DS_LARGER :
			(operand_carry < operator_carry ? DS_SMALLER : 
			(operand_int_ulong > operator_int_ulong ? DS_LARGER :
			(operand_int_ulong < operator_int_ulong ? DS_SMALLER :
			(operand_fra_fp12 > operator_fra_fp12 ? DS_LARGER :
			(operand_fra_fp12 < operator_fra_fp12 ? DS_SMALLER : DS_EQUAL
			))))));

	return(lc_result);
}

/* A sub function for ds_fpdiv().
	 The carry for operand should be 0.
 */
int ds_shiftleft44bits(
int operand_carry, unsigned long operand_int_ulong, unsigned long operand_fra_fp12,
int shift,
int *operated_carry, unsigned long *operated_int_ulong, unsigned long *operated_fra_fp12)
{
	unsigned long targ_int_ulong = 0;
	unsigned long targ_fra_fp12 = 0;
	int operand_int_first1 = 0;

	*operated_carry = 0;
	*operated_int_ulong = 0;
	*operated_fra_fp12 = 0;

	if(operand_carry != 0){
		printk(KERN_INFO "[ds_shiftleft44bits] Error1: Shift left beyond 33 bits.\n");
		return(-1);
	}

	operand_int_first1 = ds_find_first1_in_integer_part(operand_int_ulong);

	if(operand_int_first1+shift > 33){
		printk(KERN_INFO "[ds_shiftleft44bits] Error2: Shift left beyond 33 bits.\n");
		return(-1);
	}
	else{
		if(operand_int_first1+shift == 33) *operated_carry = 1;
		targ_int_ulong = operand_int_ulong << shift;
		if(shift >= 12){
			targ_int_ulong |= operand_fra_fp12 << (shift-12);
			targ_fra_fp12 = 0x0;
		}
		else{
			switch(shift){
				case 11:
					targ_int_ulong |= operand_fra_fp12 >> 1;
					targ_fra_fp12 = (operand_fra_fp12 & 0x1) << 11;
					break;
				case 10:
					targ_int_ulong |= operand_fra_fp12 >> 2;
					targ_fra_fp12 = (operand_fra_fp12 & 0x3) << 10;
					break;
				case 9:
					targ_int_ulong |= operand_fra_fp12 >> 3;
					targ_fra_fp12 = (operand_fra_fp12 & 0x7) << 9;
					break;
				case 8:
					targ_int_ulong |= operand_fra_fp12 >> 4;
					targ_fra_fp12 = (operand_fra_fp12 & 0xf) << 8;
					break;
				case 7:
					targ_int_ulong |= operand_fra_fp12 >> 5;
					targ_fra_fp12 = (operand_fra_fp12 & 0x1f) << 7;
					break;
				case 6:
					targ_int_ulong |= operand_fra_fp12 >> 6;
					targ_fra_fp12 = (operand_fra_fp12 & 0x3f) << 6;
					break;
				case 5:
					targ_int_ulong |= operand_fra_fp12 >> 7;
					targ_fra_fp12 = (operand_fra_fp12 & 0x7f) << 5;
					break;
				case 4:
					targ_int_ulong |= operand_fra_fp12 >> 8;
					targ_fra_fp12 = (operand_fra_fp12 & 0xff) << 4;
					break;
				case 3:
					targ_int_ulong |= operand_fra_fp12 >> 9;
					targ_fra_fp12 = (operand_fra_fp12 & 0x1ff) << 3;
					break;
				case 2:
					targ_int_ulong |= operand_fra_fp12 >> 10;
					targ_fra_fp12 = (operand_fra_fp12 & 0x3ff) << 2;
					break;
				case 1:
					targ_int_ulong |= operand_fra_fp12 >> 11;
					targ_fra_fp12 = (operand_fra_fp12 & 0x7ff) << 1;
					break;
				case 0:
					targ_fra_fp12 |= operand_fra_fp12 & 0xfff;
			}
		}
		*operated_int_ulong = targ_int_ulong;
		*operated_fra_fp12 = targ_fra_fp12;
		return(0);
	}
}

/* A sub function for ds_fpdiv().
	 Operand should be larger than operator.
 */
int ds_subtract44bits(
int operand_carry, unsigned long operand_int_ulong, unsigned long operand_fra_fp12,
int operator_carry, unsigned long operator_int_ulong, unsigned long operator_fra_fp12,
int *operated_carry, unsigned long *operated_int_ulong, unsigned long *operated_fra_fp12)
{
	if(operand_carry == operator_carry){
		*operated_carry = 0;
		if(operand_fra_fp12 >= operator_fra_fp12){
			*operated_fra_fp12 = operand_fra_fp12 - operator_fra_fp12;
			*operated_int_ulong = operand_int_ulong - operator_int_ulong;
		}
		else{
			*operated_fra_fp12 = operand_fra_fp12 + 0x1000 - operator_fra_fp12;
			*operated_int_ulong = operand_int_ulong - 0x1 - operator_int_ulong;
		}
	}
	/* If operand_carry != operator_carry,
		 natually operand_carry == 1 and operator_carry == 0 */
	else{
		if(operand_int_ulong >= operator_int_ulong){
			*operated_carry = 1;
			if(operand_fra_fp12 >= operator_fra_fp12){
				*operated_fra_fp12 = operand_fra_fp12 - operator_fra_fp12;
				*operated_int_ulong = operand_int_ulong - operator_int_ulong;
			}
			else{
				*operated_fra_fp12 = operand_fra_fp12 + 0x1000 - operator_fra_fp12;
				*operated_int_ulong = operand_int_ulong - 0x1 - operator_int_ulong;
			}
		}
		else{
			*operated_carry = 0;
			if(operand_fra_fp12 >= operator_fra_fp12){
				*operated_fra_fp12 = operand_fra_fp12 - operator_fra_fp12;
				*operated_int_ulong = (0xffffffff - operator_int_ulong) + 1 + operand_int_ulong;
			}
			else{
				*operated_fra_fp12 = operand_fra_fp12 + 0x1000 - operator_fra_fp12;
				*operated_int_ulong = (0xffffffff - operator_int_ulong) + operand_int_ulong;
			}
		}
	}

	return(0);
}

/* ds_fpmul12 can treat only the case where the entire value
	 (integer part + fractional part) of the multiplicand, multiplier,
	 and multiplied all can be expressed in U(20,12) fixed point fractional number format,
	 i.e., max. 20 bits of integer part (for fractional parts, we don't need to worry about
	 because, whatever the fractional value is, it will be expressed in fixed 12 bits).

	 This function covers the rest cases that ds_fpmul12 can not treat by repeated additions
	 instead of the direct multiplication using ds_fpmul12.
	 For the case that ds_fpmul12 can treat, this function uses ds_fpmul12.

	 Note that multiplicand_fra_fp12 and multiplier_fra_fp12 should not include integer part.

	 multiplicand_int_ulong is the integer part of multiplicand.
	 multiplicand_fra_fp12 is the fraction part of multiplicand.
	 multiplier_int_ulong is the integer part of multiplier.
	 multiplier_fra_fp12 is the fraction part of multiplier.
	 multiplied_int_ulong is the integer part of multiplied.
	 multiplied_fra_fp12 is the fraction part of multiplied.
 */
int ds_fpmul(unsigned long multiplicand_int_ulong, unsigned long multiplicand_fra_fp12,
		 unsigned long multiplier_int_ulong, unsigned long multiplier_fra_fp12,
		 unsigned long *multiplied_int_ulong, unsigned long *multiplied_fra_fp12)
{
	int multiplicand_int_digits = 0;
	int multiplier_int_digits = 0;
	int i = 0;

	unsigned long operand_fp12 = 0;
	unsigned long operand_int_ulong = 0;
	unsigned long operand_fra_fp12 = 0;
	unsigned long operand_int_ulong_rest = 0;
	unsigned long operand_fp12_rest = 0;

	unsigned long operator_fp12 = 0;
	unsigned long operator_int_ulong = 0;
	unsigned long operator_fra_fp12 = 0;

	unsigned long operated_fp12 = 0;
	unsigned long operated_int_ulong = 0;
	unsigned long operated_fra_fp12 = 0;

	int operand_int_digits = 0;
	int operator_int_digits = 0;

	*multiplied_int_ulong = 0;
	*multiplied_fra_fp12 = 0;

	/* (1) Check if the integer part of the multiplied value will not be able to expressed
				 by a 32-bit unsigned long variable.
	 */
	multiplicand_int_digits = ds_find_first1_in_integer_part(multiplicand_int_ulong);
	multiplier_int_digits = ds_find_first1_in_integer_part(multiplier_int_ulong);
	if(multiplicand_int_digits + multiplier_int_digits > 32){
		printk(KERN_INFO "[ds_fpmul] multiplicand_int_ulong digits %d + \
			 multiplier_int_ulong digits %d exceeds 32\n",
			 multiplicand_int_digits, multiplier_int_digits);
		return(-1);
	}
	else{
		/* (2) If the integer + fraction of the multiplicand, multiplier, and multiplied
					 all can be represented in U(20,12), directly use ds_fpmul12.
		 */
		if(multiplicand_int_digits <= 20 &&
			 multiplier_int_digits <= 20 &&
			 multiplicand_int_digits + multiplier_int_digits <= 20)
		{
			operand_fp12 = DS_ULONG2FP12(multiplicand_int_ulong) | multiplicand_fra_fp12;
			operator_fp12 = DS_ULONG2FP12(multiplier_int_ulong) | multiplier_fra_fp12;
			ds_fpmul12(operand_fp12, operator_fp12, &operated_fp12);
			*multiplied_int_ulong = DS_GETFP12INT(operated_fp12);
			*multiplied_fra_fp12 = DS_GETFP12FRA(operated_fp12);
		}
		/* (3) If the integer + fraction of the multiplicand, multiplier, and multiplied
			 can not all be represented in U(20,12), use subsequent additions.

			 Now possible cases are

			 1. The integer + fraction of both the multiplicand and multiplier
				can be represented in U(20,12), but not for the multiplied.

			 2. The integer + fraction of either the multiplicand or multiplier
				can not be represented in U(20,12), and natually not for the multiplied.

			 Note that it is impossible that the integer + fraction of both the
			 multiplicand and multiplier can not be represented in U(20,12)
			 because such cases should have been filtered out by (1).
		 */
		else{

			/* Take one between the multiplicand and multiplier that can be respresented in
				 U(20,12) as operator.
			 */
			if(multiplicand_int_digits >= multiplier_int_digits){
				operand_int_digits = multiplicand_int_digits;
				operand_int_ulong = multiplicand_int_ulong;
				operand_fra_fp12 = multiplicand_fra_fp12;

				operator_int_digits = multiplier_int_digits;
				operator_int_ulong = multiplier_int_ulong;
				operator_fra_fp12 = multiplier_fra_fp12;
				operator_fp12 = DS_ULONG2FP12(multiplier_int_ulong) | multiplier_fra_fp12;
			}
			else{
				operand_int_digits = multiplier_int_digits;
				operand_int_ulong = multiplier_int_ulong;
				operand_fra_fp12 = multiplier_fra_fp12;

				operator_int_digits = multiplicand_int_digits;
				operator_int_ulong = multiplicand_int_ulong;
				operator_fra_fp12 = multiplicand_fra_fp12;
				operator_fp12 = DS_ULONG2FP12(multiplicand_int_ulong) | multiplicand_fra_fp12;
			}

			/* A multiplication is the sum of the operator shifted by each 1 in operand.
				 Thus, we repeatly add the operator (entire 32 bits) shifted by each 1 in operand
				 until the remaineder of the integer part of the operand becomes sufficiently small
				 such that the integer part of the operated is smaller than 20 bits.
				 Once the remainder of the integer part of the operand becomes sufficiently small,
				 then we use ds_fpmul12 for the remainder.
			 */
			for(i=operand_int_digits-1;i>(19-operator_int_digits);i--){

				/* Repeatedly add the shifted operator upon every 1 in operand */
				if((operand_int_ulong & (1<<i)) != 0){
					operated_int_ulong += operator_int_ulong << i;
					/* If the shift is not smaller than 12,
						 the entire operator_fra_fp12 becomes integer value.
						 Thus, just add the shifted operator_fra_fp12
						 to the integer part of the result. */
					if(i >= 12){
						operated_int_ulong += operator_fra_fp12 << (i-12);
					}
					/* If the shift is smaller than 12,
						 a fraction of operator_fra_fp12 will remains as fractional value.
						 Thus, just add the shifted operator_fra_fp12
						 to the integer part of the result. */
					else{
						switch(i){
							case 11:
								operated_int_ulong += operator_fra_fp12 >> 1;
								operated_fra_fp12 += (operator_fra_fp12 & 0x1) << 11;
								break;
							case 10:
								operated_int_ulong += operator_fra_fp12 >> 2;
								operated_fra_fp12 += (operator_fra_fp12 & 0x3) << 10;
								break;
							case 9:
								operated_int_ulong += operator_fra_fp12 >> 3;
								operated_fra_fp12 += (operator_fra_fp12 & 0x7) << 9;
								break;
							case 8:
								operated_int_ulong += operator_fra_fp12 >> 4;
								operated_fra_fp12 += (operator_fra_fp12 & 0xf) << 8;
								break;
							case 7:
								operated_int_ulong += operator_fra_fp12 >> 5;
								operated_fra_fp12 += (operator_fra_fp12 & 0x1f) << 7;
								break;
							case 6:
								operated_int_ulong += operator_fra_fp12 >> 6;
								operated_fra_fp12 += (operator_fra_fp12 & 0x3f) << 6;
								break;
							case 5:
								operated_int_ulong += operator_fra_fp12 >> 7;
								operated_fra_fp12 += (operator_fra_fp12 & 0x7f) << 5;
								break;
							case 4:
								operated_int_ulong += operator_fra_fp12 >> 8;
								operated_fra_fp12 += (operator_fra_fp12 & 0xff) << 4;
								break;
							case 3:
								operated_int_ulong += operator_fra_fp12 >> 9;
								operated_fra_fp12 += (operator_fra_fp12 & 0x1ff) << 3;
								break;
							case 2:
								operated_int_ulong += operator_fra_fp12 >> 10;
								operated_fra_fp12 += (operator_fra_fp12 & 0x3ff) << 2;
								break;
							case 1:
								operated_int_ulong += operator_fra_fp12 >> 11;
								operated_fra_fp12 += (operator_fra_fp12 & 0x7ff) << 1;
								break;
							case 0:
								operated_fra_fp12 += operator_fra_fp12 & 0xfff;
						}
						operated_int_ulong += DS_GETFP12INT(operated_fra_fp12);
						operated_fra_fp12 = DS_GETFP12FRA(operated_fra_fp12);
					}
				}
			}

			/* Now we can use ds_fpmul12.
				 Extract the remained operand,
				 i.e., the lower (20-operator_int_digits) digits only */
			switch(operator_int_digits){
				case 20: operand_int_ulong_rest = 0; break;
				case 19: operand_int_ulong_rest = operand_int_ulong & 0x1; break;
				case 18: operand_int_ulong_rest = operand_int_ulong & 0x3; break;
				case 17: operand_int_ulong_rest = operand_int_ulong & 0x7; break;
				case 16: operand_int_ulong_rest = operand_int_ulong & 0xf; break;
				case 15: operand_int_ulong_rest = operand_int_ulong & 0x1f; break;
				case 14: operand_int_ulong_rest = operand_int_ulong & 0x3f; break;
				case 13: operand_int_ulong_rest = operand_int_ulong & 0x7f; break;
				case 12: operand_int_ulong_rest = operand_int_ulong & 0xff; break;
				case 11: operand_int_ulong_rest = operand_int_ulong & 0x1ff; break;
				case 10: operand_int_ulong_rest = operand_int_ulong & 0x3ff; break;
				case 9: operand_int_ulong_rest = operand_int_ulong & 0x7ff; break;
				case 8: operand_int_ulong_rest = operand_int_ulong & 0xfff; break;
				case 7: operand_int_ulong_rest = operand_int_ulong & 0x1fff; break;
				case 6: operand_int_ulong_rest = operand_int_ulong & 0x3fff; break;
				case 5: operand_int_ulong_rest = operand_int_ulong & 0x7fff; break;
				case 4: operand_int_ulong_rest = operand_int_ulong & 0xffff; break;
				case 3: operand_int_ulong_rest = operand_int_ulong & 0x1ffff; break;
				case 2: operand_int_ulong_rest = operand_int_ulong & 0x3ffff; break;
				case 1: operand_int_ulong_rest = operand_int_ulong & 0x7ffff; break;
				case 0: operand_int_ulong_rest = operand_int_ulong & 0xfffff;
			}

			/* Call ds_fpmul12 */
			operand_fp12_rest = DS_ULONG2FP12(operand_int_ulong_rest) | operand_fra_fp12;
			operator_fp12 = DS_ULONG2FP12(operator_int_ulong) | operator_fra_fp12;
			ds_fpmul12(operand_fp12_rest, operator_fp12, &operated_fp12);
			operated_int_ulong += DS_GETFP12INT(operated_fp12);
			operated_fra_fp12 += DS_GETFP12FRA(operated_fp12);
			operated_int_ulong += DS_GETFP12INT(operated_fra_fp12);
			operated_fra_fp12 = DS_GETFP12FRA(operated_fra_fp12);
			*multiplied_int_ulong = operated_int_ulong;
			*multiplied_fra_fp12 = operated_fra_fp12;
		}
		return(0);
	}
}

/* ds_fpdiv12 can treat only in the case where the entire value
	 (integer part + fractional part) of the dividend, divisor,
	 and divided all can be expressed in U(20,12) fixed point fractional number format,
	 i.e., max. 20 bits of integer part (for fractional parts, we don't need to worry about
	 because, whatever the fractional value is, it will be expressed in fixed 12 bits.).

	 This function covers the rest cases that ds_fpdiv12 can not treat by repeated
	 subtraction instead of the direct division using ds_fpidv12.
	 For the case that ds_fpidv12 can treat, this function uses ds_fpidv12.

	 Note that dividend_fra_fp12 and divisor_fra_fp12 should not include integer part.

	 dividend_int_ulong is the integer part of dividend.
	 dividend_fra_fp12 is the fraction part of dividend.
	 divisor_int_ulong is the integer part of divisor.
	 divisor_fra_fp12 is the fraction part of divisor.
	 divided_int_ulong is the integer part of divided.
	 divided_fra_fp12 is the fraction part of divided.
 */
int ds_fpdiv(unsigned long dividend_int_ulong, unsigned long dividend_fra_fp12,
		 unsigned long divisor_int_ulong, unsigned long divisor_fra_fp12,
		 unsigned long *divided_int_ulong, unsigned long *divided_fra_fp12)
{
	int dividend_int_digits = 0;
	int divisor_int_digits = 0;
	int divisor_fra_digits = 0;
	int i = 0;

	unsigned long operand_fp12 = 0;
	unsigned long operand_int_ulong = 0;
	unsigned long operand_fra_fp12 = 0;

	unsigned long operand_int_ulong_shifted = 0;
	unsigned long operand_fra_fp12_shifted = 0;

	int operand_carry = 0;
	int operand_carry_shifted = 0;

	unsigned long operator_fp12 = 0;
	unsigned long operator_int_ulong = 0;
	unsigned long operator_fra_fp12 = 0;
	unsigned long operator_int_ulong_shifted = 0;
	unsigned long operator_fra_fp12_shifted = 0;

	int operator_carry = 0;
	int operator_carry_shifted = 0;

	unsigned long operated_fp12 = 0;
	unsigned long operated_int_ulong = 0;
	unsigned long operated_fra_fp12 = 0;

	unsigned long tmp_ulong = 0;

	int accumulated_operand_shifted_digits = 0;

	*divided_int_ulong = 0;
	*divided_fra_fp12 = 0;

	if(divisor_int_ulong == 0 && divisor_fra_fp12 == 0){
		printk(KERN_INFO "[ds_fpdiv] Error1: Divided by 0 \
				(divisor_int_ulong == 0 divisor_fra_fp12 == 0).\n");
		return(-1);
	}

	if(dividend_int_ulong == 0 && dividend_fra_fp12 == 0){
		*divided_int_ulong = 0;
		*divided_fra_fp12 = 0;
		return(0);
	}

	if(dividend_int_ulong == divisor_int_ulong && dividend_fra_fp12 == divisor_fra_fp12){
		*divided_int_ulong = 1;
		*divided_fra_fp12 = 0;
		return(0);
	}

	/* (1) Check if the integer part of the multiplied value will not be able to expressed
				 by a 32-bit unsigned long variable.
	 */
	dividend_int_digits = ds_find_first1_in_integer_part(dividend_int_ulong);
	divisor_int_digits = ds_find_first1_in_integer_part(divisor_int_ulong);
	divisor_fra_digits = ds_find_first1_in_fraction_part(divisor_fra_fp12);
	if(divisor_int_ulong == 0 && dividend_int_digits + divisor_fra_digits > 32){
		printk(KERN_INFO "[ds_fpdiv] Error2: divisor_int_ulong == 0 && \
				 dividend_int_digits %d + divisor_fra_digits %d > 32.\n",
				 dividend_int_digits, divisor_fra_digits);
		return(-1);
	}
	else{

		/* (2) If the integer + fraction of the dividend, divisor, and divided
					 all can be represented in U(20,12), directly use ds_fpdiv12.
		 */
		if(dividend_int_digits <= 20 &&
			 divisor_int_digits <= 20 &&
			 dividend_int_digits + divisor_fra_digits <= 20)
		{
			operand_fp12 = DS_ULONG2FP12(dividend_int_ulong) | dividend_fra_fp12;
			operator_fp12 = DS_ULONG2FP12(divisor_int_ulong) | divisor_fra_fp12;
			ds_fpdiv12(operand_fp12, operator_fp12, &operated_fp12, &tmp_ulong);
			*divided_int_ulong = DS_GETFP12INT(operated_fp12);
			*divided_fra_fp12 = DS_GETFP12FRA(operated_fp12);
		}
		/* (3) If the integer + fraction of the dividend, divisor, and divided
			 can not all be represented in U(20,12), use subsequent subtractions.

			 Now possible cases are

			 1. The integer + fraction of both the dividend and divisor
				can be represented in U(20,12),
				but the integer + fraction of divided can not be represented in U(20,12).

			 2. The integer + fraction of the dividend can not be represented in U(20,12)
				but that of the divisor can be.
				In this case, the integer + fraction of the divided may or may not be
				represented in U(20,12).

			 3. The integer + fraction of the dividend can be represented in U(20,12)
				but that of the divisor can not be.
				In this case, the integer + fraction of the divided always can be
				represented in U(20,12).

			 4. The integer + fraction of neigther the dividend nor divisor can be
				represented in U(20,12).
				In this case, the integer + fraction of the divided always can be
				represented in U(20,12).
		 */
		else{
			operand_carry = 0;
			operand_int_ulong = dividend_int_ulong;
			operand_fra_fp12 = dividend_fra_fp12;

			operator_carry = 0;
			operator_int_ulong = divisor_int_ulong;
			operator_fra_fp12 = divisor_fra_fp12;

			/* Calculate the integer part of the operated
				 as long as the operand is larger than the operator.
			 */
			while(ds_compare45bits(operand_carry,
						 operand_int_ulong,
						 operand_fra_fp12,
						 operator_carry,
						 operator_int_ulong,
						 operator_fra_fp12) == DS_LARGER)
			{
				/* Find the maximum number of digits
					 the operator can be shifted without exceeding the operand */
				i = 0;
				do{
					i ++;
					if(ds_shiftleft44bits(operator_carry,
								operator_int_ulong,
								operator_fra_fp12,
								i,
								&operator_carry_shifted,
								&operator_int_ulong_shifted,
								&operator_fra_fp12_shifted) < 0)
						return(-1);
				}while(ds_compare45bits(operator_carry_shifted,
								operator_int_ulong_shifted,
								operator_fra_fp12_shifted,
								operand_carry,
								operand_int_ulong,
								operand_fra_fp12) != DS_LARGER);
				/* The found number of digit to shift is i-1 */

				/* The found digit number is a fraction of the quotient */
				operated_int_ulong |= 1<<(i-1);

				/* Shift the operator left by the found digit number */
				if(ds_shiftleft44bits(operator_carry,
							operator_int_ulong,
							operator_fra_fp12, i-1,
							&operator_carry_shifted,
							&operator_int_ulong_shifted,
							&operator_fra_fp12_shifted) < 0) return(-1);

				/* Then, subtract the shifted operator
					 from the operand to get the remained value of the operand */
				ds_subtract44bits(operand_carry,
							operand_int_ulong,
							operand_fra_fp12,
							operator_carry_shifted,
							operator_int_ulong_shifted,
							operator_fra_fp12_shifted,
							&operand_carry,
							&operand_int_ulong,
							&operand_fra_fp12);

				/* If no remained value exists in the operand, it's the end of division */
				if(operand_carry == 0 && operand_int_ulong == 0 && operand_fra_fp12 == 0){
					*divided_int_ulong = operated_int_ulong;
					*divided_fra_fp12 = 0;
					return(0);
				}
			}

			/* Calculate the fractional part of the operated after the operand becomes
				 smaller than the operator.
				 This calculation needs to be repeated just until the LSB of the 12-bits
				 fraction part of result is determined.
				 And, obviously, the LSB is determined when the accumulated shifted digit
				 number of the operand reaches 12.
			 */
			accumulated_operand_shifted_digits = 0;
			while(accumulated_operand_shifted_digits < 12){
				/* Find the minimum number of digits the operand should be shifted to be
					 same or larger than the operator */
				i = 0;
				do{
					i ++;
					if(ds_shiftleft44bits(operand_carry,
								operand_int_ulong,
								operand_fra_fp12, i,
								&operand_carry_shifted,
								&operand_int_ulong_shifted,
								&operand_fra_fp12_shifted) < 0) return(-1);
				}while(ds_compare45bits(operand_carry_shifted,
								operand_int_ulong_shifted,
								operand_fra_fp12_shifted,
								operator_carry,
								operator_int_ulong,
								operator_fra_fp12) == DS_SMALLER);
				/* The found number of digit to shift is i */

				/* Update accumulated_operand_shifted_digits */
				accumulated_operand_shifted_digits += i;

				/* After the finding, if accumulated shifted digit number of the operand
					 exceeds 12, stop calculation */
				if(accumulated_operand_shifted_digits > 12){
					*divided_int_ulong = operated_int_ulong;
					*divided_fra_fp12 = operated_fra_fp12;
					return(0);
				}

				/* The found digit number is a fraction of the fractional part of the result */
				operated_fra_fp12 |= 1<<(12-accumulated_operand_shifted_digits);

				/* Subtract the operator from the shifted operand to get the remained value
					 of the shifted operand */
				ds_subtract44bits(operand_carry_shifted,
						operand_int_ulong_shifted,
						operand_fra_fp12_shifted,
						operator_carry,
						operator_int_ulong,
						operator_fra_fp12,
						&operand_carry,
						&operand_int_ulong,
						&operand_fra_fp12);

				/* If no remained value exists in the operand, it's the end of division */
				if(operand_carry == 0 && operand_int_ulong == 0 && operand_fra_fp12 == 0){
					*divided_int_ulong = operated_int_ulong;
					*divided_fra_fp12 = operated_fra_fp12;
					return(0);
				}
			}

			*divided_int_ulong = operated_int_ulong;
			*divided_fra_fp12 = operated_fra_fp12;
		}
		return(0);
	}
}

/*====================================================================
	The function which finds and returns the next high CPU_OP index.
	====================================================================*/

unsigned int ds_get_next_high_cpu_op_index(
unsigned long perf_requirement_int_ulong,
unsigned long perf_requirement_fra_fp12)
{

	/* Ensure to take only the last 12 bits */
	perf_requirement_fra_fp12 = perf_requirement_fra_fp12 & 0xfff;

	/* If perf_requirement_int_ulong > 0,
		 which means that the required performance is greater than 1,
		 apply the maximum CPU_OP. */
	if(perf_requirement_int_ulong > 0){
		return(per_cpu(ds_sys_status, 0).locked_max_cpu_op_index);
	}
	else{
		switch(per_cpu(ds_sys_status, 0).locked_max_cpu_op_index){
#ifdef CONFIG_P970_OPPS_ENABLED
			case 1350000000: 	goto OPP_13;
			case 1300000000: 	goto OPP_12;
			case 1200000000: 	goto OPP_11;
			case 1100000000: 	goto OPP_10;
#endif
			case 1000000000: 	goto OPP_9;
			case 900000000: 	goto OPP_8;
			case 800000000: 	goto OPP_7;
			case 700000000: 	goto OPP_6;
			case 600000000: 	goto OPP_5;
			case 500000000: 	goto OPP_4;
			case 400000000: 	goto OPP_3;
			case 300000000: 	goto OPP_2;
			default:		goto OPP_9;
		}
	}

#ifdef CONFIG_P970_OPPS_ENABLED
/* New approach... until 600MHz all functions behave as 1000MHz config and
 * after that linealy until the specific max_freq */
OPP_13:
	if (perf_requirement_fra_fp12 > 3987) return(DS_CPU_OP_INDEX_0);
	else if (perf_requirement_fra_fp12 > 3769) return(DS_CPU_OP_INDEX_1);
	else if (perf_requirement_fra_fp12 > 3550) return(DS_CPU_OP_INDEX_2);
	else if (perf_requirement_fra_fp12 > 3332) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 3113) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 2895) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2677) return(DS_CPU_OP_INDEX_6);
	else goto OPP_COM;

OPP_12:
	if (perf_requirement_fra_fp12 > 3862) return(DS_CPU_OP_INDEX_1);
	else if (perf_requirement_fra_fp12 > 3628) return(DS_CPU_OP_INDEX_2);
	else if (perf_requirement_fra_fp12 > 3394) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 3160) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 2926) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2692) return(DS_CPU_OP_INDEX_6);
	else goto OPP_COM;

OPP_11:
	if (perf_requirement_fra_fp12 > 3823) return(DS_CPU_OP_INDEX_2);
	else if (perf_requirement_fra_fp12 > 3550) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 3277) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 3004) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2731) return(DS_CPU_OP_INDEX_6);
	else goto OPP_COM;

OPP_10:
	if (perf_requirement_fra_fp12 > 3769) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 3441) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 3113) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2786) return(DS_CPU_OP_INDEX_6);
	else goto OPP_COM;

#if 0
/* Old approach... all functions are lineal */
OPP_13:
	if (perf_requirement_fra_fp12 > 3945) return(DS_CPU_OP_INDEX_0);
	else if (perf_requirement_fra_fp12 > 3641) return(DS_CPU_OP_INDEX_1);
	else if (perf_requirement_fra_fp12 > 3338) return(DS_CPU_OP_INDEX_2);
	else if (perf_requirement_fra_fp12 > 3035) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 2731) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 2428) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2124) return(DS_CPU_OP_INDEX_6);
	else if (perf_requirement_fra_fp12 > 1821) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 1518) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 1214) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 911) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 607) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 304) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_12:
	if (perf_requirement_fra_fp12 > 3781) return(DS_CPU_OP_INDEX_1);
	else if (perf_requirement_fra_fp12 > 3466) return(DS_CPU_OP_INDEX_2);
	else if (perf_requirement_fra_fp12 > 3151) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 2836) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 2521) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2206) return(DS_CPU_OP_INDEX_6);
	else if (perf_requirement_fra_fp12 > 1891) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 1576) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 1261) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 946) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 631) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 316) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_11:
	if (perf_requirement_fra_fp12 > 3755) return(DS_CPU_OP_INDEX_2);
	else if (perf_requirement_fra_fp12 > 3414) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 3072) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 2731) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2390) return(DS_CPU_OP_INDEX_6);
	else if (perf_requirement_fra_fp12 > 2048) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 1707) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 1366) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 1024) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 683) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 342) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_10:
	if (perf_requirement_fra_fp12 > 3724) return(DS_CPU_OP_INDEX_3);
	else if (perf_requirement_fra_fp12 > 3352) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 2979) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2607) return(DS_CPU_OP_INDEX_6);
	else if (perf_requirement_fra_fp12 > 2235) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 1862) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 1490) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 1118) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 745) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 373) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);
#endif
#endif

OPP_9:
	if (perf_requirement_fra_fp12 > 3687) return(DS_CPU_OP_INDEX_4);
	else if (perf_requirement_fra_fp12 > 3277) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 2868) return(DS_CPU_OP_INDEX_6);
	else goto OPP_COM;

OPP_COM:
	if (perf_requirement_fra_fp12 > 2458) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 2048) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 1639) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 1229) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 820) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 410) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_8:
	if (perf_requirement_fra_fp12 > 3641) return(DS_CPU_OP_INDEX_5);
	else if (perf_requirement_fra_fp12 > 3186) return(DS_CPU_OP_INDEX_6);
	else if (perf_requirement_fra_fp12 > 2731) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 2276) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 1821) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 1366) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 911) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 456) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_7:
	if (perf_requirement_fra_fp12 > 3584) return(DS_CPU_OP_INDEX_6);
	else if (perf_requirement_fra_fp12 > 3072) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 2560) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 2048) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 1536) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 1024) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 512) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_6:
	if (perf_requirement_fra_fp12 > 3511) return(DS_CPU_OP_INDEX_7);
	else if (perf_requirement_fra_fp12 > 2926) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 2341) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 1756) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 1171) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 586) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_5:
	if (perf_requirement_fra_fp12 > 3414) return(DS_CPU_OP_INDEX_8);
	else if (perf_requirement_fra_fp12 > 2731) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 2048) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 1366) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 683) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_4:
	if (perf_requirement_fra_fp12 > 3277) return(DS_CPU_OP_INDEX_9);
	else if (perf_requirement_fra_fp12 > 2458) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 1639) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 820) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_3:
	if (perf_requirement_fra_fp12 > 3072) return(DS_CPU_OP_INDEX_10);
	else if (perf_requirement_fra_fp12 > 2048) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 1024) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

OPP_2:
	if (perf_requirement_fra_fp12 > 2731) return(DS_CPU_OP_INDEX_11);
	else if (perf_requirement_fra_fp12 > 1366) return(DS_CPU_OP_INDEX_12);
	else return(DS_CPU_OP_INDEX_13);

}

/*====================================================================
	The function which updates the fractions of
	elapsed and busy time both in wall time and full speed equivalent time.
	====================================================================*/

int ds_update_time_counter(int ds_cpu){

	cputime64_t lc_usec_wall_cur = 0;
	cputime64_t lc_usec_idle_cur = 0;
	cputime64_t lc_usec_iowait_cur = 0;

	unsigned long lc_usec_interval = 0;
	unsigned long lc_usec_idle = 0;
	unsigned long lc_usec_iowait = 0;
	unsigned long lc_usec_busy = 0;

	if(per_cpu(ds_counter, ds_cpu).counter_mutex != 0) return(0);
	per_cpu(ds_counter, ds_cpu).counter_mutex ++;

	lc_usec_wall_cur = jiffies64_to_cputime64(get_jiffies_64());
	lc_usec_idle_cur = get_cpu_idle_time_us(ds_cpu, &lc_usec_wall_cur);
	lc_usec_iowait_cur = get_cpu_iowait_time_us(ds_cpu, &lc_usec_wall_cur);

	if(per_cpu(ds_counter, ds_cpu).flag_counter_initialized == 0){
		per_cpu(ds_counter, ds_cpu).flag_counter_initialized = 1;
	}
	else{
		lc_usec_interval = 
			(unsigned long)(lc_usec_wall_cur - per_cpu(ds_counter, ds_cpu).wall_usec_base);
		lc_usec_idle = 
			(unsigned long)(lc_usec_idle_cur - per_cpu(ds_counter, ds_cpu).idle_usec_base);
		lc_usec_iowait = 
			(unsigned long)(lc_usec_iowait_cur - per_cpu(ds_counter, ds_cpu).iowait_usec_base);

#if 0
		if(lc_usec_idle > lc_usec_iowait)
			lc_usec_idle -= lc_usec_iowait;
		else
			lc_usec_idle = 0;
#endif

		if(lc_usec_interval > lc_usec_idle)
			lc_usec_busy = lc_usec_interval - lc_usec_idle;
		else
			lc_usec_busy = 0;

		/* Elapsed */
		per_cpu(ds_counter, ds_cpu).elapsed_sec += (lc_usec_interval / 1000000);
		per_cpu(ds_counter, ds_cpu).elapsed_usec += (lc_usec_interval % 1000000);
		if(per_cpu(ds_counter, ds_cpu).elapsed_usec >= 1000000){
			per_cpu(ds_counter, ds_cpu).elapsed_sec += 1;
			per_cpu(ds_counter, ds_cpu).elapsed_usec -= 1000000;
		}

		/* Busy */
		per_cpu(ds_counter, ds_cpu).busy_sec += (lc_usec_busy / 1000000);
		per_cpu(ds_counter, ds_cpu).busy_usec += (lc_usec_busy % 1000000);
		if(per_cpu(ds_counter, ds_cpu).busy_usec >= 1000000){
			per_cpu(ds_counter, ds_cpu).busy_sec += 1;
			per_cpu(ds_counter, ds_cpu).busy_usec -= 1000000;
		}
	}

	per_cpu(ds_counter, ds_cpu).wall_usec_base = lc_usec_wall_cur;
	per_cpu(ds_counter, ds_cpu).idle_usec_base = lc_usec_idle_cur;
	per_cpu(ds_counter, ds_cpu).iowait_usec_base = lc_usec_iowait_cur;

	per_cpu(ds_counter, ds_cpu).counter_mutex --;

	return(0);
}

/*====================================================================
	The functions to perform DVS scheme:
	AIDVS.
	====================================================================*/

int ds_do_dvs_aidvs(int ds_cpu, unsigned int *target_cpu_op_index, DS_AIDVS_STAT_STRUCT *stat)
{
	unsigned long lc_time_usec_interval_inc = 0;
	unsigned long lc_time_usec_work_inc = 0;

	unsigned long lc_old_moving_avg_int_ulong = 0;
	unsigned long lc_old_moving_avg_fra_fp12 = 0x0;
	unsigned long lc_old_utilization_int_ulong = 0;
	unsigned long lc_old_utilization_fra_fp12 = 0x0;
	unsigned long lc_old_moving_avg_int_ulong_by_weight = 0;
	unsigned long lc_old_moving_avg_fra_fp12_by_weight = 0x0;
	unsigned long lc_numerator_int_ulong = 0;
	unsigned long lc_numerator_fra_fp12 = 0x0;

	/* Calc interval */
	if(per_cpu(ds_counter, ds_cpu).elapsed_usec >= stat->time_usec_interval_inc_base){
		lc_time_usec_interval_inc =
			per_cpu(ds_counter, ds_cpu).elapsed_usec - stat->time_usec_interval_inc_base;
	}
	else{
		lc_time_usec_interval_inc =
			per_cpu(ds_counter, ds_cpu).elapsed_usec + (1000000 - stat->time_usec_interval_inc_base);
	}
	stat->time_usec_interval += lc_time_usec_interval_inc;
	stat->time_sec_interval_inc_base = per_cpu(ds_counter, ds_cpu).elapsed_sec;
	stat->time_usec_interval_inc_base = per_cpu(ds_counter, ds_cpu).elapsed_usec;

	/* Calc work */
	if(per_cpu(ds_counter, ds_cpu).busy_usec >= stat->time_usec_work_inc_base){
		lc_time_usec_work_inc =
			per_cpu(ds_counter, ds_cpu).busy_usec - stat->time_usec_work_inc_base;
	}
	else{
		lc_time_usec_work_inc =
			per_cpu(ds_counter, ds_cpu).busy_usec + (1000000 - stat->time_usec_work_inc_base);
	}
	stat->time_usec_work += lc_time_usec_work_inc;
	stat->time_sec_work_inc_base = per_cpu(ds_counter, ds_cpu).busy_sec;
	stat->time_usec_work_inc_base = per_cpu(ds_counter, ds_cpu).busy_usec;

	/* Determine interval_length to use */
	if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0){
		stat->interval_length = ds_control.aidvs_interval_length;
	}
	else{
		stat->interval_length = DS_AIDVS_PE_INTERVAL_LENGTH;
	}

	/* Calc cpu_op if we reached interval_window_legnth */
	if(stat->time_usec_interval >= stat->interval_length)
	{
		/* SAVE OLD */
		lc_old_utilization_int_ulong = stat->utilization_int_ulong;
		lc_old_utilization_fra_fp12 = stat->utilization_fra_fp12;
		lc_old_moving_avg_int_ulong = stat->moving_avg_int_ulong;
		lc_old_moving_avg_fra_fp12 = stat->moving_avg_fra_fp12;

		/* Calculate NEW, i.e., the current interval window's utilization */
		/* In wall clock time */
		if(stat->time_usec_interval == 0){
			stat->utilization_int_ulong = 1;
			stat->utilization_fra_fp12 = 0x0;
		}
		else{
			if(ds_fpdiv(stat->time_usec_work,
					0x0,
					stat->time_usec_interval,
					0x0,
					&(stat->utilization_int_ulong),
					&(stat->utilization_fra_fp12)) < 0)
			{
				printk(KERN_INFO "[ds_do_dvs_aidvs 1] Error: ds_fpdiv failed. \
						dvs_suite gonna be off.\n");
				ds_control.on_dvs = 0;
				ds_control.flag_run_dvs = 0;
				return(-1);
			}
		}

		/* Determine WEIGHT */
		if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0){
			stat->moving_avg_weight = ds_control.aidvs_moving_avg_weight;
		}
		else
			stat->moving_avg_weight = DS_AIDVS_PE_MOVING_AVG_WEIGHT;

		/* Calculate OLD x WEIGHT */
		if(ds_fpmul(lc_old_moving_avg_int_ulong, 
					lc_old_moving_avg_fra_fp12, 
					stat->moving_avg_weight,
					0x0,
					&lc_old_moving_avg_int_ulong_by_weight, 
					&lc_old_moving_avg_fra_fp12_by_weight) < 0)
		{
			printk(KERN_INFO "[ds_do_dvs_aidvs 4] Error: ds_fpmul failed. \
					dvs_suite gonna be off.\n");
			ds_control.on_dvs = 0;
			ds_control.flag_run_dvs = 0;
			return(-1);
		}

		/* Calculate OLD x WEIGHT + NEW, i.e., the numerator of moving average */
		lc_numerator_int_ulong = 
			lc_old_moving_avg_int_ulong_by_weight + stat->utilization_int_ulong;
		lc_numerator_fra_fp12 = 
			lc_old_moving_avg_fra_fp12_by_weight + stat->utilization_fra_fp12;
		if(lc_numerator_fra_fp12 >= 0x1000){
			lc_numerator_int_ulong += 1;
			lc_numerator_fra_fp12 -= 0x1000;
		}

		/* Calculate (OLD x WEIGHT + NEW) / (WEIGHT + 1), i.e., the moving average */
		if(ds_fpdiv(lc_numerator_int_ulong, 
					lc_numerator_fra_fp12, 
					stat->moving_avg_weight+1, 
					0x0,
					&stat->moving_avg_int_ulong, 
					&stat->moving_avg_fra_fp12) < 0)
		{
			printk(KERN_INFO "[ds_do_dvs_aidvs 5] Error: ds_fpdiv failed. \
					dvs_suite gonna be off.\n");
			ds_control.on_dvs = 0;
			ds_control.flag_run_dvs = 0;
			return(-1);
		}

		/* Find the CPU_OP_INDEX corresponding to the calculated utilization */
		stat->cpu_op_index = 
			ds_get_next_high_cpu_op_index(stat->moving_avg_int_ulong, stat->moving_avg_fra_fp12);
#if 0
		/* TODO: Tune for OMAP */
		if(ds_cpu){
			switch(stat->cpu_op_index){
				case DS_CPU_OP_INDEX_0:		stat->cpu_op_index = DS_CPU_OP_INDEX_0; break;
				case DS_CPU_OP_INDEX_1:		stat->cpu_op_index = DS_CPU_OP_INDEX_1; break;
				case DS_CPU_OP_INDEX_2:		stat->cpu_op_index = DS_CPU_OP_INDEX_2; break;
				case DS_CPU_OP_INDEX_3:		stat->cpu_op_index = DS_CPU_OP_INDEX_3; break;
				case DS_CPU_OP_INDEX_4:		stat->cpu_op_index = DS_CPU_OP_INDEX_4; break;
				case DS_CPU_OP_INDEX_5:		stat->cpu_op_index = DS_CPU_OP_INDEX_5; break;
				case DS_CPU_OP_INDEX_6:		stat->cpu_op_index = DS_CPU_OP_INDEX_6; break;
				case DS_CPU_OP_INDEX_7:		stat->cpu_op_index = DS_CPU_OP_INDEX_7; break;
				case DS_CPU_OP_INDEX_8:		stat->cpu_op_index = DS_CPU_OP_INDEX_8; break;
				case DS_CPU_OP_INDEX_9:		stat->cpu_op_index = DS_CPU_OP_INDEX_9; break;
#ifdef CONFIG_P970_OPPS_ENABLED
				case DS_CPU_OP_INDEX_10:	stat->cpu_op_index = DS_CPU_OP_INDEX_10; break;
				case DS_CPU_OP_INDEX_11:	stat->cpu_op_index = DS_CPU_OP_INDEX_11; break;
				case DS_CPU_OP_INDEX_12:	stat->cpu_op_index = DS_CPU_OP_INDEX_12; break;
				case DS_CPU_OP_INDEX_13:	stat->cpu_op_index = DS_CPU_OP_INDEX_13; break;
#endif
				default:			stat->cpu_op_index = DS_CPU_OP_INDEX_0; break;
			}
		}
#endif

		stat->time_usec_interval = 0;
		stat->time_usec_work = 0;
	}

	/* (4) Determine *target_cpu_op_index.
	 */
	*target_cpu_op_index = stat->cpu_op_index;

	return(0);
}

/*====================================================================
	The functions to perform DVS scheme:
	GPScheDVS.
	====================================================================*/
int ds_do_dvs_gpschedvs(int ds_cpu, unsigned int *target_cpu_op_index){

	unsigned int lc_target_cpu_op_index_lower = min(per_cpu(ds_sys_status, 0).locked_min_cpu_op_index,
						        per_cpu(ds_sys_status, 0).locked_max_cpu_op_index);
	unsigned int lc_target_cpu_op_index_highest = lc_target_cpu_op_index_lower;
	unsigned int lc_target_cpu_op_index_aidvs = lc_target_cpu_op_index_lower;
//	unsigned int lc_target_cpu_op_index_touch = lc_target_cpu_op_index_lower;

	/* Calc target_cpu_op based on workload */
	ds_do_dvs_aidvs(ds_cpu, &lc_target_cpu_op_index_aidvs, &(per_cpu(ds_aidvs_status, ds_cpu)));

	lc_target_cpu_op_index_highest = lc_target_cpu_op_index_aidvs;

	/* LCD is on (i.e., before early suspend) */
	if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0){

#if 0
		/* Huexxx: Since all TOUCH OPPs are min_freq, there's no reason to mantain this stuff
		 * because touch_freq never will be higher than target freq */
		/* Touch event occurred and being processed - Begin */
		if(per_cpu(ds_sys_status, 0).flag_touch_timeout_count != 0){

			/* If DS_TOUCH_CPU_OP_UP_INTERVAL is over */
			if((per_cpu(ds_counter, ds_cpu).elapsed_sec * 1000000 + 
				per_cpu(ds_counter, ds_cpu).elapsed_usec) >= 
				(per_cpu(ds_sys_status, 0).touch_timeout_sec * 1000000 +
				per_cpu(ds_sys_status, 0).touch_timeout_usec))
			{
				if(per_cpu(ds_counter, ds_cpu).elapsed_usec + DS_TOUCH_CPU_OP_UP_INTERVAL < 1000000){
					per_cpu(ds_sys_status, 0).touch_timeout_sec = 
						per_cpu(ds_counter, ds_cpu).elapsed_sec;
					per_cpu(ds_sys_status, 0).touch_timeout_usec = 
						per_cpu(ds_counter, ds_cpu).elapsed_usec + DS_TOUCH_CPU_OP_UP_INTERVAL;
				}
				else{
					per_cpu(ds_sys_status, 0).touch_timeout_sec = 
						per_cpu(ds_counter, ds_cpu).elapsed_sec + 1;
					per_cpu(ds_sys_status, 0).touch_timeout_usec = 
						(per_cpu(ds_counter, ds_cpu).elapsed_usec + DS_TOUCH_CPU_OP_UP_INTERVAL) - 1000000;
				}

				switch(per_cpu(ds_sys_status, 0).flag_touch_timeout_count){
					case DS_TOUCH_CPU_OP_UP_CNT_MAX:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 39;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH39;
						break;
					case 39:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 38;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH38;
						break;
					case 38:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 37;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH37;
						break;
					case 37:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 36;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH36;
						break;
					case 36:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 35;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH35;
						break;
					case 35:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 34;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH34;
						break;
					case 34:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 33;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH33;
						break;
					case 33:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 32;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH32;
						break;
					case 32:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 31;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH31;
						break;
					case 31:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 30;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH30;
						break;
					case 30:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 29;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH29;
						break;
					case 29:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 28;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH28;
						break;
					case 28:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 27;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH27;
						break;
					case 27:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 26;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH26;
						break;
					case 26:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 25;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH25;
						break;
					case 25:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 24;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH24;
						break;
					case 24:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 23;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH23;
						break;
					case 23:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 22;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH22;
						break;
					case 22:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 21;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH21;
						break;
					case 21:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 20;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH20;
						break;
					case 20:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 19;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH19;
						break;
					case 19:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 18;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH18;
						break;
					case 18:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 17;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH17;
						break;
					case 17:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 16;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH16;
						break;
					case 16:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 15;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH15;
						break;
					case 15:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 14;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH14;
						break;
					case 14:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 13;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH13;
						break;
					case 13:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 12;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH12;
						break;
					case 12:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 11;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH11;
						break;
					case 11:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 10;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH10;
						break;
					case 10:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 9;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH9;
						break;
					case 9:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 8;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH8;
						break;
					case 8:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 7;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH7;
						break;
					case 7:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 6;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH6;
						break;
					case 6:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 5;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH5;
						break;
					case 5:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 4;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH4;
						break;
					case 4:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 3;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH3;
						break;
					case 3:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 2;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH2;
						break;
					case 2:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 1;
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH1;
						break;
					case 1:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 0;
						lc_target_cpu_op_index_touch = lc_target_cpu_op_index_lower;
						per_cpu(ds_sys_status, 0).touch_timeout_sec = 0;
						per_cpu(ds_sys_status, 0).touch_timeout_usec = 0;
						break;
					default:
						per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 0;
						lc_target_cpu_op_index_touch = lc_target_cpu_op_index_lower;
						per_cpu(ds_sys_status, 0).touch_timeout_sec = 0;
						per_cpu(ds_sys_status, 0).touch_timeout_usec = 0;
						break;
				}
			}
			/* If DS_TOUCH_CPU_OP_UP_INTERVAL is not over yet */
			else
			{
				switch(per_cpu(ds_sys_status, 0).flag_touch_timeout_count){
					case DS_TOUCH_CPU_OP_UP_CNT_MAX:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH40;
						break;
					case 39:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH39;
						break;
					case 38:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH38;
						break;
					case 37:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH37;
						break;
					case 36:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH36;
						break;
					case 35:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH35;
						break;
					case 34:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH34;
						break;
					case 33:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH33;
						break;
					case 32:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH32;
						break;
					case 31:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH31;
						break;
					case 30:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH30;
						break;
					case 29:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH29;
						break;
					case 28:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH28;
						break;
					case 27:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH27;
						break;
					case 26:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH26;
						break;
					case 25:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH25;
						break;
					case 24:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH24;
						break;
					case 23:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH23;
						break;
					case 22:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH22;
						break;
					case 21:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH21;
						break;
					case 20:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH20;
						break;
					case 19:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH19;
						break;
					case 18:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH18;
						break;
					case 17:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH17;
						break;
					case 16:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH16;
						break;
					case 15:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH15;
						break;
					case 14:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH14;
						break;
					case 13:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH13;
						break;
					case 12:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH12;
						break;
					case 11:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH11;
						break;
					case 10:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH10;
						break;
					case 9:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH9;
						break;
					case 8:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH8;
						break;
					case 7:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH7;
						break;
					case 6:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH6;
						break;
					case 5:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH5;
						break;
					case 4:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH4;
						break;
					case 3:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH3;
						break;
					case 2:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH2;
						break;
					case 1:
						lc_target_cpu_op_index_touch = DS_CPU_OP_INDEX_TOUCH1;
						break;
					default:
						lc_target_cpu_op_index_touch = lc_target_cpu_op_index_lower;
						break;
				}
			}

			/* Ramping up cpu op upon touch events */
			if(lc_target_cpu_op_index_highest < lc_target_cpu_op_index_touch)
				lc_target_cpu_op_index_highest = lc_target_cpu_op_index_touch;
		}
		/* Touch event occurred and being processed - End */
#endif
		/* Special treatment upon frequency ceiling or flooring - Begin */
		/* Ceiled by long consecutive touches */
		if(per_cpu(ds_sys_status, 0).flag_long_consecutive_touches == 1){
			if(lc_target_cpu_op_index_highest > DS_CPU_OP_INDEX_CONT_TOUCH)
				lc_target_cpu_op_index_highest = DS_CPU_OP_INDEX_CONT_TOUCH;
		}
#if 0
		/* Huexxx: already ceiled within ds_get_next_high_cpu_op_index() function */
		/* Ceiled by cpufreq sysfs */
		if(per_cpu(ds_sys_status, 0).locked_max_cpu_op_index < DS_CPU_OP_INDEX_MAX){
			if(lc_target_cpu_op_index_highest > per_cpu(ds_sys_status, 0).locked_max_cpu_op_index)
				lc_target_cpu_op_index_highest = per_cpu(ds_sys_status, 0).locked_max_cpu_op_index;
		}
#endif

		/* Floored by cpufreq sysfs */
		if(lc_target_cpu_op_index_lower > lc_target_cpu_op_index_highest)
			lc_target_cpu_op_index_highest = lc_target_cpu_op_index_lower;
#if 0
		if(lc_target_cpu_op_index_lower > DS_CPU_OP_INDEX_MIN){	
			per_cpu(ds_sys_status, 0).locked_min_cpu_op_release_sec = 
				per_cpu(ds_counter, ds_cpu).elapsed_sec + DS_CPU_OP_LOCK_SUSTAIN_SEC;
			per_cpu(ds_sys_status, 0).flag_locked_min_cpu_op = 1;
			if(lc_target_cpu_op_index_lower > DS_CPU_OP_INDEX_LOCKED_MIN){
				if(lc_target_cpu_op_index_highest < DS_CPU_OP_INDEX_LOCKED_MIN)
					lc_target_cpu_op_index_highest = DS_CPU_OP_INDEX_LOCKED_MIN;
			}
			else{
				if(lc_target_cpu_op_index_highest < lc_target_cpu_op_index_lower)
					lc_target_cpu_op_index_highest = lc_target_cpu_op_index_lower;
			}
		}
		/* Frequency had been floored. But not now */
		else{
			if(per_cpu(ds_sys_status, 0).locked_min_cpu_op_release_sec >
				per_cpu(ds_counter, ds_cpu).elapsed_sec)
			{
				per_cpu(ds_sys_status, 0).flag_locked_min_cpu_op = 1;
				if(lc_target_cpu_op_index_highest < DS_CPU_OP_INDEX_LOCKED_MIN)
					lc_target_cpu_op_index_highest = DS_CPU_OP_INDEX_LOCKED_MIN;
			}
			else{
				per_cpu(ds_sys_status, 0).locked_min_cpu_op_release_sec = 0;
				per_cpu(ds_sys_status, 0).flag_locked_min_cpu_op = 0;
			}
		}
		/* Special care for the frequency locking through cpufreq sysfs - End */
#endif
	}
	/* LCD is off (i.e., after early suspend) */
	else{
		/* If target op is lower than max ... */
		if (lc_target_cpu_op_index_highest < per_cpu(ds_sys_status, 0).locked_max_cpu_op_index)
			/* ... and higher that 300MHz ... */
			if (lc_target_cpu_op_index_highest > DS_CPU_OP_INDEX_11)
				/* ... limit it to 300MHz */
				lc_target_cpu_op_index_highest = DS_CPU_OP_INDEX_11;
		/* Limit maximum frequency with screen of to 800MHz */
		if (lc_target_cpu_op_index_highest > DS_CPU_OP_INDEX_6)
			lc_target_cpu_op_index_highest = DS_CPU_OP_INDEX_6;
	}

	*target_cpu_op_index = lc_target_cpu_op_index_highest;

	return(0);
}

/*====================================================================
	The functions to initialize dvs_suite_system_status structure.
	====================================================================*/

int ds_initialize_ds_control(void){

	ds_control.aidvs_moving_avg_weight = DS_AIDVS_NM_MOVING_AVG_WEIGHT;
	ds_control.aidvs_interval_length = DS_AIDVS_NM_INTERVAL_LENGTH;

	return(0);
}

/*====================================================================
	The functions to initialize dvs_suite_sysfs_status structure.
	====================================================================*/

int ds_initialize_ds_sysfs_status(void){

	per_cpu(ds_sys_status, 0).sysfs_min_cpu_op_index = DS_CPU_OP_INDEX_11; 	// 300 MHz
	per_cpu(ds_sys_status, 0).sysfs_max_cpu_op_index = DS_CPU_OP_INDEX_4; 	// 1 GHz

	return(0);
}


/*====================================================================
	The functions to initialize dvs_suite_system_status structure.
	====================================================================*/

int ds_initialize_ds_sys_status(void){

	per_cpu(ds_sys_status, 0).locked_min_cpu_op_index =
		per_cpu(ds_sys_status, 0).sysfs_min_cpu_op_index;
	per_cpu(ds_sys_status, 0).locked_min_cpu_op_release_sec = 0;
	per_cpu(ds_sys_status, 0).flag_locked_min_cpu_op = 0;

	per_cpu(ds_sys_status, 0).locked_max_cpu_op_index =
		per_cpu(ds_sys_status, 0).sysfs_max_cpu_op_index;
	per_cpu(ds_sys_status, 0).locked_min_iva_freq = 0;
	per_cpu(ds_sys_status, 0).locked_min_l3_freq = 0;

	per_cpu(ds_sys_status, 0).flag_touch_timeout_count = 0;
	per_cpu(ds_sys_status, 0).touch_timeout_sec = 0;
	per_cpu(ds_sys_status, 0).touch_timeout_usec = 0;

	per_cpu(ds_sys_status, 0).flag_consecutive_touches = 0;
	per_cpu(ds_sys_status, 0).new_touch_sec = 0;
	per_cpu(ds_sys_status, 0).new_touch_usec = 0;
	per_cpu(ds_sys_status, 0).first_consecutive_touch_sec = 0;
	per_cpu(ds_sys_status, 0).flag_long_consecutive_touches = 0;

	per_cpu(ds_sys_status, 0).flag_post_early_suspend = 0;
	per_cpu(ds_sys_status, 0).flag_do_post_early_suspend = 0;
	per_cpu(ds_sys_status, 0).do_post_early_suspend_sec = 0;

	return(0);
}

/*====================================================================
	The functions to initialize dvs_suite_cpu_status structure.
	====================================================================*/

int ds_initialize_ds_cpu_status(int ds_cpu, int cpu_mode){

	per_cpu(ds_cpu_status, ds_cpu).cpu_mode = cpu_mode;
	per_cpu(ds_cpu_status, ds_cpu).dvs_suite_mutex = 0;

	per_cpu(ds_cpu_status, ds_cpu).flag_update_cpu_op = 0;
	per_cpu(ds_cpu_status, ds_cpu).cpu_op_mutex = 0;

	per_cpu(ds_cpu_status, ds_cpu).current_cpu_op_index = DS_CPU_OP_INDEX_INI;
	per_cpu(ds_cpu_status, ds_cpu).target_cpu_op_index = DS_CPU_OP_INDEX_INI;
	per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_sec = 0;
	per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_usec = 0;

	return(0);
}

/*====================================================================
	The functions to initialize dvs_suite_counter structure.
	====================================================================*/

int ds_initialize_ds_counter(int ds_cpu, int mode){

	if(mode == 0) per_cpu(ds_counter, ds_cpu).flag_counter_initialized = 0;
	per_cpu(ds_counter, ds_cpu).counter_mutex = 0;

	if(mode == 0){
		per_cpu(ds_counter, ds_cpu).wall_usec_base = 0;
		per_cpu(ds_counter, ds_cpu).idle_usec_base = 0;
		per_cpu(ds_counter, ds_cpu).iowait_usec_base = 0;

		per_cpu(ds_counter, ds_cpu).elapsed_sec = 0;
		per_cpu(ds_counter, ds_cpu).elapsed_usec = 0;

		per_cpu(ds_counter, ds_cpu).busy_sec = 0;
		per_cpu(ds_counter, ds_cpu).busy_usec = 0;
	}
	else{
		per_cpu(ds_counter, ds_cpu).wall_usec_base = per_cpu(ds_counter, 0).wall_usec_base;
		per_cpu(ds_counter, ds_cpu).idle_usec_base = per_cpu(ds_counter, 0).idle_usec_base;
		per_cpu(ds_counter, ds_cpu).iowait_usec_base = per_cpu(ds_counter, 0).iowait_usec_base;

		per_cpu(ds_counter, ds_cpu).elapsed_sec = per_cpu(ds_counter, 0).elapsed_sec;
		per_cpu(ds_counter, ds_cpu).elapsed_usec = per_cpu(ds_counter, 0).elapsed_usec;

		per_cpu(ds_counter, ds_cpu).busy_sec = per_cpu(ds_counter, 0).busy_sec;
		per_cpu(ds_counter, ds_cpu).busy_usec = per_cpu(ds_counter, 0).busy_usec;
	}

	return(0);
}

/*====================================================================
	The functions to initialize ds_aidvs_status structure.
	====================================================================*/

int ds_initialize_aidvs(int ds_cpu, int mode){

	per_cpu(ds_aidvs_status, ds_cpu).moving_avg_weight = ds_control.aidvs_moving_avg_weight;
	per_cpu(ds_aidvs_status, ds_cpu).interval_length = ds_control.aidvs_interval_length;

	if(mode == 0){
		per_cpu(ds_aidvs_status, ds_cpu).time_usec_interval = 0;
		per_cpu(ds_aidvs_status, ds_cpu).time_sec_interval_inc_base =
			per_cpu(ds_counter, ds_cpu).elapsed_sec;
		per_cpu(ds_aidvs_status, ds_cpu).time_usec_interval_inc_base =
			per_cpu(ds_counter, ds_cpu).elapsed_usec;

		per_cpu(ds_aidvs_status, ds_cpu).time_usec_work = 0;
		per_cpu(ds_aidvs_status, ds_cpu).time_sec_work_inc_base =
			per_cpu(ds_counter, ds_cpu).busy_sec;
		per_cpu(ds_aidvs_status, ds_cpu).time_usec_work_inc_base =
			per_cpu(ds_counter, ds_cpu).busy_usec;
	}
	else{
		per_cpu(ds_aidvs_status, ds_cpu).time_usec_interval =
			per_cpu(ds_aidvs_status, 0).time_usec_interval;
		per_cpu(ds_aidvs_status, ds_cpu).time_sec_interval_inc_base =
			per_cpu(ds_aidvs_status, 0).time_sec_interval_inc_base;
		per_cpu(ds_aidvs_status, ds_cpu).time_usec_interval_inc_base =
			per_cpu(ds_aidvs_status, 0).time_usec_interval_inc_base;

		per_cpu(ds_aidvs_status, ds_cpu).time_usec_work =
			per_cpu(ds_aidvs_status, 0).time_usec_work;
		per_cpu(ds_aidvs_status, ds_cpu).time_sec_work_inc_base =
			per_cpu(ds_aidvs_status, 0).time_sec_work_inc_base;
		per_cpu(ds_aidvs_status, ds_cpu).time_usec_work_inc_base =
			per_cpu(ds_aidvs_status, 0).time_usec_work_inc_base;
	}

	per_cpu(ds_aidvs_status, ds_cpu).utilization_int_ulong = 1;
	per_cpu(ds_aidvs_status, ds_cpu).utilization_fra_fp12 = 0x0;

	per_cpu(ds_aidvs_status, ds_cpu).moving_avg_int_ulong = 1;
	per_cpu(ds_aidvs_status, ds_cpu).moving_avg_fra_fp12 = 0x0;

	per_cpu(ds_aidvs_status, ds_cpu).cpu_op_index = DS_CPU_OP_INDEX_INI;

	return(0);
}

/*====================================================================
	Function to change the priority of normal tasks
	NOTE: We apply the RR rt scheduler for HRT and DS_SRT_UI_SERVER_TASK.
		  On the other hand, we apply the normal scheduler for
		  other tasks.
	====================================================================*/

int ds_update_priority_normal(int ds_cpu, struct task_struct *p){

	int lc_existing_nice = 0;
	int lc_nice_by_type = 0;
	int lc_static_prio_by_type = 0;
	int lc_resultant_nice = 0;
	int lc_resultant_static_prio = 0;

	if(p == 0 || p->pid == 0) return(0);
	if((per_cpu(ds_sys_status, 0).type[p->pid] & DS_TYPE_2B_CHANGED_M) == 0) return(0);

	lc_existing_nice = p->static_prio - 120;

	switch(per_cpu(ds_sys_status, 0).type[p->pid] & DS_TYPE_M){
		case DS_HRT_TASK:
			lc_nice_by_type = DS_HRT_NICE;
			lc_static_prio_by_type = DS_HRT_STATIC_PRIO;
			break;
		case DS_SRT_UI_SERVER_TASK:
			lc_nice_by_type = DS_DBSRT_NICE;
			lc_static_prio_by_type = DS_DBSRT_STATIC_PRIO;
			break;
		case DS_SRT_UI_CLIENT_TASK:
			lc_nice_by_type = DS_DBSRT_NICE;
			lc_static_prio_by_type = DS_DBSRT_STATIC_PRIO;
			break;
		case DS_SRT_KERNEL_THREAD:
			lc_nice_by_type = DS_RBSRT_NICE;
			lc_static_prio_by_type = DS_RBSRT_STATIC_PRIO;
			break;
		case DS_SRT_DAEMON_TASK:
			lc_nice_by_type = DS_RBSRT_NICE;
			lc_static_prio_by_type = DS_RBSRT_STATIC_PRIO;
			break;
		case DS_NRT_TASK:
			lc_nice_by_type = DS_NRT_NICE;
			lc_static_prio_by_type = DS_NRT_STATIC_PRIO;
			break;
		default:
			lc_nice_by_type = DS_NRT_NICE;
			lc_static_prio_by_type = DS_NRT_STATIC_PRIO;
			break;
	}

	lc_resultant_nice = lc_nice_by_type + lc_existing_nice;
	if(lc_resultant_nice < -20) lc_resultant_nice = -20;
	if(lc_resultant_nice > 19) lc_resultant_nice = 19;

#if 1
	lc_resultant_static_prio = lc_static_prio_by_type + lc_existing_nice;
	if(lc_resultant_static_prio < 100) lc_resultant_static_prio = 100;
	if(lc_resultant_static_prio > 139) lc_resultant_static_prio = 139;

	p->static_prio = lc_resultant_static_prio;
	p->normal_prio = p->static_prio;
	p->prio = p->static_prio;
#endif

	set_user_nice(p, lc_resultant_nice);

	per_cpu(ds_sys_status, 0).type[p->pid] &= DS_TYPE_2B_CHANGED_N;

	return(0);
}

/*====================================================================
	Function to trace every task's type
	====================================================================*/
int ds_detect_task_type(int ds_cpu){

	int old_type = 0;
	int new_type = 0;
	int i = 0;

	int next_pid;

	/* Upon ds_parameter.entry_type == DS_ENTRY_SWITCH_TO, do followings.

		 For ds_parameter.next_p:

		(1) Check if per_cpu(ds_sys_status, 0).type[next_pid] & DS_TYPE_M is still its initial value.
			If it is, determine it.
	 */
	if(ds_parameter.entry_type == DS_ENTRY_SWITCH_TO)
	{

		/* For prev_p */
#if 0
		if(ds_parameter.prev_p != 0){
			if(ds_parameter.prev_p->pid != 0){
				// Nothing to do now.
			}
		}
#endif

		/* For next_p */
		if(ds_parameter.next_p != 0)
		{

			next_pid = ds_parameter.next_p->pid;

			if(next_pid != 0){

				if(next_pid == ds_parameter.next_p->tgid)
				{
					for(i=0;i<16;i++)
						per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid][i] = 
						*(ds_parameter.next_p->comm+i);
				}

				/* Get old type. */
				old_type = per_cpu(ds_sys_status, 0).type[next_pid] & DS_TYPE_M;
				if(old_type == 0) old_type = DS_NRT_TASK;

				/* C) Initialize and then determine new type. */
				new_type = old_type;

				/*----------------------------------------------------------------------
				 * Manual type detection
				 ----------------------------------------------------------------------*/
				switch(*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+0)){
					case 'a':
#if 0	// youtube. [application process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'u' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'u' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'e'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'b':
#if 0	// /system/bin/dbus-daemon. bluetooth related [daemon process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'i' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'n' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == '/' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'd' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'b' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'u' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 's' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == '-' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'd' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'n'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// /system/bin/mediaserver. [daemon process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'v' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'r'
						)
						{
							//new_type = DS_HRT_TASK;
							new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'c':
#if 1	// com.lge.camera. LGE On-Screen Phone [application]. O 4 touch.
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'o' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'm' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'l' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'g' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'c' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'a' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'a'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else 
#endif
#if 0	// com.lge.media. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'o' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'm' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'l' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'g' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'm' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'a'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else 
#endif
#if 0	// com.broadcom.bt.app.pbap. bluetooth. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'p'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'd':
#if 1	// com.android.lgecamera. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'r' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'o' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'i' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'd' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'l' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'g' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'c' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'a'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// android.process.media. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'p' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'r' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'o' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'c' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 's' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 's' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'a'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// com.android.bluetooth. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'r' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'o' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'i' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'd' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'b' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'l' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'u' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'h'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 1	// android.process.acore. Contact, home, and etc. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'p' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'r' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'o' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'c' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 's' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 's' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'c' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'e'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_SRT_KERNEL_TASK;
							//new_type = DS_NRT_TASK;
#if 0
if(per_cpu(ds_sys_status, 0).flag_touch_timeout_count != 0){
printk(KERN_DEBUG "\n");
}
#endif
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'e':
#if 0	// com.google.process.gapps. [application]. Google services.
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'p' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'r' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'o' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'c' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'e' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 's' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 's' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == '.' && 
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'g' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 's'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'f':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'g':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'h':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'i':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'j':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'k':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'l':
#if 0	// com.lge.videoplayer. [application process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'g' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'v' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'l' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'r'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'm':
#if 0	// com.lge.launcher2. [application process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'l' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'g' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'l' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'u' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'c' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'h' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == '2'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							//per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// com.android.phone. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'h' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'e'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// com.android.music. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'u' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'c'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// com.broadcom.bt.app.system. bluetooth. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'm'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'n':
#if 0	// com.android.systemui. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'u' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'i'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'o':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'p':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'q':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'r':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 's':
#if 1	// system_server. Android system service [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == '_' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'v' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'r'
						)
						{
							//new_type = DS_HRT_TASK;
							new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// /system/bin/netd. Network related [daemon process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'd'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// /system/bin/rild. RIL related [daemon process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'l' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'd'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// /system/bin/vold. Volume (sdcard etc.) server [daemon process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'v' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'l' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'd'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 't':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'u':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'v':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'w':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'x':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'y':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'z':
#if 0	// zygote. Android process spawning [daemon process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'y' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'g' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'e'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'A':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'B':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'C':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'D':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'E':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'F':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'G':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'H':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'I':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'J':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'K':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'L':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'M':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'N':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'O':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'P':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'Q':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'R':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'S':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'T':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'U':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'V':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'W':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'X':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'Y':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case 'Z':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case ' ':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case '_':
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case '/':
#if 0	// /system/bin/servicemanager. Android system service [daemon process]
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'v' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'c' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'a' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'g' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'r'
						)
						{
							//new_type = DS_HRT_TASK;
							new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// /sbin/ueventd. Polling server [daemon process].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'b' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == '/' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'u' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 'v' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'n' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 't' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'd'
						)
						{
							new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							//new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
					case '.':
#if 0	// android.process.lghome. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'p' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'c' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == 'l' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'g' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'h' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'e'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
#if 0	// com.cooliris.media. Gallary related. [application].
						if(
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+1) == 'c' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+2) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+3) == 'o' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+4) == 'l' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+5) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+6) == 'r' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+7) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+8) == 's' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+9) == '.' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+10) == 'm' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+11) == 'e' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+12) == 'd' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+13) == 'i' &&
							*(per_cpu(ds_sys_status, 0).tg_owner_comm[next_pid]+14) == 'a'
						)
						{
							//new_type = DS_HRT_TASK;
							//new_type = DS_SRT_UI_SERVER_TASK;
							new_type = DS_SRT_UI_CLIENT_TASK;
							//new_type = DS_SRT_KERNEL_THREAD;
							//new_type = DS_SRT_DAEMON_TASK;
							//new_type = DS_NRT_TASK;
							per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
							goto DS_TASK_TYPE_DETECTION_DONE;
						}
						else
#endif
						goto DS_UNKNOWN_TASK_NAME;
						break;
				}

DS_UNKNOWN_TASK_NAME:

				/*----------------------------------------------------------------------
				 * Autonomous type detection
				 ----------------------------------------------------------------------*/
#if 0
				if(next_pid == 1 ||
					next_pid == 2 ||
					ds_parameter.next_p->parent->pid == 2)
				{
					new_type = DS_SRT_KERNEL_THREAD;
					per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
					goto DS_TASK_TYPE_DETECTION_DONE;
				}
				else if(ds_parameter.next_p->parent->pid == 1){
					new_type = DS_SRT_DAEMON_TASK;
				}
				else{
					new_type = DS_NRT_TASK;
				}
#else	/* Apply this without autonomous type detection */
new_type = DS_NRT_TASK;
per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_FIXED_M;
goto DS_TASK_TYPE_DETECTION_DONE;
#endif

DS_TASK_TYPE_DETECTION_DONE:

				/* D) Check type change. */
				if(old_type != new_type){
					per_cpu(ds_sys_status, 0).type[next_pid] &= DS_TYPE_N;
					per_cpu(ds_sys_status, 0).type[next_pid] |= new_type;
					per_cpu(ds_sys_status, 0).type[next_pid] |= DS_TYPE_2B_CHANGED_M;
				}
			}
		}
	}

	return(0);
}

/*====================================================================
	The function which updates the CPU operating point.
	====================================================================*/
asmlinkage void ds_update_cpu_op(int ds_cpu)
{
	struct device *mpu_dev;	// Moved from cpu-omap.c
	struct device *l3_dev;	// Moved from cpu-omap.c
	struct device *iva_dev; // Moved from cpu-omap.c

	unsigned long lc_min_cpu_op_update_interval = 0;

	if(!cpu_active(ds_cpu)) return;
	if(!ds_control.flag_run_dvs) return;
	if(!per_cpu(ds_cpu_status, ds_cpu).flag_update_cpu_op) return;
	if(per_cpu(ds_cpu_status, ds_cpu).cpu_op_mutex) return;
	per_cpu(ds_cpu_status, ds_cpu).cpu_op_mutex ++;

	if(per_cpu(ds_cpu_status, ds_cpu).target_cpu_op_index > 
		per_cpu(ds_cpu_status, ds_cpu).current_cpu_op_index)
		lc_min_cpu_op_update_interval = DS_MIN_CPU_OP_UPDATE_INTERVAL_U;
	else
		lc_min_cpu_op_update_interval = DS_MIN_CPU_OP_UPDATE_INTERVAL_D;

	/* If this is the first time */
	if(per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_sec == 0 && 
		per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_usec == 0)
	{
		goto update_cpu_op;
	}
	else{
		/* If lc_min_cpu_op_update_interval is over since the last update */
		if(per_cpu(ds_counter, ds_cpu).elapsed_sec > 
			per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_sec + 1)
		{
			goto update_cpu_op;
		}
		else if(per_cpu(ds_counter, ds_cpu).elapsed_sec > 
			per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_sec)
		{
			if(per_cpu(ds_counter, ds_cpu).elapsed_usec > 
				per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_usec)
			{
				goto update_cpu_op;
			}
			else if(per_cpu(ds_counter, ds_cpu).elapsed_usec + 1000000
				> per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_usec + 
				lc_min_cpu_op_update_interval)
			{
				goto update_cpu_op;
			}
			else{
				goto do_not_update;
			}
		}
		else{
			if(per_cpu(ds_counter, ds_cpu).elapsed_usec
				> per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_usec + 
				lc_min_cpu_op_update_interval)
			{
				goto update_cpu_op;
			}
			else{
				goto do_not_update;
			}
		}
	}

update_cpu_op:

		mpu_dev = omap2_get_mpuss_device();
		l3_dev = omap2_get_l3_device();
		iva_dev = omap2_get_iva_device();

		switch(per_cpu(ds_cpu_status, ds_cpu).target_cpu_op_index){
#ifdef CONFIG_P970_OPPS_ENABLED
			case DS_CPU_OP_INDEX_0:
				omap_pm_cpu_set_freq(1350000000);	// VDD1_OPP13	1.35GHz
				omap_device_set_rate(mpu_dev, mpu_dev, 1350000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 1000000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_1:
				omap_pm_cpu_set_freq(1300000000);	// VDD1_OPP12	1.3GHz
				omap_device_set_rate(mpu_dev, mpu_dev, 1300000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 970000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_2:
				omap_pm_cpu_set_freq(1200000000);	// VDD1_OPP11	1.2GHz
				omap_device_set_rate(mpu_dev, mpu_dev, 1200000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 930000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_3:
				omap_pm_cpu_set_freq(1100000000);	// VDD1_OPP10	1.1GHz
				omap_device_set_rate(mpu_dev, mpu_dev, 1100000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 870000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
#endif
			case DS_CPU_OP_INDEX_4:
				omap_pm_cpu_set_freq(1000000000);	// VDD1_OPP9	1GHz
				omap_device_set_rate(mpu_dev, mpu_dev, 1000000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 800000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_5:
				omap_pm_cpu_set_freq(900000000);	// VDD1_OPP8	900MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 900000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 730000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_6:
				omap_pm_cpu_set_freq(800000000);	// VDD1_OPP7	800MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 800000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 660000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_7:
				omap_pm_cpu_set_freq(700000000);	// VDD1_OPP6	700MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 700000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 590000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_8:
				omap_pm_cpu_set_freq(600000000);	// VDD1_OPP5	600MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 600000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 520000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_9:
				omap_pm_cpu_set_freq(500000000);	// VDD1_OPP4	500MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 500000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 440000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_10:
				omap_pm_cpu_set_freq(400000000);	// VDD1_OPP3	400MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 400000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 350000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
#if 0
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
#endif
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_11:
				omap_pm_cpu_set_freq(300000000);	// VDD1_OPP2	300MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 300000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 260000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
#if 0
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
#endif
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_12:
				omap_pm_cpu_set_freq(200000000);	// VDD1_OPP1	200MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 200000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 170000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
#if 0
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
#endif
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
			case DS_CPU_OP_INDEX_13:
			default:
				omap_pm_cpu_set_freq(100000000);	// VDD1_OPP0	100MHz
				omap_device_set_rate(mpu_dev, mpu_dev, 100000000);
				if(per_cpu(ds_sys_status, 0).locked_min_iva_freq == 0){
					omap_device_set_rate(iva_dev, iva_dev, 90000000);
				}
				else{
					omap_device_set_rate(iva_dev, iva_dev, per_cpu(ds_sys_status, 0).locked_min_iva_freq);
				}
				if(per_cpu(ds_sys_status, 0).locked_min_l3_freq == 0){
#if 0
					if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0)
						omap_device_set_rate(l3_dev, l3_dev, 200000000);
					else
#endif
						omap_device_set_rate(l3_dev, l3_dev, 100000000);
				}
				else{
					omap_device_set_rate(l3_dev, l3_dev, per_cpu(ds_sys_status, 0).locked_min_l3_freq);
				}
				break;
		}

	per_cpu(ds_cpu_status, ds_cpu).current_cpu_op_index = 
		per_cpu(ds_cpu_status, ds_cpu).target_cpu_op_index;

	per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_sec = 
		per_cpu(ds_counter, ds_cpu).elapsed_sec;
	per_cpu(ds_cpu_status, ds_cpu).cpu_op_last_update_usec = 
		per_cpu(ds_counter, ds_cpu).elapsed_usec;

	per_cpu(ds_cpu_status, ds_cpu).flag_update_cpu_op = 0;

do_not_update:

	per_cpu(ds_cpu_status, ds_cpu).cpu_op_mutex --;

	return;
}
EXPORT_SYMBOL(ds_update_cpu_op);

/*====================================================================
	The functions which up or down the auxiliary cores.
	====================================================================*/
#if 0	// Need CONFIG_HOTPLUG_CPU
void ds_up_aux_cpu(int ds_cpu, int cpu){

	if(cpu == 0) return;

	if(ds_cpu == 0 && !cpu_active(cpu)){
		cpu_hotplug_driver_lock();
		cpu_up(cpu);
		cpu_hotplug_driver_unlock();
	}

	return;
}

void ds_down_aux_cpu(int ds_cpu, int cpu){

	if(cpu == 0) return;

	if(ds_cpu == 0 && cpu_active(cpu)){
		cpu_hotplug_driver_lock();
		cpu_down(cpu);
		cpu_hotplug_driver_unlock();
	}

	return;
}
#endif

/*====================================================================
	The main dynamic voltage scaling and
	performance evaluation kernel function.
	====================================================================*/

/* This function is called at the end of context_swtich()
 * and update_process_times().
 */
void do_dvs_suite(int ds_cpu){

	unsigned int lc_target_cpu_op_index = 0;

	if(per_cpu(ds_cpu_status, ds_cpu).dvs_suite_mutex != 0) return;
	per_cpu(ds_cpu_status, ds_cpu).dvs_suite_mutex ++;

	per_cpu(ds_cpu_status, ds_cpu).cpu_mode = DS_CPU_MODE_DVS_SUITE;

	/* dvs_suite has been activated. */
	if(ds_control.flag_run_dvs == 1)
	{
		/* Delayed application of the special treatment upon early suspend */
		if(ds_cpu == 0){
			if(per_cpu(ds_sys_status, 0).flag_post_early_suspend == 1){
				if(per_cpu(ds_sys_status, 0).flag_do_post_early_suspend == 0){
					if(per_cpu(ds_sys_status, 0).do_post_early_suspend_sec <
						per_cpu(ds_counter, ds_cpu).elapsed_sec)
					{
						per_cpu(ds_sys_status, 0).flag_do_post_early_suspend = 1;
					}
				}
			}
		}

		/* Detect task type, set task priority, and determine cpu frequency */
		if(ds_parameter.entry_type == DS_ENTRY_SWITCH_TO)
		{
			if((per_cpu(ds_sys_status, 0).type[ds_parameter.next_p->pid] & 
				DS_TYPE_FIXED_M) == 0)
			{
				ds_detect_task_type(ds_cpu);
			}
		}
		else{	// DS_ENTRY_TIMER_IRQ

			/* Check if consecutive touches have been ended - Begin */
			if(ds_cpu == 0){
				if(per_cpu(ds_sys_status, 0).flag_consecutive_touches == 1){
					if((per_cpu(ds_counter, ds_cpu).elapsed_sec - 
						per_cpu(ds_sys_status, 0).new_touch_sec) * 1000000 +
						(per_cpu(ds_counter, ds_cpu).elapsed_usec - 
						per_cpu(ds_sys_status, 0).new_touch_usec) > DS_CONT_TOUCH_THRESHOLD_USEC)
					{
						per_cpu(ds_sys_status, 0).flag_consecutive_touches = 0;
						per_cpu(ds_sys_status, 0).flag_long_consecutive_touches = 0;
					}
				}
			}
			/* Check if consecutive touches have been ended - End */

			ds_do_dvs_gpschedvs(ds_cpu, &lc_target_cpu_op_index);

			/* Schedule the actual CPU frequency and voltage changes. */
			if(lc_target_cpu_op_index != per_cpu(ds_cpu_status, ds_cpu).current_cpu_op_index)
			{
				per_cpu(ds_cpu_status, ds_cpu).flag_update_cpu_op = 1;
				per_cpu(ds_cpu_status, ds_cpu).target_cpu_op_index = lc_target_cpu_op_index;
			}

			/* Control the auxiliary cores - Begin */
#if 0
			if(ds_cpu == 0){
				if(per_cpu(ds_sys_status, 0).flag_long_consecutive_touches == 1)
					ds_down_aux_cpu(ds_cpu, 1);
			}
#endif
			/* Control the auxiliary cores - End */
		}
	}

	/* dvs_suite has not been activated yet. */
	else{

		/* Delayed activation of LG-DVFS */
		if(ds_cpu == 0){
			if(per_cpu(ds_counter, ds_cpu).elapsed_sec > DS_INIT_DELAY_SEC){
				ds_control.flag_run_dvs = 1;
				printk(KERN_INFO "[LG-DVFS] LG-DVFS starts running.\n");
			}
		}
	}

	if(ds_parameter.entry_type == DS_ENTRY_TIMER_IRQ)
	{
		if(current->pid == 0)
			per_cpu(ds_cpu_status, ds_cpu).cpu_mode = DS_CPU_MODE_IDLE;
		else
			per_cpu(ds_cpu_status, ds_cpu).cpu_mode = DS_CPU_MODE_TASK;
	}
	else{	// DS_ENTRY_SWITCH_TO
		per_cpu(ds_cpu_status, ds_cpu).cpu_mode = DS_CPU_MODE_SCHEDULE;
	}

	per_cpu(ds_cpu_status, ds_cpu).dvs_suite_mutex --;

	return;
}

void do_dvs_suite_timer(struct work_struct *work){

	int ds_cpu = smp_processor_id();

	ds_update_cpu_op(ds_cpu);

	return;
}

inline void dvs_suite_timer_init(void){

	INIT_WORK(&dvs_suite_work, do_dvs_suite_timer);

	return;
}

inline void dvs_suite_timer_exit(void){

	cancel_work_sync(&dvs_suite_work);

	return;
}

static int __init lg_dvfs_init(void)
{
	printk(KERN_WARNING "lg_dvfs_init\n");

	return 0;
}

static void __exit lg_dvfs_exit(void)
{
	printk(KERN_WARNING "lg_dvfs_exit\n");
}

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("cpu");
//MODULE_LICENSE("GPL");

module_init(lg_dvfs_init);
module_exit(lg_dvfs_exit);
