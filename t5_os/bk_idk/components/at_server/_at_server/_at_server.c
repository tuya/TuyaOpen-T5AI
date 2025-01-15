#include "_at_server.h"
#include "_atsvr_func.h"
#include "atsvr_port.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include <ctype.h>


//static const char *_atsvr_version_num = ATSVR_VERSION_NUM;
extern _at_svr_ctrl_env_t _at_svr_env;
//extern struct atsvr_port_env atsvr_port;
extern int atsvr_cmd_forward(char *cmd, u16 cmd_len);
extern void atsvr_cmd_ind_out(const char *format, ...);

extern int atsvr_get_cpu_id(void);

//char *_get_atsvr_version(void)
//{
//	return (char*)_atsvr_version_num;
//}


void _set_atsvr_echo_mode(_atsvr_env_t *env,_atsvr_echo_t echo)
{
	if(env){
		env->echo = echo;
	}
}

_atsvr_echo_t _get_atsvr_echo_mode(_atsvr_env_t *env)
{
	return env->echo;
}

int _atsvr_register_command(_atsvr_env_t *env,const struct _atsvr_command *command)
{
	int i,j;
	int cmd_nums;

	if (!command->name || ((!command->query_func)&&(!command->setup_func))) 
		{
			return ATSVR_ERROR;
		}
	if (env->num_commands[0]!= 0) 
	{
		/* Check if the command has already been registered.
		* Return 0, if it has been registered.
		*/
		for (i = 0; i < env->cmds_reg_index; i++)
		{
			cmd_nums = env->num_commands[i];
			if(cmd_nums == 0)
				continue;
			const struct _atsvr_command**ext_command = &env->commands[i];
			for(j = 0 ; j < cmd_nums;j++)
			{
				if (0 == strcmp((*ext_command)[j].name,command->name))
				{
					ATSVRLOGE("command \"%s\" has been registered,cannot register again\r\n",command->name);	
					return ATSVR_GENERAL;
				}	
			}
		}
		return ATSVR_OK;
	}
	else
	/* This is the first register.*/
	{
		return ATSVR_OK;
	}	
}


int _atsvr_register_commands(_atsvr_env_t *env,const struct _atsvr_command *commands_table,int num_commands, char *tag,atsvr_event_cb evecb)
{
	if(commands_table == NULL)
		return ATSVR_ERROR;
	for (int i = 0; i < num_commands; i++) 
	{
		const struct _atsvr_command **command = &commands_table; 
		int sta;
		sta = _atsvr_register_command(env, (*command+i));
		//ATSVRLOG("command \"%s\" \r\n",commands_table[i].name);	
		if (sta != ATSVR_OK)
		{
			if(sta == ATSVR_ERROR)
			{
				ATSVRLOGE("this command  %s has no name/query/setup func,need check\r\n",commands_table[i].name);
				BK_ASSERT(0);
			}
			else if(sta == ATSVR_GENERAL)
			{
				ATSVRLOGE("this command  %s same as former,you can register other cmds,but the same one can not work\r\n",commands_table[i].name);
				return ATSVR_OK;
			}
		}
	}
	/* this cmd table can be registered*/
	if((env->cmds_reg_index) < ATSVR_MAX_COMMANDS)
	{
		env->commands[env->cmds_reg_index] = commands_table;
		env->num_commands[env->cmds_reg_index] = num_commands;
		env->cmd_type[env->cmds_reg_index] = tag;
		env->cmd_event_cb[env->cmds_reg_index] = evecb;
		env->cmds_reg_index+=1;
		return ATSVR_OK;
	}
	else
	{
		ATSVRLOGE("it has no mem,cannot register this commands\r\n");	
		return ATSVR_NnMemoryErr;
	}
	

}

void _atsvr_notice_ready(void)
{
#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
	shell_cmd_ind_out(ATSVR_READY_MSG);
#else
	atsvr_cmd_ind_out(ATSVR_READY_MSG);
#endif
	return;
}

#if 0
void _atsvr_cmd_analysis_notice_error(void)
{
#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
	shell_cmd_ind_out(ATSVR_CMDMSG_ERROR_RSP);
#else
	atsvr_cmd_ind_out(ATSVR_CMDMSG_ERROR_RSP);
#endif
	return;
}
#endif


void _atsvr_cmd_rsp_ok(_atsvr_env_t *env)
{
	if(env->setup_status!= _ATSVR_SETUP_TIMEOUT)
	{
		env->setup_done = true;
		env->setup_status = _ATSVR_SETUP_DONE;
		#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
		shell_cmd_ind_out(ATSVR_CMD_RSP_SUCCEED);
		#else
		atsvr_cmd_ind_out(ATSVR_CMD_RSP_SUCCEED);
		#endif
	}	
	return;
}


void _atsvr_cmd_rsp_error(_atsvr_env_t *env)
{
	if(env->setup_status!= _ATSVR_SETUP_TIMEOUT)
	{
		env->setup_done = true;
		env->setup_status = _ATSVR_SETUP_ERROR;
		#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
			shell_cmd_ind_out(ATSVR_CMDMSG_ERROR_RSP);
		#else
			atsvr_cmd_ind_out(ATSVR_CMDMSG_ERROR_RSP);
		#endif
	}
	return;
}

void _atsvr_cmd_rsp_timeout(_atsvr_env_t *env)
{
	env->setup_done = false;
	env->setup_status = _ATSVR_SETUP_TIMEOUT;

	#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
		shell_cmd_ind_out(ATSVR_CMDMSG_TIMEOUT);
	#else
		atsvr_cmd_ind_out(ATSVR_CMDMSG_TIMEOUT);
	#endif
}

void _atsvr_output_help(const char *msg)
{
#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
	shell_cmd_ind_out(msg);
#else
	atsvr_cmd_ind_out(msg);
#endif

}

void _atsvr_output_msg(char *msg)
{
#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
	shell_cmd_ind_out(msg);
#else
	atsvr_cmd_ind_out(msg);
#endif

}


static void _atsvr_timeout_cb(void* cmd)
{
	struct _atsvr_command* command = (struct _atsvr_command*)cmd;
	if(command->bassert)
	{
		ATSVRLOGE("[ATSVR]command:%s is time out\r\n",command->name);
		BK_ASSERT(0);
	}
	else
	{
		ATSVRLOGE("[ATSVR]command:%s is time out,check next cmd\r\n",command->name);
		_atsvr_cmd_rsp_timeout(&_at_svr_env);
		rtos_deinit_timer(_at_svr_env.atsvr_timer);	
		_at_svr_env.atsvr_timer = NULL;
	}
	return;
}

static void proc_func_timeout_cb(void* ptr)
{
	ATSVRLOGE("[ATSVR]command_timer is too loong ,need check if use dead loop\r\n");
	beken_timer_t* tptr = (beken_timer_t*)ptr;
	rtos_deinit_timer(tptr);
	_atsvr_cmd_rsp_timeout(&_at_svr_env);
	return;
}


static const struct _atsvr_command *_atsvr_lookup_at_command(_atsvr_env_t *env,char *name, int len)
{


	int name_len,cmd_name_len;

	name_len = strlen(name);

	for(int i = 0;i < ATSVR_MAX_COMMANDS;i++)
	{
		const struct _atsvr_command  **command_table  = &env->commands[i];
		for(int n = 0;n<env->num_commands[i];n++)
		{
			if ((*command_table)[n].name == NULL) {
				n++;
				continue;
			}
#if defined(ATSVR_OPTIM_FD_CMD) && ATSVR_OPTIM_FD_CMD
		cmd_name_len = (*command_table)[n].name_len;
#else
		cmd_name_len = strlen((*command_table)[n].name);
#endif
		/* See if partial or full match is expected */
		if (len != 0) 
		{
			if ((cmd_name_len >= len)
				&& (!strncmp((*command_table)[n].name, name, len))) 
				{
				return &(*command_table)[n];
				}
		}
		else 
		{
			if ((cmd_name_len == name_len)
				&& (!strcmp((*command_table)[n].name, name))) 
			{
				//_at_svr_env.cmd_index = i;
				return &(*command_table)[n];
			}
		}
		continue;
		}
	continue;	
	}
#if (CONFIG_CPU_CNT >1)
#if CONFIG_SYS_CPU0
	if(true == env->cpu1_ready)
	{
		ATSVRLOGE("[ATSVR]command in cpu0 is null,we will check cpu1 sooner\r\n");
		env->priority_cpu = _ATSVR_CPU1;
	}
	#endif	
#endif
	return NULL;
}

int _atsvc_command_handle(_atsvr_env_t *env,char argc,char **argv,int len)
{
    const struct _atsvr_command *command = NULL;
	atsvr_handler func = NULL;
    int i;
    char *p;
	bool  is_setfunc;
	static beken_timer_t func_proc;//protect the function in case of function time is too long >100ms	
#if (CONFIG_CPU_CNT > 1)
#if CONFIG_SYS_CPU0
#if CONFIG_AT_SUPPORT_MULTICORE
	int ret;
#endif
#endif
#endif
    i = ((p = strchr(argv[0], '.')) == NULL) ? 0 : (p - argv[0]);
#if CONFIG_SYS_CPU0
	if((strcmp("AT+BLE",argv[0]) == 0) || (strcmp("AT+WIFI",argv[0]) == 0) || (strcmp("AT+BT",argv[0]) == 0)  || (strcmp("AT+MAC",argv[0]) == 0))
	{
	#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
		shell_cmd_ind_out(ATSVR_CMD_NEW_VERSION_NOTIFY);
	#else
		atsvr_cmd_ind_out(ATSVR_CMD_NEW_VERSION_NOTIFY);
	#endif
		return ATSVR_OK;
	}
#endif
#if (CONFIG_CPU_CNT > 1)
#if CONFIG_SYS_CPU0
#if CONFIG_AT_SUPPORT_MULTICORE	
	/*add AT+PING = CPUX command,if we catch it,we need not to lookup,just send it to peer CPU*/
	if(strcmp("AT+CPATH",argv[0]) == 0)
	{
		if(strcmp("0",argv[1])==0)
		{ 
			env->priority_cpu = _ATSVR_CPU0;
			env->core_sel_model = false;
		#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
			shell_cmd_ind_out(ATSVR_CMD_RSP_SUCCEED);
		#else
			atsvr_cmd_ind_out(ATSVR_CMD_RSP_SUCCEED);
		#endif
			return ATSVR_OK;
		}
		if(strcmp("1",argv[1])==0)
		{
			env->priority_cpu = _ATSVR_CPU1;
			env->core_sel_model = true;
		#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
			shell_cmd_ind_out(ATSVR_CMD_RSP_SUCCEED);
		#else
			atsvr_cmd_ind_out(ATSVR_CMD_RSP_SUCCEED);
		#endif
			return ATSVR_OK;
		}
	}
	/*we will send the AT cmd to peer CPU*/
	if(env->core_sel_model==true)
	{	
		if(env->priority_cpu == _ATSVR_CPU1)//cmd  use sel_cpu and send it to cpu1 and cpu1 is ready
		{
			if(env->cpu1_ready)
			{

				char* mb_ptr = malloc(sizeof(char*)*argc+1);
				char* p = mb_ptr;
				*mb_ptr++ = argc;
				memcpy(mb_ptr,argv,sizeof(char*)*argc);
				GLOBAL_INT_DECLARATION();
				GLOBAL_INT_DISABLE();
				ret = atsvr_cmd_forward(p,sizeof(char*)*argc+1);		
				GLOBAL_INT_RESTORE();
				if((env->core_sel_model == false) && (env->priority_cpu == _ATSVR_CPU1))
					env->priority_cpu = _ATSVR_CPU0;
				if(ret != 1)/*for mb,return 1 means a success write*/
				{
					free(p);
					return ATSVR_ERROR;
				}
				else
				{
					free(p);
					return ATSVR_OK;
				}
			}
			else
			{
				ATSVRLOGE("you use the sel cmd but cpu1 is not ready,cmd:%s is not working\r\n",argv[0]);
			}
		}
	}
	else{
#endif
#endif
#endif
    command = _atsvr_lookup_at_command(env,argv[0], i);

#if (CONFIG_CPU_CNT > 1)
#if CONFIG_SYS_CPU0
#if CONFIG_AT_SUPPORT_MULTICORE	
	}
	if(env->priority_cpu == _ATSVR_CPU1)//cmd not use sel_cpu and not match in cpu0 and cpu1 is ready
	{
		char* mb_ptr = malloc(sizeof(char*)*argc+1);
		char* p = mb_ptr;
		*mb_ptr++ = argc;
		memcpy(mb_ptr,argv,sizeof(char*)*argc);
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		ret = atsvr_cmd_forward(p,sizeof(char*)*argc+1);		
		GLOBAL_INT_RESTORE();
		if((env->core_sel_model == false) && (env->priority_cpu == _ATSVR_CPU1))
			env->priority_cpu = _ATSVR_CPU0;
		if(ret != 1)/*for mb,return 1 means a success write*/
		{
			free(p);
			return ATSVR_ERROR;
		}
		else
		{
			free(p);
			return ATSVR_OK;
		}	
	}
#endif
#endif
#endif
	if(command == NULL)
	{
		ATSVRLOGE("[ATSVR]command in cpu%d is null\r\n",atsvr_get_cpu_id());
		return ATSVR_CMD_NULL;
	}

    if ( (!command->query_func)&&(!command->setup_func)) //cmd has no both query func and setup func
	{
		ATSVRLOGE("[ATSVR]cmd \"%s\" is an incomplete command without query&setup function\r\n",argv[0]);
        return ATSVR_INCOMPLETE_CMD;
    }
	else
	{
		if(!command->setup_func)//cmd has only a query func
		{
			if(argc >1)
			{		
				if(strcmp("?",argv[1]) == 0)
				{
					func = command->query_func;
					is_setfunc = false;
				}
				else
				{
					ATSVRLOGE("[ATSVR]cmd %s is a setup cmd,but you set no setup funcion,need check\r\n",command->name);
					return ATSVR_INCOMPLETE_CMD;
				}
			}
			else
			{
				ATSVRLOGE("[ATSVR]cmd %s is a setup cmd2,but you set no setup funcion,need check\r\n",command->name);
				return ATSVR_INCOMPLETE_CMD;
			}
		}
		else
		{
			if(!command->query_func)//cmd has only a setup func
			{		
				if(argc>1)
				{
					if(strcmp("?",argv[1]) == 0)
					{		
						ATSVRLOGE("[ATSVR]cmd %s is a query cmd,but you set no query funcion,need check\r\n",argv[0]);
						return ATSVR_GENERAL;
					}
					else
					{
						func = command->setup_func;
						is_setfunc = true;
					}
				}
				else
				{
					func = command->setup_func;
					is_setfunc = true;
				}
			}
			else //cmd has query and setup funcs
			{
				if(argc>1)
				{
					if(strcmp("?",argv[1]) == 0)
					{		
						func = command->query_func;
						is_setfunc = false;
					}
					else
					{
						func = command->setup_func;
						is_setfunc = true;
					}
				}
				else
				{
					func = command->setup_func;
					is_setfunc = true;
				}
			}
		}
	}
	if(command->timeout)
	{
		if(command->sync_time - _ATSVR_DEFAULT_TIME > 0)
			rtos_init_timer(&func_proc,_ATSVR_DEFAULT_TIME,proc_func_timeout_cb,(void*)&func_proc);
		else
			if(command->timeout_func == NULL)
				rtos_init_timer(&func_proc,command->timeout,proc_func_timeout_cb,(void*)command);
			else
				rtos_init_timer(&func_proc,command->timeout,command->timeout_func,(void*)&func_proc);
	}
	else
	{
			rtos_init_timer(&func_proc,_ATSVR_DEFAULT_TIME,proc_func_timeout_cb,(void*)&func_proc);
	}
	if(is_setfunc == true)
		if(argc==1)
			func(command->sync,0, NULL);
		else
			func(command->sync,argc-1, argv+1);
	else
		if(argc==2)
			func(command->sync,0, NULL);
		else
			func(command->sync,argc-2, argv+2);
	//use sema?
	if(env->setup_done == true)
	{
		if(env->setup_status == _ATSVR_SETUP_DONE)
		{		
			ATSVRLOG("[ATSVR]command:%s is not time out and success\r\n",command->name);
			if(&func_proc!=NULL)
			{
				rtos_deinit_timer(&func_proc);	
			}

		}
		else if(_at_svr_env.setup_status == _ATSVR_SETUP_ERROR)
		{
			ATSVRLOGE("[ATSVR]command:%s is not time out but error\r\n",command->name);
			if(&func_proc!=NULL)
			{
				rtos_deinit_timer(&func_proc);
			}
			
		}
	}
	else
	{
		if(_at_svr_env.setup_status == _ATSVR_SETUP_TIMEOUT)
		{	
			ATSVRLOGE("[ATSVR]command:%s is  time out and failed!!!\r\n",command->name);
			if(&func_proc!=NULL)
			{
				rtos_deinit_timer(&func_proc);	
			}
			return ATSVR_ERROR;
		}
	}
	
    return ATSVR_OK;
}

typedef struct{
	unsigned inArg : 1;
	unsigned done : 1;
	unsigned isD : 2;
	unsigned limQ : 1;
}_atsvr_handle_input_stat;

static int _atsvr_handle_input(_atsvr_env_t *env,unsigned char *inbuf,int len)
{
	_atsvr_handle_input_stat stat;
    char *argv[ATSVR_MAX_ARG];
    int argc = 0;
    int i = 0;
	int offset = 0;

    memset((void *)&argv, 0, sizeof(argv));
    memset(&stat, 0, sizeof(stat));

    do
    {
		offset++;
		if(offset > len){
			ATSVRLOGE("The data is incomplete\r\n");
			return ATSVR_INCOMPLETE_CMD;  ////error
		}
                if(atsvr_islower(inbuf[i])&& stat.isD == 0)
		{
			inbuf[i] = toupper(inbuf[i]);
		}
        switch (inbuf[i])
        {
        case '\0':
			if(((argc == 0)||(stat.isD == 1))||(stat.limQ)){
				ATSVRLOGE("The data does not conform to the regulations %d\r\n",__LINE__);
				return ATSVR_INREGULAR_CMD;
			}

			stat.done = 1;
            break;
        case '=':
			if (i > 0 && inbuf[i - 1] == '\\' && stat.inArg) {
				memcpy((void*)&inbuf[i - 1], (const char*)&inbuf[i],
						  strlen((const char*)&inbuf[i]) + 1);
				--i;
				break;
			}
            if(argc == 1) {
				inbuf[i] = '\0';
				stat.inArg = 0;
				stat.isD = 1;
			}
            else if(argc == 0){
				ATSVRLOGE("The data does not conform to the regulations %d\r\n",__LINE__);
				return ATSVR_INREGULAR_CMD;
            }
            break;
#if 0 //ATSVR_ADD_ESCAPE_CFG
		case '\\':  ////"\"
			offset += 1;
			if((offset + 1) > len){
				ATSVRLOGE("The data does not conform to the regulations %d\r\n",__LINE__);
				return ATSVR_INREGULAR_CMD;  ////error
			}
			memmove(&inbuf[i],&inbuf[i+1],(len-offset) + 1);
            break;
#endif
        case ',':
			if (i > 0 && inbuf[i - 1] == '\\' && stat.inArg) {
				memcpy((void*)&inbuf[i - 1], (const char*)&inbuf[i],
						  strlen((const char*)&inbuf[i]) + 1);
				--i;
				break;
			}
            if((stat.isD == 1)&&(argc == 1))  ///=,
            {
				ATSVRLOGE("The data does not conform to the regulations %d\r\n",__LINE__);
                return ATSVR_INREGULAR_CMD;
            }
            if(stat.inArg) {
                stat.inArg = 0;
                inbuf[i] = '\0';
                stat.limQ = 1;
            }
            break;
        default:
            if(!stat.inArg) {
                stat.inArg = 1;
                argc++;
                argv[argc - 1] = (char*)&inbuf[i];
                stat.limQ = 0;
                if(stat.isD == 1) {
                    stat.isD = 2;
                }
            }
            break;
        }
    }
    while (!stat.done && (++i < ATSVR_INPUT_BUFF_MAX_SIZE));

    if (argc < 1) {
		ATSVRLOGE("Data parsing exception\r\n");
        return ATSVR_DATA_PARSE_ERROR;
    }

    return _atsvc_command_handle(env,argc,argv,len);
}


int _atsvr_input_msg_analysis_handler(_atsvr_env_t *env,char *msg,unsigned int msg_len)
{
	int ret;

	if(env == NULL){
		return ATSVR_SEVERE_ERR;
	}
	ret = _atsvr_handle_input(env,(unsigned char*)msg,msg_len);

	return ret;

}


int _atsvr_def_config(_atsvr_env_t *env)
{

	if(env == NULL)
	{
		return ATSVR_SEVERE_ERR;
	}
	env->cmd_index = 0;
	for(int i = 0; i < ATSVR_MAX_COMMANDS; i++)
	{
		env->num_commands[i] = 0;
		env->commands[i] = 0;
		env->cmd_type[i] = 0;
	}

	env->echo = ATSVR_ECHO_DEFAULT;
	env->cmds_reg_index = 0;
	return ATSVR_OK;
}


_atsvr_work_st _get_atsvr_work_state(_atsvr_env_t *env)
{
	_atsvr_work_st wk_st;
	wk_st = env->wk_st;
	return wk_st;
}

void _set_atsvr_work_state(_atsvr_env_t *env,_atsvr_work_st wk_st)
{
	env->wk_st = wk_st;
	return;
}


