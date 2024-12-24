test step:
0, Reset_Handler() add:
	uint32_t dlv_is_startup(void);
	void dlv_startup(void);

	if(dlv_is_startup()){
		dlv_startup();
	}

1, portmacrocommon.h add macro:
	#define portSVC_DEEP_LV_ENTER              0x0D
	#define portSVC_DEEP_LV_EXIT               0x0E

2, port.c vPortSVCHandler_C add:
	extern void deep_lv_enter(void);
	extern void deep_lv_exit(void);

        case portSVC_DEEP_LV_ENTER:
			deep_lv_enter();
            break;
	case portSVC_DEEP_LV_EXIT:
		deep_lv_exit();
		break;

3, add two cli commands
	3.0, deep_low_voltage_test();
	3.1, cpu1_wakeup();

	void cpu1_wakeup(void)
	{
		vPortStartCore(1);
	}
