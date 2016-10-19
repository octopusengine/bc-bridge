#ifndef _TMP112_H
#define _TMP112_H

#define TMP112_REG_TEMPERATURE 0x00

#define TMP112_MASK_T 0xFFF0
#define TMP112_BIT_T11 0x8000
#define TMP112_BIT_T10 0x4000
#define TMP112_BIT_T9 0x2000
#define TMP112_BIT_T8 0x1000
#define TMP112_BIT_T7 0x0800
#define TMP112_BIT_T6 0x0400
#define TMP112_BIT_T5 0x0200
#define TMP112_BIT_T4 0x0100
#define TMP112_BIT_T3 0x0080
#define TMP112_BIT_T2 0x0040
#define TMP112_BIT_T1 0x0020
#define TMP112_BIT_T0 0x0010

#define TMP112_MASK_EM_T 0xFFF8
#define TMP112_BIT_EM_T12 0x8000
#define TMP112_BIT_EM_T11 0x4000
#define TMP112_BIT_EM_T10 0x2000
#define TMP112_BIT_EM_T9 0x1000
#define TMP112_BIT_EM_T8 0x0800
#define TMP112_BIT_EM_T7 0x0400
#define TMP112_BIT_EM_T6 0x0200
#define TMP112_BIT_EM_T5 0x0100
#define TMP112_BIT_EM_T4 0x0080
#define TMP112_BIT_EM_T3 0x0040
#define TMP112_BIT_EM_T2 0x0020
#define TMP112_BIT_EM_T1 0x0010
#define TMP112_BIT_EM_T0 0x0008

#define TMP112_REG_CONFIGURATION 0x01

#define TMP112_BIT_OS 0x8000
#define TMP112_MASK_R 0x6000
#define TMP112_BIT_R1 0x4000
#define TMP112_BIT_R0 0x2000
#define TMP112_MASK_F 0x1800
#define TMP112_BIT_F1 0x1000
#define TMP112_BIT_F0 0x0800
#define TMP112_BIT_POL 0x0400
#define TMP112_BIT_TM 0x0200
#define TMP112_BIT_SD 0x0100
#define TMP112_MASK_CR 0x00C0
#define TMP112_BIT_CR1 0x0080
#define TMP112_BIT_CR0 0x0040
#define TMP112_BIT_AL 0x0020
#define TMP112_BIT_EM 0x0010

#define TMP112_REG_T_LOW 0x02

#define TMP112_MASK_L 0xFFF0
#define TMP112_BIT_L11 0x8000
#define TMP112_BIT_L10 0x4000
#define TMP112_BIT_L9 0x2000
#define TMP112_BIT_L8 0x1000
#define TMP112_BIT_L7 0x0800
#define TMP112_BIT_L6 0x0400
#define TMP112_BIT_L5 0x0200
#define TMP112_BIT_L4 0x0100
#define TMP112_BIT_L3 0x0080
#define TMP112_BIT_L2 0x0040
#define TMP112_BIT_L1 0x0020
#define TMP112_BIT_L0 0x0010

#define TMP112_MASK_EM_L 0xFFF8
#define TMP112_BIT_EM_L12 0x8000
#define TMP112_BIT_EM_L11 0x4000
#define TMP112_BIT_EM_L10 0x2000
#define TMP112_BIT_EM_L9 0x1000
#define TMP112_BIT_EM_L8 0x0800
#define TMP112_BIT_EM_L7 0x0400
#define TMP112_BIT_EM_L6 0x0200
#define TMP112_BIT_EM_L5 0x0100
#define TMP112_BIT_EM_L4 0x0080
#define TMP112_BIT_EM_L3 0x0040
#define TMP112_BIT_EM_L2 0x0020
#define TMP112_BIT_EM_L1 0x0010
#define TMP112_BIT_EM_L0 0x0008

#define TMP112_REG_T_HIGH 0x03

#define TMP112_MASK_H 0xFFF0
#define TMP112_BIT_H11 0x8000
#define TMP112_BIT_H10 0x4000
#define TMP112_BIT_H9 0x2000
#define TMP112_BIT_H8 0x1000
#define TMP112_BIT_H7 0x0800
#define TMP112_BIT_H6 0x0400
#define TMP112_BIT_H5 0x0200
#define TMP112_BIT_H4 0x0100
#define TMP112_BIT_H3 0x0080
#define TMP112_BIT_H2 0x0040
#define TMP112_BIT_H1 0x0020
#define TMP112_BIT_H0 0x0010

#define TMP112_MASK_EM_H 0xFFF8
#define TMP112_BIT_EM_H12 0x8000
#define TMP112_BIT_EM_H11 0x4000
#define TMP112_BIT_EM_H10 0x2000
#define TMP112_BIT_EM_H9 0x1000
#define TMP112_BIT_EM_H8 0x0800
#define TMP112_BIT_EM_H7 0x0400
#define TMP112_BIT_EM_H6 0x0200
#define TMP112_BIT_EM_H5 0x0100
#define TMP112_BIT_EM_H4 0x0080
#define TMP112_BIT_EM_H3 0x0040
#define TMP112_BIT_EM_H2 0x0020
#define TMP112_BIT_EM_H1 0x0010
#define TMP112_BIT_EM_H0 0x0008

#endif /* _TMP112_H */
