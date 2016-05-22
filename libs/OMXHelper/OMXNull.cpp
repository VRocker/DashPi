#include "OMXNull.h"


OMXNull::OMXNull()
{
	Initialise("OMX.broadcom.null_sink", OMX_IndexParamOtherInit);
}


OMXNull::~OMXNull()
{
}
