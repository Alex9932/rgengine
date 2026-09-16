#ifndef _FILEDIALOG_H
#define _FILEDIALOG_H

#include "rgtypes.h"

typedef struct {
	String name;
	String spec;
} FD_Filter;

namespace Engine {

	void RG_NFDInit();
	void RG_NFDDestroy();

	RG_DECLSPEC Bool ShowOpenDialog(char* dst_path, Uint32 maxlen, FD_Filter* filters, Uint32 filter_count);
}

#endif