#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include "../src/private/charwidth.h"
#include "../src/private/graphbreak.h"
#include "../src/utf8lite.h"

const char *str_charwidth(enum charwidth_prop prop)
{
	switch (prop) {
	case CHARWIDTH_OTHER:
		return "Other";
	case CHARWIDTH_EMOJI:
		return "Emoji";
	case CHARWIDTH_AMBIGUOUS:
		return "Ambiguous";
	case CHARWIDTH_IGNORABLE:
		return "Ignorable";
	case CHARWIDTH_NONE:
		return "None";
	case CHARWIDTH_NARROW:
		return "Narrow";
	case CHARWIDTH_WIDE:
		return "Wide";
	default:
		assert(0 && "Unrecognized charwidth property");
	}
}

const char *str_graph_break(enum graph_break_prop prop)
{
	switch (prop) {
	case GRAPH_BREAK_OTHER:
		return "None";
	case GRAPH_BREAK_CR:
		return "CR";
	case GRAPH_BREAK_CONTROL:
		return "Control";
	case GRAPH_BREAK_E_BASE:
		return "EBase";
	case GRAPH_BREAK_E_BASE_GAZ:
		return "EBaseGAZ";
	case GRAPH_BREAK_E_MODIFIER:
		return "EModifier";
	case GRAPH_BREAK_EXTEND:
		return "Extend";
	case GRAPH_BREAK_GLUE_AFTER_ZWJ:
		return "GlueAfterZWJ";
	case GRAPH_BREAK_L:
		return "L";
	case GRAPH_BREAK_LF:
		return "LF";
	case GRAPH_BREAK_LV:
		return "LV";
	case GRAPH_BREAK_LVT:
		return "LVT";
	case GRAPH_BREAK_PREPEND:
		return "Prepend";
	case GRAPH_BREAK_REGIONAL_INDICATOR:
		return "RegionalIndicator";
	case GRAPH_BREAK_SPACINGMARK:
		return "SpacingMark";
	case GRAPH_BREAK_T:
		return "T";
	case GRAPH_BREAK_V:
		return "V";
	case GRAPH_BREAK_ZWJ:
		return "ZWJ";
	default:
		assert(0 && "Unrecognized graph break property");
	}
}

int main(int argc, const char **argv)
{
	int32_t i, n = UTF8LITE_CODE_MAX;
	int cw, gb;

	printf("code,width,graph\n");
	for (i = 0; i <= n; i++) {
		cw = charwidth(i);
		gb = graph_break(i);
		printf("U+%04"PRIX32",%s,%s\n", (uint32_t)i,
		       str_charwidth(cw), str_graph_break(gb));
	}

	return 0;
}
