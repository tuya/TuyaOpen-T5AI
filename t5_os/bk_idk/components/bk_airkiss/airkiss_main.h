#ifndef _AIRKISS_MAIN_H_
#define _AIRKISS_MAIN_H_

#define MAX_CHANNELS                14
#define MAX_MAC                     50
#define AIRKISS_SWITCH_TIMER        50     // ms
#define AIRKISS_DOING_TIMER         20000  // 20s
#define MIN_SEL_CHAN_TIMER          1000   // ms
#define MAX_SEL_CHAN_TIMER          5000   // ms
#define AIRKISS_CONNECT_TIMER       60000  // Ms

#define AIRKISS_MIN_RX_BUF_SIZE     24   // fctrl + duration + mac1+ mac2 + mac3 + seq
#define MIN_UDP_RANDOM_SEND         20
#define MIN_VALID_DATACNT_INCHAN    4
#define MIN_VALID_BCNCNT_INCHAN     1

#define AIRKISS_DEBUG           1
#if AIRKISS_DEBUG
#define AIRKISS_PRT             os_printf
#define AIRKISS_WARN            warning_prf
#define AIRKISS_FATAL           fatal_prf
#else
#define AIRKISS_PRT             null_prf
#define AIRKISS_WARN            warning_prf
#define AIRKISS_FATAL           fatal_prf
#endif

typedef enum {
	AIRKISS_SCAN_ALL_CHAN = 0,
	AIRKISS_SCAN_SELECTED_CHAN
} airkiss_mode;

typedef struct {
	u8 bcn_cnt;
	u8 data_cnt;
	u16 channel;
} chan_param_t;

typedef struct {
	u8 frame_cnt;
	u8 mac_crc;
	u8 ap_sta;
	u16 channel;
} mac_param_t;

typedef struct {
	chan_param_t chan[MAX_CHANNELS];
	u8 cur_chan_idx;
	u8 all_chan_nums;
	u8 selected_chan_nums;
	u8 mode;
} airkiss_channel_t;

typedef struct {
	mac_param_t mac[MAX_MAC];
	u8 mac_cnt;
} airkiss_mac_t;

extern uint32_t airkiss_is_at_its_context(void);

#endif // _AIRKISS_MAIN_H_
// eof
