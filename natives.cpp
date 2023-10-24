#include <time.h>
#include "natives.h"

Val clock_native(int num_args, Val* args) {
	return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
}