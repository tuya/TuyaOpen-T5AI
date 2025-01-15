#ifndef _RWNX_DEBUG_H_
#define _RWNX_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RWNX_MEM_STATS                   CONFIG_RWNX_MEM_STATS

/** RWNX memory stats */
struct rwnx_stats_mem {
    uint16_t tx_err;
    uint16_t rx_err;
};

/** RWNX stats container */
struct rwnx_stats_ {
    /** RWNX memory */
    struct rwnx_stats_mem rwnx_mem;
};

/** Global variable containing RWNX internal statistics. Add this to your debugger's watchlist. */
extern struct rwnx_stats_ rwnx_stats;

#define RWNX_STATS_INC(x) ++rwnx_stats.x
#define RWNX_STATS_DEC(x) --rwnx_stats.x

void stats_display_rwnx_mem(struct rwnx_stats_mem *mem);
void stats_reset_rwnx_mem(struct rwnx_stats_mem *mem);

#define RWNX_MEM_STATS_INC(x) RWNX_STATS_INC(rwnx_mem.x)
#define RWNX_MEM_STATS_RESET() stats_reset_rwnx_mem(&rwnx_stats.rwnx_mem)
#define RWNX_MEM_STATS_DISPLAY() stats_display_rwnx_mem(&rwnx_stats.rwnx_mem)

#ifdef __cplusplus
}
#endif

#endif // _RWNX_DEBUG_H_
// eof

