#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "shell.h"
#include "msg.h"

#include "periph/adc.h"
#include "periph/gpio.h"
#include "analog_util.h"

#include "net/gnrc.h"
#include "net/netif.h"
#include "net/ipv6/addr.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/ipv6.h"

#include "net/emcute.h"

#ifndef EMCUTE_ID
#define EMCUTE_ID           ("gertrud")
#endif
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)

#define ADC_RES             ADC_RES_12BIT
#define DELAY               (5000LU * US_PER_MS) /* 5000 ms */
#define _IPV6_DEFAULT_PREFIX_LEN        (64U)

#define NUMOFSUBS           (16U)
#define TOPIC_MAXLEN        (64U)


static char stack[THREAD_STACKSIZE_DEFAULT];
static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];
static msg_t queue[8];

static uint8_t get_prefix_len(char *addr)
{
    int prefix_len = ipv6_addr_split_int(addr, '/', _IPV6_DEFAULT_PREFIX_LEN);

    if (prefix_len < 1) {
        prefix_len = _IPV6_DEFAULT_PREFIX_LEN;
    }

    return prefix_len;
}

static int netif_add(char *iface_name,char *addr_str)
{

    netif_t *iface = netif_get_by_name(iface_name);
        if (!iface) {
            puts("error: invalid interface given");
            return 1;
        }
    enum {
        _UNICAST = 0,
        _ANYCAST
    } type = _UNICAST;
    
    ipv6_addr_t addr;
    uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID;
    uint8_t prefix_len;


    prefix_len = get_prefix_len(addr_str);

    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("error: unable to parse IPv6 address.");
        return 1;
    }

    if (ipv6_addr_is_multicast(&addr)) {
        if (netif_set_opt(iface, NETOPT_IPV6_GROUP, 0, &addr,
                          sizeof(addr)) < 0) {
            printf("error: unable to join IPv6 multicast group\n");
            return 1;
        }
    }
    else {
        if (type == _ANYCAST) {
            flags |= GNRC_NETIF_IPV6_ADDRS_FLAGS_ANYCAST;
        }
        flags |= (prefix_len << 8U);
        if (netif_set_opt(iface, NETOPT_IPV6_ADDR, flags, &addr,
                          sizeof(addr)) < 0) {
            printf("error: unable to add IPv6 address\n");
            return 1;
        }
    }

    printf("success: added %s/%d to interface ", addr_str, prefix_len);
    printf("\n");

    return 0;

}

static void *emcute_thread(void *arg)
{
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL;    /* should never be reached */
}

static void on_pub(const emcute_topic_t *topic, void *data, size_t len)
{
    char *in = (char *)data;

    printf("### got publication for topic '%s' [%i] ###\n",
           topic->name, (int)topic->id);
    for (size_t i = 0; i < len; i++) {
        printf("%c", in[i]);
    }
    puts("");
}

static unsigned get_qos(const char *str)
{
    int qos = atoi(str);
    switch (qos) {
        case 1:     return EMCUTE_QOS_1;
        case 2:     return EMCUTE_QOS_2;
        default:    return EMCUTE_QOS_0;
    }
}

static int cmd_con(int argc, char **argv)
{
    sock_udp_ep_t gw = { .family = AF_INET6, .port = CONFIG_EMCUTE_DEFAULT_PORT };
    char *topic = NULL;
    char *message = NULL;
    size_t len = 0;

    if (argc < 2) {
        printf("usage: %s <ipv6 addr> [port] [<will topic> <will message>]\n",
                argv[0]);
        return 1;
    }

    /* parse address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, argv[1]) == NULL) {
        printf("error parsing IPv6 address\n");
        return 1;
    }

    if (argc >= 3) {
        gw.port = atoi(argv[2]);
    }
    if (argc >= 5) {
        topic = argv[3];
        message = argv[4];
        len = strlen(message);
    }

    if (emcute_con(&gw, true, topic, message, len, 0) != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", argv[1], (int)gw.port);
        return 1;
    }
    printf("Successfully connected to gateway at [%s]:%i\n",
           argv[1], (int)gw.port);

    return 0;
}

static int cmd_discon(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int res = emcute_discon();
    if (res == EMCUTE_NOGW) {
        puts("error: not connected to any broker");
        return 1;
    }
    else if (res != EMCUTE_OK) {
        puts("error: unable to disconnect");
        return 1;
    }
    puts("Disconnect successful");
    return 0;
}

static int cmd_pub(int argc, char **argv)
{
    emcute_topic_t topic;
    unsigned flags = EMCUTE_QOS_0;

    if (argc < 2) {
        printf("usage: %s <topic name>  [QoS level]\n", argv[0]);
        return 1;
    }

    /* parse QoS level */
    if (argc >= 4) {
        flags |= get_qos(argv[3]);
    }

    printf("pub with topic: %s and name %s and flags 0x%02x\n", argv[1], argv[2], (int)flags);

    /* step 1: get topic id */
    topic.name = argv[1];
    if (emcute_reg(&topic) != EMCUTE_OK) {
        puts("error: unable to obtain topic ID");
        return 1;
    }

    /*json structure*/
    char json[128];  

    /*Sensor-Actuator Parameters Initialization*/
    int sample_temp = 0;
    int temp = 0;
	int sample_light = 0;
    int light = 0;
	char temp_status[50];
	char light_status[50];
	gpio_init(GPIO_PIN(2,7),GPIO_OUT);
	gpio_init(GPIO_PIN(0,9),GPIO_OUT);
	gpio_init(GPIO_PIN(0,8),GPIO_OUT);
	gpio_init(GPIO_PIN(1,6),GPIO_OUT);
	
	// Get date and time 
	char   todayDateStr[100];
    time_t rawtime;
    struct tm *timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    strftime(todayDateStr, strlen("DD-MMM-YYYY HH:MM")+1,"%d-%b-%Y %H:%M",timeinfo);


	
    
/* Sample continously the ADC line and publish sensor data on given topic*/
    while (1) {
		
		xtimer_ticks32_t last = xtimer_now(); 
    
	// Takes the sample sensor data
        //Temp
        sample_temp = adc_sample(ADC_LINE(0), ADC_RES);
        temp = adc_util_map(sample_temp, ADC_RES, 10, 100);
		// Light
		sample_light = adc_sample(ADC_LINE(1), ADC_RES);
        light = adc_util_map(sample_light, ADC_RES, 10, 100);

		
		 argv[3] = "1"; 
		 
		// Actuator Activation Based on Sensor Values
		
		if(sample_temp>3400){
			gpio_set(GPIO_PIN(0,9));
			strcpy(temp_status, "Temp is High");
		} else 
		{
			gpio_clear(GPIO_PIN(0,9));
			strcpy(temp_status, "Temp is Normal");
		}
		if(sample_light>4000){
			gpio_clear(GPIO_PIN(2,7));
			gpio_clear(GPIO_PIN(1,6));
			gpio_set(GPIO_PIN(0,8));
			strcpy(light_status, "Light is High");
		} else if (sample_light<3000 && sample_light>1000){
			gpio_clear(GPIO_PIN(0,8));
			gpio_clear(GPIO_PIN(1,6));
			gpio_set(GPIO_PIN(2,7));
			strcpy(light_status, "Light is Normal");
		} else{
			gpio_clear(GPIO_PIN(0,8));
			gpio_clear(GPIO_PIN(2,7));
			gpio_set(GPIO_PIN(1,6));	
			strcpy(light_status, "Light is Low");
		}
		// fills the json document		
		sprintf(json, "{\"id\": \"%d\", \"datetime\": \"%s\", \"tempreture\": %i,\"tempstatus\": %s, \"lightdens\": %i,\"lightstatus\": %s}",
                  atoi(argv[3]), todayDateStr, temp,temp_status, light,light_status);                  
      argv[2] = json;  
	  /* step 2: publish data */
      if (emcute_pub(&topic, argv[2], strlen(argv[2]), flags) != EMCUTE_OK) {
        printf("error: unable to publish data to topic '%s [%i]'\n",
                topic.name, (int)topic.id);
        return 1;
      }

      printf("Published %i bytes to topic '%s  [%i]'\n",
            (int)strlen(argv[2]), topic.name, topic.id);

      xtimer_periodic_wakeup(&last, DELAY);
// it sleeps for five seconds
      xtimer_sleep(5);            
   }
    return 0;
}

static int cmd_sub(int argc, char **argv)
{
    unsigned flags = EMCUTE_QOS_0;

    if (argc < 2) {
        printf("usage: %s <topic name> [QoS level]\n", argv[0]);
        return 1;
    }

    if (strlen(argv[1]) > TOPIC_MAXLEN) {
        puts("error: topic name exceeds maximum possible size");
        return 1;
    }
    if (argc >= 3) {
        flags |= get_qos(argv[2]);
    }

    /* find empty subscription slot */
    unsigned i = 0;
    for (; (i < NUMOFSUBS) && (subscriptions[i].topic.id != 0); i++) {}
    if (i == NUMOFSUBS) {
        puts("error: no memory to store new subscriptions");
        return 1;
    }

    subscriptions[i].cb = on_pub;
    strcpy(topics[i], argv[1]);
    subscriptions[i].topic.name = topics[i];
    if (emcute_sub(&subscriptions[i], flags) != EMCUTE_OK) {
        printf("error: unable to subscribe to %s\n", argv[1]);
        return 1;
    }

    printf("Now subscribed to %s\n", argv[1]);
    return 0;
}

static int cmd_unsub(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage %s <topic name>\n", argv[0]);
        return 1;
    }

    /* find subscriptions entry */
    for (unsigned i = 0; i < NUMOFSUBS; i++) {
        if (subscriptions[i].topic.name &&
            (strcmp(subscriptions[i].topic.name, argv[1]) == 0)) {
            if (emcute_unsub(&subscriptions[i]) == EMCUTE_OK) {
                memset(&subscriptions[i], 0, sizeof(emcute_sub_t));
                printf("Unsubscribed from '%s'\n", argv[1]);
            }
            else {
                printf("Unsubscription form '%s' failed\n", argv[1]);
            }
            return 0;
        }
    }

    printf("error: no subscription for topic '%s' found\n", argv[1]);
    return 1;
}

static int cmd_will(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage %s <will topic name> <will message content>\n", argv[0]);
        return 1;
    }

    if (emcute_willupd_topic(argv[1], 0) != EMCUTE_OK) {
        puts("error: unable to update the last will topic");
        return 1;
    }
    if (emcute_willupd_msg(argv[2], strlen(argv[2])) != EMCUTE_OK) {
        puts("error: unable to update the last will message");
        return 1;
    }

    puts("Successfully updated last will topic and message");
    return 0;
}


static const shell_command_t shell_commands[] = {
    {"con", "Start the station", cmd_con},
    { "discon", "disconnect from the current broker", cmd_discon },
    { "pub", "publish something", cmd_pub },
    { "sub", "subscribe topic", cmd_sub },
    { "unsub", "unsubscribe from topic", cmd_unsub },
    { "will", "register a last will", cmd_will },
    {NULL, NULL, NULL}};
	
int main(void)
{
    puts("TempLight Application");
	
   /* initialize the ADC line */
	for (unsigned i = 0; i < 2; i++) {
    if (adc_init(ADC_LINE(i)) < 0) {
        printf("Initialization of ADC_LINE(%u) failed\n", i);
        return 1;
    }
    else {
        printf("Successfully initialized ADC_LINE(%u)\n", i);
    }
	}
	
   /* the main thread needs a msg queue to be able to run `ping6`*/
    msg_init_queue(queue, ARRAY_SIZE(queue));

    /* initialize our subscription buffers */
    memset(subscriptions, 0, (NUMOFSUBS * sizeof(emcute_sub_t)));

    /* start the emcute thread */
    thread_create(stack, sizeof(stack), EMCUTE_PRIO, 0,
                  emcute_thread, NULL, "emcute");
    
/* Aggiunta temporanea*/

netif_add("4","2001:0db8:0:f101::5");

/*aggiunta temporanea fine*/

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}