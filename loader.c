/* $Id: loader.c,v 1.3 2001/12/09 09:06:26 andrew Exp $
 * loads a glsnake modelfile
 *
 * Lines beginning with '#' are comments and are ignored
 * Whitespace is (generally) ignored
 * Models are stored in one line of a name, a colon, then a list of rotations
 * Rotations are either 'L', 'R', 'P', 'Z', or 1, 2, 3, 4.  Case insensitive.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "model.h"


model_t * add_model(model_t * models, char * name, int * rotations, int * count) {
	int i;
	
	(*count)++;
	models = realloc(models, sizeof(model_t) * (*count));
	models[(*count)-1].name = strdup(name);
#ifdef DEBUG
	fprintf(stderr, "resized models to %d bytes for model %s\n", sizeof(model_t) * (*count), models[(*count)-1].name);
#endif
	for (i = 0; i < 24; i++) {
		models[(*count)-1].node[i] = rotations[i] * 90.0;
	}
	return models;
}

/* filename is the name of the file to load
 * models is the pointer to where the models will be kept
 * returns a new pointer to models
 * count is number of models read
 */
model_t * load_modelfile(char * filename, model_t * models, int * count) {
	char c;
	FILE * f;
	char buffy[256];
	int rotations[24];
	int name = 1;
	int rots = 0;

	f = fopen(filename, "r");
	if (f == NULL) {
		int error_msg_len = strlen(filename) + 33 + 1;
		char * error_msg = (char *) malloc(sizeof(char) * error_msg_len);
		sprintf(error_msg, "Unable to open model data file \"%s\"", filename);
		perror(error_msg);
		free(error_msg);
		return models;
	}
		
	while ((c = getc(f)) != EOF) {
		switch (c) {
			/* ignore comments */
			case '#':
				while (c != '\n')
					c = getc(f);
				break;
			case ':':
				buffy[name-1] = '\0';
				name = 0;
				break;
			case '\n':
				if (rots > 0) {
#ifdef DEBUG
					/* print out the model we just read in */
					int i;
					printf("%s: ", buffy);
					for (i = 0; i < rots; i++) {
						switch (rotations[i]) {
							case LEFT:
								printf("L");
								break;
							case RIGHT:
								printf("R");
								break;
							case PIN:
								printf("P");
								break;
							case ZERO:
								printf("Z");
								break;
						}
					}
					printf("\n");
#endif
					models = add_model(models, buffy, rotations, count);
				}
				name = 1;
				rots = 0;
				break;
			default:
				if (name) {
					buffy[name-1] = c;
					name++;
					if (name > 255)
						fprintf(stderr, "buffy overflow warning\n");
				} else {
					switch (c) {
						case '0':
						case 'Z':
							rotations[rots] = ZERO;
							rots++;
							break;
						case '1':
						case 'L':
							rotations[rots] = LEFT;
							rots++;
							break;
						case '2':
						case 'P':
							rotations[rots] = PIN;
							rots++;
							break;
						case '3':
						case 'R':
							rotations[rots] = RIGHT;
							rots++;
							break;
						default:
							break;
					}
				}
				break;
		}
	}
	return models;
}
