#ifndef __TX510_H
#define __TX510_H

#include "sys.h"

#define K210_FACE_NONE      0
#define K210_FACE_KNOWN     1
#define K210_FACE_UNKNOWN   2

#define K210_OP_REGISTER    1
#define K210_OP_DELETE      2
#define K210_OP_CLEAR       3

void K210_SendRegister(u8 slot);
void K210_SendDelete(u8 slot);
void K210_SendClearAll(void);

u8 K210_GetFaceState(u8 *state, u8 *face_id, u8 *total);
u8 K210_FetchFaceUpdate(u8 *state, u8 *face_id, u8 *total);
u8 K210_FetchOpResult(u8 *op, u8 *result, u8 *slot, u8 *total);
void K210_ClearFaceUpdate(void);
void K210_ClearOpResult(void);

#endif
