#include <time.h>
#include <stdio.h>
#include <libparodus.h>
#include <string.h>
#include "libpd.h"
#include "webpa_adapter.h"

#define CONTENT_TYPE_JSON       "application/json"
#define DEVICE_PROPS_FILE   "/etc/device.properties"
#define CLIENT_PORT_NUM     6667
#define URL_SIZE 	    64

static void connect_parodus();
static void get_parodus_url(char *parodus_url, char *client_url);
libpd_instance_t current_instance;
char parodus_url[URL_SIZE] = {'\0'};
char client_url[URL_SIZE] = {'\0'};

static void connect_parodus()
{
        int backoffRetryTime = 0;
        int backoff_max_time = 5;
        int max_retry_sleep;
        //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
        int c =2;
        int retval=-1;
      
	
	pthread_detach(pthread_self());

        max_retry_sleep = (int) pow(2, backoff_max_time) -1;
        WalInfo("max_retry_sleep is %d\n", max_retry_sleep );
        
	    get_parodus_url(parodus_url, client_url);
	
	    libpd_cfg_t cfg1 = {.service_name = "config",
					.receive = true, .keepalive_timeout_secs = 64,
					.parodus_url = parodus_url,
					.client_url = client_url
				   };
                
        WalPrint("libparodus_init with parodus url %s and client url %s\n",cfg1.parodus_url,cfg1.client_url);
        

        while(1)
        {
                if(backoffRetryTime < max_retry_sleep)
                {
                      backoffRetryTime = (int) pow(2, c) -1;
                }

                WalPrint("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
                int ret =libparodus_init (&current_instance, &cfg1);
                WalPrint("ret is %d\n",ret);
                if(ret ==0)
                {
                        WalInfo("Init for parodus Success..!!\n");
                        WalInfo("WebPA is now ready to process requests\n");
                        system("print_uptime \"boot_to_WEBPA_READY_uptime\" \"/rdklogs/logs/WEBPAlog.txt.0\"");
                        break;
                }
                else
                {
                        WalError("Init for parodus failed: '%s'\n",libparodus_strerror(ret));
                        sleep(backoffRetryTime);
                        c++;
                        
                        if(backoffRetryTime == max_retry_sleep)
                        {
                        	c = 2;
                        	backoffRetryTime = 0;
                        	WalPrint("backoffRetryTime reached max value, reseting to initial value\n");
                        }
                }
                retval = libparodus_shutdown(&current_instance);
                WalPrint("libparodus_shutdown retval %d\n", retval);
        }
}

	
void parodus_receive_wait()
{
	int rtn;
        wrp_msg_t *wrp_msg;
        wrp_msg_t *res_wrp_msg ;

        struct timespec start,end,*startPtr,*endPtr;
        startPtr = &start;
        endPtr = &end;
        char *contentType = NULL;

        while (1) 
        {
                rtn = libparodus_receive (current_instance, &wrp_msg, 2000);
                if (rtn == 1) 
                {
                        continue;
                }

                if (rtn != 0)
                {
                        WalError ("Libparodus failed to recieve message: '%s'\n",libparodus_strerror(rtn));
                        sleep(5);
                        continue;
                }
               
                if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
                {
                        res_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
                        memset(res_wrp_msg, 0, sizeof(wrp_msg_t));

                        getCurrentTime(startPtr);
                        processRequest((char *)wrp_msg->u.req.payload, wrp_msg->u.req.transaction_uuid, ((char **)(&(res_wrp_msg->u.req.payload))));

                        WalPrint("Response payload is %s\n",(char *)(res_wrp_msg->u.req.payload));
			if(res_wrp_msg->u.req.payload !=NULL)
                        {
                        	res_wrp_msg->u.req.payload_size = strlen(res_wrp_msg->u.req.payload);
			}
                        res_wrp_msg->msg_type = wrp_msg->msg_type;
                        res_wrp_msg->u.req.source = wrp_msg->u.req.dest;
                        res_wrp_msg->u.req.dest = wrp_msg->u.req.source;
                        res_wrp_msg->u.req.transaction_uuid = wrp_msg->u.req.transaction_uuid;
                        contentType = (char *)malloc(sizeof(char)*(strlen(CONTENT_TYPE_JSON)+1));
                        strncpy(contentType,CONTENT_TYPE_JSON,strlen(CONTENT_TYPE_JSON)+1);
                        res_wrp_msg->u.req.content_type = contentType;
                        int sendStatus = libparodus_send(current_instance, res_wrp_msg);     
                        WalPrint("sendStatus is %d\n",sendStatus);
                        if(sendStatus == 0)
                        {
                                WalInfo("Sent message successfully to parodus\n");
                        }
                        else
                        {
                                WalError("Failed to send message: '%s'\n",libparodus_strerror(sendStatus));
                        }
                        getCurrentTime(endPtr);
                        WalInfo("Elapsed time : %ld ms\n", timeValDiff(startPtr, endPtr));
                        wrp_free_struct (res_wrp_msg); 
                }        
                free(wrp_msg);
        }

        libparodus_shutdown(&current_instance);

        WalPrint ("End of parodus_upstream\n");
        return 0;
}

void sendNotification(char *payload, char *source, char *destination)
{
	wrp_msg_t *notif_wrp_msg = NULL;
	int retry_count = 0;
	int sendStatus = -1;
	int backoffRetryTime = 0;  
	int c=2;
	char *contentType = NULL;
	
	notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
	memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));

	notif_wrp_msg ->msg_type = WRP_MSG_TYPE__EVENT;
	WalPrint("source: %s\n",source);
	notif_wrp_msg ->u.event.source = source;
	WalPrint("destination: %s\n", destination);
	notif_wrp_msg ->u.event.dest = strdup(destination);
	contentType = (char *)malloc(sizeof(char)*(strlen(CONTENT_TYPE_JSON)+1));
  strncpy(contentType,CONTENT_TYPE_JSON,strlen(CONTENT_TYPE_JSON)+1);
  notif_wrp_msg->u.event.content_type = contentType;
  WalPrint("content_type is %s\n",notif_wrp_msg->u.event.content_type);
	
	WalInfo("Notification payload: %s\n",payload);
	notif_wrp_msg ->u.event.payload = (void *)payload;
	notif_wrp_msg ->u.event.payload_size = strlen(notif_wrp_msg ->u.event.payload);

	while(retry_count<=3)
	{
		backoffRetryTime = (int) pow(2, c) -1;	
		
		sendStatus = libparodus_send(current_instance, notif_wrp_msg );
		if(sendStatus == 0)
		{
			retry_count = 0;
			WalInfo("Notification successfully sent to parodus\n");
			break;
		}
		else
		{
			WalError("Failed to send Notification: '%s', retrying ....\n",libparodus_strerror(sendStatus));
			WalPrint("sendNotification backoffRetryTime %d seconds\n", backoffRetryTime);
			sleep(backoffRetryTime);
                        c++;
			retry_count++;
		}
	}

	WalPrint("sendStatus is %d\n",sendStatus);
	wrp_free_struct (notif_wrp_msg );
	WalPrint("Freed notif_wrp_msg struct.\n");
}

void libpd_client_mgr()
{
	WalPrint("Connect parodus \n");
        connect_parodus();
}       
             
static void get_parodus_url(char *parodus_url, char *client_url)
{

	FILE *fp = fopen(DEVICE_PROPS_FILE, "r");
	char atom_ip[URL_SIZE] = {'\0'};
	
	if (NULL != fp)
	{
		char str[255] = {'\0'};
		while(fscanf(fp,"%s", str) != EOF)
		{
		    char *value = NULL;
		    
		    if(value = strstr(str, "PARODUS_URL="))
		    {
			value = value + strlen("PARODUS_URL=");
			strncpy(parodus_url, value, (strlen(str) - strlen("PARODUS_URL=")));
		    }
		    
		    if(value = strstr(str, "ATOM_PROXY_SERVER="))
		    {
			value = value + strlen("ATOM_PROXY_SERVER=");
			strncpy(atom_ip, value, (strlen(str) - strlen("ATOM_PROXY_SERVER=")));
		    }
		   
		}
	}
	else
	{
		WalError("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
	}
	fclose(fp);
	
	if (0 == parodus_url[0])
	{
		WalError("parodus_url is not present in device. properties:%s\n", parodus_url);
	
	}
	
	if (0 == atom_ip[0])
	{
		WalError("atom_ip is not present in device. properties:%s\n", atom_ip);
	
	}
	
	snprintf(client_url, URL_SIZE, "tcp://%s:%d", atom_ip, CLIENT_PORT_NUM);
	WalPrint("client_url formed is %s\n", client_url);
	WalPrint("parodus_url formed is %s\n", parodus_url);	
 
 }
 
const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.WEBPA";
}

