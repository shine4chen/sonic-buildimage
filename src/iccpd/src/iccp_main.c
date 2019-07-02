/*
* iccp_main.c
*
* Copyright(c) 2016-2019 Nephos/Estinet.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along with
* this program; if not, see <http://www.gnu.org/licenses/>.
*
* The full GNU General Public License is included in this distribution in
* the file called "COPYING".
*
*  Maintainer: jianjun, grace Li from nephos
*/

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../include/cmd_option.h"
#include "../include/logger.h"
#include "../include/scheduler.h"
#include "../include/system.h"

int check_instance(char* pid_file_path) 
{
    int pid_file = 0;
    int rc = 0;

    if (pid_file_path == NULL)
        return -1;

    pid_file = open(pid_file_path, O_CREAT | O_RDWR, 0666);
    if(pid_file <=0 ) 
    {
        fprintf(stderr, "Can't open a pid file. Terminate.\n");
        close(pid_file);
        exit(EXIT_FAILURE);
    }
    
    rc = flock(pid_file, LOCK_EX | LOCK_NB);

    if (rc) 
    {
        if (errno == EWOULDBLOCK) 
        {
            fprintf(stderr, "There is another instance running. Terminate.\n");
            close(pid_file);
            exit(EXIT_FAILURE);
        }
    }
    
    return pid_file;
}

void init_daemon(char* pid_file_path, int pid_file) 
{
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) 
    {
        fprintf(stderr, "Failed to enter daemon mode: %s\n", strerror(errno));
        fprintf(stderr, "Please try to check your system resources.\n");
        close(pid_file);
        unlink(pid_file_path);
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);
    sid = setsid();
    if (sid < 0) 
    {
        fprintf(stderr, "Failed to create a new SID for this program: %s\n", strerror(errno));
        fprintf(stderr, "Please try to check your system resources.\n");
        close(pid_file);
        unlink(pid_file_path);
        exit(EXIT_FAILURE);
    }

    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}

#ifndef ICCPD_RUN_DIR
#define ICCPD_RUN_DIR "/var/run/iccpd/"
#endif

static inline int iccpd_make_rundir(void)
{
    int ret;

    ret = mkdir(ICCPD_RUN_DIR, 0755);
    if (ret && errno != EEXIST) 
    {
    	ICCPD_LOG_ERR(__FUNCTION__,"Failed to create directory \"%s\"",
    		      ICCPD_RUN_DIR);
    		      
    	return -errno;
    }
    
    return 0;
}

int main(int argc, char* argv[]) 
{
    int pid_file_fd = 0;
    struct System* sys = NULL;
    int err;
    struct CmdOptionParser parser = CMD_OPTION_PARSER_INIT_VALUE;

    err = iccpd_make_rundir();
    if (err)
	return 0;
		
    if (getuid() != 0) 
    {
        fprintf(stderr,
                "This program needs root permission to do device manipulation. "
                        "Please use sudo to execute it or change your user to root.\n");
        exit(EXIT_FAILURE);
    }
    
    parser.init(&parser);
    if (parser.parse(&parser, argc, argv) != 0) 
    {
        parser.finalize(&parser);
        return -1;
    }
    
    pid_file_fd = check_instance(parser.pid_file_path);
    if (pid_file_fd < 0) 
    {
        fprintf(stderr, "Check instance with invalidate arguments, iccpd is terminated.\n");
        parser.finalize(&parser);
        exit(EXIT_FAILURE);
    }
    
    sys = system_get_instance();
    if (!sys) 
    {
        fprintf(stderr, "Can't get a system instance, iccpd is terminated.\n");
        parser.finalize(&parser);
        exit(EXIT_FAILURE);
    }
    
    /*if(!parser.console_log)
        init_daemon(parser.pid_file_path, pid_file_fd);*/
    
    log_init(&parser);       
        
    if (sys->log_file_path != NULL)
        free(sys->log_file_path);
    if (sys->cmd_file_path != NULL)
        free(sys->cmd_file_path);
    if (sys->config_file_path != NULL)
        free(sys->config_file_path);
    sys->log_file_path = strdup(parser.log_file_path);
    sys->cmd_file_path = strdup(parser.cmd_file_path);
    sys->config_file_path = strdup(parser.config_file_path);
    sys->mclagdctl_file_path = strdup(parser.mclagdctl_file_path);
    sys->pid_file_fd = pid_file_fd;
    sys->telnet_port = parser.telnet_port;
    parser.finalize(&parser);
    
    ICCPD_LOG_INFO(__FUNCTION__, "Iccpd is started, process id = %d.", getpid());
    scheduler_init();
    scheduler_start();
    scheduler_finalize();
    system_finalize();
    log_finalize();
    
    return EXIT_SUCCESS;
}
