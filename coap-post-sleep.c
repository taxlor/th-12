#include <string.h>

/* contiki */
#include "contiki.h"
#include "contiki-net.h"
#include "net/rpl/rpl.h"

/* coap */
#if WITH_COAP == 3
#include "er-coap-03-engine.h"
#elif WITH_COAP == 6
#include "er-coap-06-engine.h"
#elif WITH_COAP == 7
#include "er-coap-07-engine.h"
#else
#error "CoAP version defined by WITH_COAP not implemented"
#endif

/* mc1322x */
#include "mc1322x.h"
#include "config.h"

/* debug */
#define DEBUG DEBUG_FULL
#include "net/uip-debug.h"

#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)

PROCESS(lantern, "Lantern");
AUTOSTART_PROCESSES(&lantern);

#define TH12_CONFIG_PAGE 0x1D000 /* nvm page where conf will be stored */
#define TH12_CONFIG_VERSION 1
#define TH12_CONFIG_MAGIC 0x5448

/* th12 config */
typedef struct {
  uint16_t magic; /* th12 magic number 0x5448 */
  uint16_t version; /* th12 config version number */
  uint8_t red; 
  uint8_t green; 
  uint8_t blue; 
} TH12Config;

static TH12Config th12_cfg;

void 
th12_config_set_default(TH12Config *c) 
{
  c->magic = TH12_CONFIG_MAGIC;
  c->version = TH12_CONFIG_VERSION;
  c->red = 128;
  c->green = 128;
  c->blue = 128;
}

/* write out config to flash */
void th12_config_save(TH12Config *c) {
	nvmErr_t err;
	err = nvm_erase(gNvmInternalInterface_c, mc1322x_config.flags.nvmtype, 1 << TH12_CONFIG_PAGE/4096);
	err = nvm_write(gNvmInternalInterface_c, mc1322x_config.flags.nvmtype, (uint8_t *)c, TH12_CONFIG_PAGE, sizeof(TH12Config));
}

/* load the config from flash to the pass conf structure */
void th12_config_restore(TH12Config *c) {
	nvmErr_t err;
	nvmType_t type;
	if (mc1322x_config.flags.nvmtype == 0) { 
	  nvm_detect(gNvmInternalInterface_c, &type); 
	  mc1322x_config.flags.nvmtype = type;
	}
	err = nvm_read(gNvmInternalInterface_c, mc1322x_config.flags.nvmtype, c, TH12_CONFIG_PAGE, sizeof(TH12Config));
}

/* check the flash for magic number and proper version */
int th12_config_valid(TH12Config *c) {
	if (c->magic == TH12_CONFIG_MAGIC &&
	    c->version == TH12_CONFIG_VERSION) {
		return 1;
	} else {
#if DEBUG
		if (c->magic != TH12_CONFIG_MAGIC) { PRINTF("th12 config bad magic %04x\n\r", c->magic); }
		if (c->version != TH12_CONFIG_MAGIC) { PRINTF("th12 config bad version %04x\n\r", c->version); }
#endif
		return 0;
	}
}

void th12_config_print(void) {
	PRINTF("lantern config:\n\r");
	PRINTF("  red:    %d\n\r", th12_cfg.red);
	PRINTF("  green:  %d\n\r", th12_cfg.green);
	PRINTF("  blue:   %d\n\r", th12_cfg.blue);
	PRINTF("\n\r");	
}

int
ipaddr_sprint(char *s, const uip_ipaddr_t *addr)
{
  uint16_t a;
  unsigned int i;
  int f;
  int n;
  n = 0;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
	n += sprintf(&s[n], "::");
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
	n += sprintf(&s[n], ":");
      }
      n += sprintf(&s[n], "%x", a); 
    }
  }
  return n;
}

RESOURCE(config, METHOD_GET | METHOD_POST , "config", "title=\"Config parameters\";rt=\"Data\"");

void
config_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  void *param;
  uip_ipaddr_t *new_addr;
  const char *pstr;
  size_t len = 0;

  if ((len = REST.get_query_variable(request, "param", &pstr))) {
    if (strncmp(pstr, "red", len) == 0) {
      param = &th12_cfg.red;
    } else if(strncmp(pstr, "green", len) == 0) {
      param = &th12_cfg.green;
    } else if(strncmp(pstr, "blue", len) == 0) {
      param = &th12_cfg.blue;
    } else {
      goto bad;
    }
  } else {
    goto bad;
  }

  if (REST.get_method_type(request) == METHOD_POST) {
    const uint8_t *new;
    REST.get_request_payload(request, &new);
    *(uint8_t *)param = (uint8_t)atoi(new);
    th12_config_save(&th12_cfg);
    

    //set duty ratio properly here

  } else { /* GET */
    uint8_t n;
    n = sprintf(buffer, "%d", *(uint8_t *)param);
    REST.set_response_payload(response, buffer, n);
  }

  return;

bad:
  REST.set_response_status(response, REST.status.BAD_REQUEST);

}

PROCESS_THREAD(lantern, ev, data)
{

  PROCESS_BEGIN();

  /* Initialize the REST engine. */
  /* You need one of these */
  /* Otherwise things will "work" but fail in insidious ways */
  rest_init_engine();

  rplinfo_activate_resources();
  rest_activate_resource(&resource_config);

  PRINTF("Taylor's Awesome Lantern!!1!\n\r");

#ifdef RPL_LEAF_ONLY
  PRINTF("RPL LEAF ONLY\n\r");
#endif

  /* boost enable pin */
  GPIO->FUNC_SEL.KBI2 = 3;
  GPIO->PAD_DIR_SET.KBI2 = 1;
  gpio_set(KBI2);

  /* get the config from flash or make a default config */
  th12_config_restore(&th12_cfg);
  if (!th12_config_valid(&th12_cfg)) {
    th12_config_set_default(&th12_cfg);
    th12_config_save(&th12_cfg);
  }
  th12_config_print();

  while(1) {

    PROCESS_WAIT_EVENT();

    /* main loop-ish things could go here */

  }

  PROCESS_END();

}
