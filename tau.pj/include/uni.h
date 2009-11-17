#ifndef _uni_h_
#define _uni_h_ 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STYLE_H_
#include <style.h>
#endif

extern u16	Uni_to_lower[];
extern u16	Uni_to_upper[];

static inline u16 uni_to_lower (u16 c)
{
	return Uni_to_lower[c];
}

static inline u16 uni_to_upper (u16 c)
{
	return Uni_to_upper[c];
}

#ifdef __cplusplus
}
#endif

#endif
