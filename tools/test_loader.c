#include <stdio.h>
#include "model.h"

model_t * load_modelfile(char *, model_t *, int *);

model_t * models;
int count = 0;

int main() {
	int i, j;
	
	models = load_modelfile("../data/models.glsnake", models, &count);
	fprintf(stderr, "%d models loaded\n", count);
	
	for (i = 0; i < count; i++) {
		printf("%s:\t", models[i].name);
		for (j = 0; j < 24; j++) {
			if (models[i].node[j] == 0.0)
				printf("Z");
			if (models[i].node[j] == 90.0)
				printf("L");
			if (models[i].node[j] == 180.0)
				printf("P");
			if (models[i].node[j] == 270.0)
				printf("R");
		}	
		printf("\n");
	}
	return 0;
}
