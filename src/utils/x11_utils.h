#pragma once

typedef struct {
	char id[64];
	int kelvin;
	double bright;
	double gamma;
} DisplayTarget;

DisplayTarget *find_target(DisplayTarget *targets, int num_targets, int index, const char *name);
