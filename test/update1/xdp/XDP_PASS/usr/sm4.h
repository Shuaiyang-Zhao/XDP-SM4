#ifndef SM4_H
#define SM4_H

#define u8 unsigned char
#define u32 unsigned long

extern const u8 Sbox[256];
extern const u32 FK[4];
extern const u32 CK[32];

u32 functionB(u32 b);
u32 loopLeft(u32 a, short length);
u32 functionL1(u32 a);
u32 functionL2(u32 a);
u32 functionT(u32 a, short mode);
void extendFirst(u32 MK[], u32 K[]);
void extendSecond(u32 RK[], u32 K[]);
void getRK(u32 MK[], u32 K[], u32 RK[]);
void iterate32(u32 X[], u32 RK[]);
void reverse(u32 X[], u32 Y[]);
void encryptSM4(u32 X[], u32 RK[], u32 Y[]);
void decryptSM4(u32 X[], u32 RK[], u32 Y[]);

#endif // SM4_H
