#ifndef x86_SEGMENT_H
#define x86_SEGMENT_H

#include <kern/compiler.h>

// Set Segment Values
#define SET_SEGMENT_LIMIT(seg, limit) {         \
	(seg).sd_low_limit = (limit) & 0xFFFF;       \
	(seg).sd_high_limit = ((limit) >> 16) & 0x0F; \
}

#define SET_SEGMENT_BASE(seg, base) {           \
	(seg).sd_low_base = (base) & 0xFFFFFF;       \
	(seg).sd_high_base = ((base) >> 24) & 0xFF;   \
}

typedef struct segment_descriptor_t {
	unsigned sd_low_limit  : 16;
	unsigned sd_low_base   : 24;
	unsigned sd_type	   : 5;
	unsigned sd_dpl	       : 2;
	unsigned sd_present    : 1;
	unsigned sd_high_limit : 4;
	unsigned sd_reserved   : 2;
	unsigned sd_size	   : 1; // 0 = 16-bit, 1 = 32-bit
	unsigned sd_granularity: 1;
	unsigned sd_high_base  : 8;
} __packed seg_desc_t;

_Static_assert(sizeof(seg_desc_t) == 8, "GDT entry must be 8 bytes");

/*
 * Selectors
 */
#define	SEL_RPL_MASK	3		/* requester priv level */
#define	ISPL(s)		((s)&3)		/* priority level of a selector */
#define	SEL_KPL		0		/* kernel priority level */
#define	SEL_UPL		3		/* user priority level */
#define	ISLDT(s)	((s)&SEL_LDT)	/* is it local or global */
#define	SEL_LDT		4		/* local descriptor table */
#define	IDXSEL(s)	(((s)>>3) & 0x1fff) /* index of selector */
#define	LSEL(s,r)	(((s)<<3) | SEL_LDT | r) /* a local selector */
#define	GSEL(s,r)	(((s)<<3) | r)	/* a global selector */

#define GNULL_SEL     	0	// Null segment
#define GPRIV_SEL 		1	// Per CPU private segment
#define GUFS_SEL 		2 	// User FS segment
#define GUGS_SEL 		3	// User GS segment
#define GCODE_SEL 		4	// Kernel CS segment
#define GDATA_SEL 		5	// Kernel DS segment
#define GUCODE_SEL 		6	// User CS segment
#define GUDATA_SEL 		7	// User DS segment
#define GBIOSLOWMEM_SEL	8	// BIOS low memory segment
#define GPROC0_SEL		9	// First per-process segment
#define	GLDT_SEL		10	// Kernel LDT
#define	GULDT_SEL		11	// User LDT
#define	GPANIC_SEL		12	// Task state to consider panic from
#define	GBIOSCODE32_SEL	13	// BIOS interface (32bit Code)
#define	GBIOSCODE16_SEL	14	// BIOS interface (16bit Code)
#define	GBIOSDATA_SEL	15	// BIOS interface (Data)
#define	GBIOSUTIL_SEL	16	// BIOS interface (Utility)
#define	GBIOSARGS_SEL	17	// BIOS interface (Arguments)
#define	GNDIS_SEL		18	// For the NDIS layer
#define	NGDT			8 	// TODO: Increase this when we need more segments

// System segments
#define SEL_SYS_NULL			0
#define SEL_SYS_TSS16_AVAILABLE	1
#define SEL_SYS_LDT				2
#define SEL_SYS_TSS16_BUSY		3
#define SEL_SYS_TSS32_AVAILABLE	9
#define SEL_SYS_TSS32_BUSY		11

// Memory segments
#define SEL_MEM_RO				16 // read-only
#define SEL_MEM_ROA				17 // read-only, accessed
#define SEL_MEM_RW				18 // read/Write
#define SEL_MEM_RWA				19 // read/Write, accessed
#define SEL_MEM_ROD				20 // downward, read-only
#define SEL_MEM_RODA			21 // downward, read-only, accessed
#define SEL_MEM_RWD				22 // downward, read/Write
#define SEL_MEM_RWDA			23 // downward, read/Write, accessed
#define SEL_MEM_E				24 // execute-only
#define SEL_MEM_EA				25 // execute-only, accessed
#define SEL_MEM_ER				26 // execute, read-only
#define SEL_MEM_ERA				27 // execute, read-only, accessed
#define SEL_MEM_EC				28 // execute-only, conforming
#define SEL_MEM_ECA				29 // execute-only, conforming, accessed
#define SEL_MEM_ECR				30 // execute, read-only, conforming
#define SEL_MEM_ECRA			31 // execute, read-only, conforming, accessed

#endif // SEGMENT_H
