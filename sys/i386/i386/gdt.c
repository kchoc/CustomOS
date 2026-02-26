#include "gdt.h"
#include "x86/include/segment.h"

seg_desc_t gdt[NGDT] = {
	// GNULL_SEL
	{
		.sd_low_limit = 0,
		.sd_high_limit = 0,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = 0,
		.sd_dpl = SEL_KPL,
		.sd_present = 0,
		.sd_reserved = 0,
		.sd_size = 0,
		.sd_granularity = 0
	},
	// GPRIV_SEL
	{
		.sd_low_limit = 0xffff,
		.sd_high_limit = 0xf,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = SEL_MEM_RWA,
		.sd_dpl = SEL_KPL,
		.sd_present = 1,
		.sd_reserved = 0,
		.sd_size = 1, // 32-bit segment
		.sd_granularity = 1 // 4KB granularity
	},
	// GUFS_SEL
	{
		.sd_low_limit = 0xffff,
		.sd_high_limit = 0xf,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = SEL_MEM_RWA,
		.sd_dpl = SEL_UPL,
		.sd_present = 1,
		.sd_reserved = 0,
		.sd_size = 1, // 32-bit segment
		.sd_granularity = 1 // 4KB granularity
	},
	// GUGS_SEL
	{
		.sd_low_limit = 0xffff,
		.sd_high_limit = 0xf,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = SEL_MEM_RWA,
		.sd_dpl = SEL_UPL,
		.sd_present = 1,
		.sd_reserved = 0,
		.sd_size = 1, // 32-bit segment
		.sd_granularity = 1 // 4KB granularity
	},
	// GCODE_SEL
	{
		.sd_low_limit = 0xffff,
		.sd_high_limit = 0xf,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = SEL_MEM_ERA,
		.sd_dpl = SEL_KPL,
		.sd_present = 1,
		.sd_reserved = 0,
		.sd_size = 1, // 32-bit segment
		.sd_granularity = 1 // 4KB granularity
	},
	// GDATA_SEL
	{
		.sd_low_limit = 0xffff,
		.sd_high_limit = 0xf,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = SEL_MEM_RWA,
		.sd_dpl = SEL_KPL,
		.sd_present = 1,
		.sd_reserved = 0,
		.sd_size = 1, // 32-bit segment
		.sd_granularity = 1 // 4KB granularity
	},
	// GUCODE_SEL
	{
		.sd_low_limit = 0xffff,
		.sd_high_limit = 0xf,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = SEL_MEM_ERA,
		.sd_dpl = SEL_UPL,
		.sd_present = 1,
		.sd_reserved = 0,
		.sd_size = 1, // 32-bit segment
		.sd_granularity = 1 // 4KB granularity
	},
	// GUDATA_SEL
	{
		.sd_low_limit = 0xffff,
		.sd_high_limit = 0xf,
		.sd_low_base = 0,
		.sd_high_base = 0,
		.sd_type = SEL_MEM_RWA,
		.sd_dpl = SEL_UPL,
		.sd_present = 1,
		.sd_reserved = 0,
		.sd_size = 1, // 32-bit segment
		.sd_granularity = 1 // 4KB granularity
	}
};