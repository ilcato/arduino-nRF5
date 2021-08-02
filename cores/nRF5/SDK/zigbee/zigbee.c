#include "sdk_config.h"
#include "zboss_api.h"
#include "zboss_api_addons.h"
#include "zb_mem_config_med.h"
#include "zb_ha_dimmable_light.h"
#include "zb_error_handler.h"
#include "zb_nrf52_internal.h"
#include "zigbee_helpers.h"

#define MAX_CHILDREN                      10                                    /**< The maximum amount of connected devices. Setting this value to 0 disables association to this device.  */
#define IEEE_CHANNEL_MASK                 (1l << 15)                /**< Scan only one, predefined channel to find the coordinator. */
#define HA_DIMMABLE_LIGHT_ENDPOINT        10                                    /**< Device endpoint, used to receive light controlling commands. */
#define ERASE_PERSISTENT_CONFIG           ZB_FALSE                              /**< Do not erase NVRAM to save the network parameters after device reboot or power-off. */

/* Basic cluster attributes initial values. */
#define BULB_INIT_BASIC_APP_VERSION       01                                    /**< Version of the application software (1 byte). */
#define BULB_INIT_BASIC_STACK_VERSION     10                                    /**< Version of the implementation of the Zigbee stack (1 byte). */
#define BULB_INIT_BASIC_HW_VERSION        11                                    /**< Version of the hardware of the device (1 byte). */
#define BULB_INIT_BASIC_MANUF_NAME        "Nordic"                              /**< Manufacturer name (32 bytes). */
#define BULB_INIT_BASIC_MODEL_ID          "Dimable_Light_v0.1"                  /**< Model number assigned by manufacturer (32-bytes long string). */
#define BULB_INIT_BASIC_DATE_CODE         "20180416"                            /**< First 8 bytes specify the date of manufacturer of the device in ISO 8601 format (YYYYMMDD). The rest (8 bytes) are manufacturer specific. */
#define BULB_INIT_BASIC_POWER_SOURCE      ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE   /**< Type of power sources available for the device. For possible values see section 3.2.2.2.8 of ZCL specification. */
#define BULB_INIT_BASIC_LOCATION_DESC     "Office desk"                         /**< Describes the physical location of the device (16 bytes). May be modified during commisioning process. */
#define BULB_INIT_BASIC_PH_ENV            ZB_ZCL_BASIC_ENV_UNSPECIFIED          /**< Describes the type of physical environment. For possible values see section 3.2.2.2.10 of ZCL specification. */

/* Declare endpoint for Dimmable Light device with scenes. */
#define ZB_HA_DECLARE_LIGHT_EP(ep_name, ep_id, cluster_list)                         \
  ZB_ZCL_DECLARE_HA_DIMMABLE_LIGHT_SIMPLE_DESC(ep_name, ep_id,                       \
    ZB_HA_DIMMABLE_LIGHT_IN_CLUSTER_NUM, ZB_HA_DIMMABLE_LIGHT_OUT_CLUSTER_NUM);      \
  ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info## device_ctx_name,               \
                                     ZB_HA_DIMMABLE_LIGHT_REPORT_ATTR_COUNT);        \
  ZBOSS_DEVICE_DECLARE_LEVEL_CONTROL_CTX(cvc_alarm_info## device_ctx_name,           \
                                         ZB_HA_DIMMABLE_LIGHT_CVC_ATTR_COUNT);       \
  ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID,                   \
                              0,     \
                              NULL,                 \
                              ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t),\
                              cluster_list,                                          \
                              (zb_af_simple_desc_1_1_t*)&simple_desc_##ep_name,      \
                              ZB_HA_DIMMABLE_LIGHT_REPORT_ATTR_COUNT,                \
                              reporting_info## device_ctx_name,                      \
                              ZB_HA_DIMMABLE_LIGHT_CVC_ATTR_COUNT,                   \
                              cvc_alarm_info## device_ctx_name)


/* Main application customizable context. Stores all settings and static values. */
typedef struct
{
    zb_zcl_basic_attrs_ext_t         basic_attr;
    zb_zcl_identify_attrs_t          identify_attr;
    zb_zcl_scenes_attrs_t            scenes_attr;
    zb_zcl_groups_attrs_t            groups_attr;
    zb_zcl_on_off_attrs_ext_t        on_off_attr;
    zb_zcl_level_control_attrs_t     level_control_attr;
} bulb_device_ctx_t;


static bulb_device_ctx_t m_dev_ctx;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &m_dev_ctx.identify_attr.identify_time);


ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &m_dev_ctx.groups_attr.name_support);

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list,
                                  &m_dev_ctx.scenes_attr.scene_count,
                                  &m_dev_ctx.scenes_attr.current_scene,
                                  &m_dev_ctx.scenes_attr.current_group,
                                  &m_dev_ctx.scenes_attr.scene_valid,
                                  &m_dev_ctx.scenes_attr.name_support);

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(basic_attr_list,
                                     &m_dev_ctx.basic_attr.zcl_version,
                                     &m_dev_ctx.basic_attr.app_version,
                                     &m_dev_ctx.basic_attr.stack_version,
                                     &m_dev_ctx.basic_attr.hw_version,
                                     m_dev_ctx.basic_attr.mf_name,
                                     m_dev_ctx.basic_attr.model_id,
                                     m_dev_ctx.basic_attr.date_code,
                                     &m_dev_ctx.basic_attr.power_source,
                                     m_dev_ctx.basic_attr.location_id,
                                     &m_dev_ctx.basic_attr.ph_env,
                                     m_dev_ctx.basic_attr.sw_ver);

/* On/Off cluster attributes additions data */
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(on_off_attr_list,
                                      &m_dev_ctx.on_off_attr.on_off,
                                      &m_dev_ctx.on_off_attr.global_scene_ctrl,
                                      &m_dev_ctx.on_off_attr.on_time,
                                      &m_dev_ctx.on_off_attr.off_wait_time);

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list,
                                         &m_dev_ctx.level_control_attr.current_level,
                                         &m_dev_ctx.level_control_attr.remaining_time);

ZB_HA_DECLARE_DIMMABLE_LIGHT_CLUSTER_LIST(dimmable_light_clusters,
                                          basic_attr_list,
                                          identify_attr_list,
                                          groups_attr_list,
                                          scenes_attr_list,
                                          on_off_attr_list,
                                          level_control_attr_list);

ZB_HA_DECLARE_LIGHT_EP(dimmable_light_ep,
                       HA_DIMMABLE_LIGHT_ENDPOINT,
                       dimmable_light_clusters);

ZB_HA_DECLARE_DIMMABLE_LIGHT_CTX(dimmable_light_ctx,
                                 dimmable_light_ep);


static void (*bulbCB)(uint8_t brightness_level);

/**@brief Sets brightness of on-board LED
 *
 * @param[in] brightness_level Brightness level, allowed values 0 ... 255, 0 - turn off, 255 - full brightness
 */
static void light_bulb_onboard_set_brightness(zb_uint8_t brightness_level)
{
    bulbCB(brightness_level);
}


/**@brief Sets brightness of bulb luminous executive element
 *
 * @param[in] brightness_level Brightness level, allowed values 0 ... 255, 0 - turn off, 255 - full brightness
 */
static void light_bulb_set_brightness(zb_uint8_t brightness_level)
{
    light_bulb_onboard_set_brightness(brightness_level);
}

/**@brief Function for setting the light bulb brightness.
  *
  * @param[in]   new_level   Light bulb brightness value.
 */
static void level_control_set_value(zb_uint16_t new_level)
{
    ZB_ZCL_SET_ATTRIBUTE(HA_DIMMABLE_LIGHT_ENDPOINT,                                       
                         ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,            
                         ZB_ZCL_CLUSTER_SERVER_ROLE,                 
                         ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID, 
                         (zb_uint8_t *)&new_level,                                       
                         ZB_FALSE);                                  

    /* According to the table 7.3 of Home Automation Profile Specification v 1.2 rev 29, chapter 7.1.3. */
    if (new_level == 0)
    {
        zb_uint8_t value = ZB_FALSE;
        ZB_ZCL_SET_ATTRIBUTE(HA_DIMMABLE_LIGHT_ENDPOINT, 
                             ZB_ZCL_CLUSTER_ID_ON_OFF,    
                             ZB_ZCL_CLUSTER_SERVER_ROLE,  
                             ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                             &value,                        
                             ZB_FALSE);                   
    }
    else
    {
        zb_uint8_t value = ZB_TRUE;
        ZB_ZCL_SET_ATTRIBUTE(HA_DIMMABLE_LIGHT_ENDPOINT, 
                             ZB_ZCL_CLUSTER_ID_ON_OFF,    
                             ZB_ZCL_CLUSTER_SERVER_ROLE,  
                             ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                             &value,                        
                             ZB_FALSE);
    }

    light_bulb_set_brightness(new_level);
}


/**@brief Function for turning ON/OFF the light bulb.
 *
 * @param[in]   on   Boolean light bulb state.
 */
static void on_off_set_value(zb_bool_t on)
{

    ZB_ZCL_SET_ATTRIBUTE(HA_DIMMABLE_LIGHT_ENDPOINT, 
                         ZB_ZCL_CLUSTER_ID_ON_OFF,    
                         ZB_ZCL_CLUSTER_SERVER_ROLE,  
                         ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                         (zb_uint8_t *)&on,                        
                         ZB_FALSE);

    if (on)
    {
        level_control_set_value(m_dev_ctx.level_control_attr.current_level);
    }
    else
    {
        light_bulb_set_brightness(0U);
    }
}


/**@brief Function for initializing all clusters attributes.
 */
static void bulb_clusters_attr_init(void)
{
    /* Basic cluster attributes data */
    m_dev_ctx.basic_attr.zcl_version   = ZB_ZCL_VERSION;
    m_dev_ctx.basic_attr.app_version   = BULB_INIT_BASIC_APP_VERSION;
    m_dev_ctx.basic_attr.stack_version = BULB_INIT_BASIC_STACK_VERSION;
    m_dev_ctx.basic_attr.hw_version    = BULB_INIT_BASIC_HW_VERSION;

    /* Use ZB_ZCL_SET_STRING_VAL to set strings, because the first byte should
     * contain string length without trailing zero.
     *
     * For example "test" string wil be encoded as:
     *   [(0x4), 't', 'e', 's', 't']
     */
    ZB_ZCL_SET_STRING_VAL(m_dev_ctx.basic_attr.mf_name,
                          BULB_INIT_BASIC_MANUF_NAME,
                          ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_MANUF_NAME));

    ZB_ZCL_SET_STRING_VAL(m_dev_ctx.basic_attr.model_id,
                          BULB_INIT_BASIC_MODEL_ID,
                          ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_MODEL_ID));

    ZB_ZCL_SET_STRING_VAL(m_dev_ctx.basic_attr.date_code,
                          BULB_INIT_BASIC_DATE_CODE,
                          ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_DATE_CODE));

    m_dev_ctx.basic_attr.power_source = BULB_INIT_BASIC_POWER_SOURCE;

    ZB_ZCL_SET_STRING_VAL(m_dev_ctx.basic_attr.location_id,
                          BULB_INIT_BASIC_LOCATION_DESC,
                          ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_LOCATION_DESC));


    m_dev_ctx.basic_attr.ph_env = BULB_INIT_BASIC_PH_ENV;

    /* Identify cluster attributes data */
    m_dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

    /* On/Off cluster attributes data */
    m_dev_ctx.on_off_attr.on_off = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;

    m_dev_ctx.level_control_attr.current_level  = ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE;
    m_dev_ctx.level_control_attr.remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

    ZB_ZCL_SET_ATTRIBUTE(HA_DIMMABLE_LIGHT_ENDPOINT, 
                         ZB_ZCL_CLUSTER_ID_ON_OFF,    
                         ZB_ZCL_CLUSTER_SERVER_ROLE,  
                         ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                         (zb_uint8_t *)&m_dev_ctx.on_off_attr.on_off,                        
                         ZB_FALSE);                   

    ZB_ZCL_SET_ATTRIBUTE(HA_DIMMABLE_LIGHT_ENDPOINT,                                       
                         ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,            
                         ZB_ZCL_CLUSTER_SERVER_ROLE,                 
                         ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID, 
                         (zb_uint8_t *)&m_dev_ctx.level_control_attr.current_level,                                       
                         ZB_FALSE);                                  
}

/**@brief Function which tries to sleep down the MCU 
 *
 * Function which sleeps the MCU on the non-sleepy End Devices to optimize the power saving.
 * The weak definition inside the OSIF layer provides some minimal working template
 */
zb_void_t zb_osif_go_idle(zb_void_t)
{
    //TODO: implement your own logic if needed
    zb_osif_wait_for_event();
}

/**@brief Callback function for handling ZCL commands.
 *
 * @param[in]   bufid   Reference to Zigbee stack buffer used to pass received data.
 */
static zb_void_t zcl_device_cb(zb_bufid_t bufid)
{
    zb_uint8_t                       cluster_id;
    zb_uint8_t                       attr_id;
    zb_zcl_device_callback_param_t * p_device_cb_param = ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);

    /* Set default response value. */
    p_device_cb_param->status = RET_OK;

    switch (p_device_cb_param->device_cb_id)
    {
        case ZB_ZCL_LEVEL_CONTROL_SET_VALUE_CB_ID:
            level_control_set_value(p_device_cb_param->cb_param.level_control_set_value_param.new_value);
            break;

        case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
            cluster_id = p_device_cb_param->cb_param.set_attr_value_param.cluster_id;
            attr_id    = p_device_cb_param->cb_param.set_attr_value_param.attr_id;

            if (cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF)
            {
                uint8_t value = p_device_cb_param->cb_param.set_attr_value_param.values.data8;

                if (attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID)
                {
                    on_off_set_value((zb_bool_t) value);
                }
            }
            else if (cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
            {
                uint16_t value = p_device_cb_param->cb_param.set_attr_value_param.values.data16;

                if (attr_id == ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID)
                {
                    level_control_set_value(value);
                }
            }
            else
            {
                /* Other clusters can be processed here */
            }
            break;

        default:
            p_device_cb_param->status = RET_ERROR;
            break;
    }

}

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
    /* Update network status LED */
//    zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

    /* No application-specific behavior is required. Call default signal handler. */
    zigbee_default_signal_handler(bufid);

    if (bufid)
    {
        zb_buf_free(bufid);
    }
}


void regBulbCB(void (*aBulbCB)(uint8_t brightness_level)) {
    bulbCB = aBulbCB;
}  

void zigbeeinit(void)
{
    zb_ret_t       zb_err_code;
    zb_ieee_addr_t ieee_addr;

    /* Set Zigbee stack logging level and traffic dump subsystem. */
    ZB_SET_TRACE_LEVEL(ZIGBEE_TRACE_LEVEL);
    ZB_SET_TRACE_MASK(ZIGBEE_TRACE_MASK);
    ZB_SET_TRAF_DUMP_OFF();

    /* Initialize Zigbee stack. */
    ZB_INIT("led_bulb");

    /* Set device address to the value read from FICR registers. */
    zb_osif_get_ieee_eui64(ieee_addr);
    zb_set_long_address(ieee_addr);

    /* Set static long IEEE address. */
    zb_set_network_router_role(IEEE_CHANNEL_MASK);
    zb_set_max_children(MAX_CHILDREN);
    zigbee_erase_persistent_storage(ERASE_PERSISTENT_CONFIG);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));

    /* Initialize application context structure. */
    UNUSED_RETURN_VALUE(ZB_MEMSET(&m_dev_ctx, 0, sizeof(m_dev_ctx)));

    /* Register callback for handling ZCL commands. */
    ZB_ZCL_REGISTER_DEVICE_CB(zcl_device_cb);


}