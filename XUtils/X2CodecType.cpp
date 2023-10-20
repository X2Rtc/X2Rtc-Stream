#include "X2CodecType.h"

namespace x2rtc {

const char* getX2CodeTypeName(X2CodecType eType)
{
	switch (eType) {
#define X2T(name, value, str, desc) case name : return str;
		X2CodecType_MAP(X2T)
#undef X2T
	default: return "unknow";
	}
}

}	// namespace x2rtc