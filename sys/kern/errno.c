#include "errno.h"

int is_errno(int code) {
	return code < 0 && code >= -MAX_ERRNO;
}
