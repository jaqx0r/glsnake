/* $Id: model.h,v 1.1 2001/12/04 06:55:07 jaq Exp $
 */

#ifndef __MODEL_H__
#define __MODEL_H__

/* angles */
#define ZERO	0
#define LEFT	1
#define PIN    	2
#define RIGHT    3

typedef struct model_s {
	char * name;
	float node[24];
} model_t;

#endif /* __MODEL_H__ */
