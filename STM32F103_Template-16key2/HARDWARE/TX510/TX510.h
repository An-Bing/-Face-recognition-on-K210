#ifndef __TX510_H
#define __TX510_H

#include "sys.h"

#define K210_FACE_NONE      0
#define K210_FACE_KNOWN     1
#define K210_FACE_UNKNOWN   2

#define K210_OP_REGISTER    1
#define K210_OP_DELETE      2
#define K210_OP_CLEAR       3

#define K210_GRAY_MAX_PIXELS    256

void K210_SendRegister(u8 slot);
void K210_SendDelete(u8 slot);
void K210_SendClearAll(void);

u8 K210_GetFaceState(u8 *state, u8 *face_id, u8 *total);
u8 K210_FetchFaceUpdate(u8 *state, u8 *face_id, u8 *total);
u8 K210_FetchOpResult(u8 *op, u8 *result, u8 *slot, u8 *total);
u8 K210_FetchGrayImage(u8 *buf, u16 max_len, u8 *w, u8 *h, u8 *seq);
void K210_ClearFaceUpdate(void);
void K210_ClearOpResult(void);
void K210_ClearGrayImage(void);

#endif
