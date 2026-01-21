/* This stage configures the OS to support effective preferred busy
   polling, allowing for significantly improved network stack (XDP)
   performance if enabled. */

#include "configure.h"

#define NAME "sysfs-poll"

#define VERY_HIGH_VAL 1000000000U

static char const setting_napi_defer_hard_irqs[] = "napi_defer_hard_irqs";

static char const setting_gro_flush_timeout[] = "gro_flush_timeout";

static char const setting_irq_suspend_timeout[] = "irq_suspend_timeout";

static int
enabled( config_t * config ) {
    return !strcmp( config->tiles.xdp.poll_mode, "pref_busy" );
}

static void
init_perm ( fd_caps_ctx * caps,
            config_t    * FD_PARAM_UNUSED ) {
    fd_caps_check_capability( caps, NAME, CAP_NET_ADMIN, "configure preferred busy polling via `/sys/class/net/*/{napi_defer_hard_irqs, gro_flush_timeout, irq_suspend_timeout}`" );
}

static void
sysfs_net_set( char const * device,
               char const * setting,
               uint         value ) {
    char path[ PATH_MAX ];
    fd_cstr_printf_check( path, PATH_MAX, NULL, "/sys/class/net/%s/%s", device, setting );
    FD_LOG_NOTICE(( "RUN: `echo \"%u\" > %s`", value, path ));
    write_uint_file( path, value );
}

static void
init( config_t * config ) {
    sysfs_net_set( config->tiles.net.interface, setting_napi_defer_hard_irqs, VERY_HIGH_VAL );
    sysfs_net_set( config->tiles.net.interface, setting_gro_flush_timeout,    VERY_HIGH_VAL );
    sysfs_net_set( config->tiles.net.interface, setting_irq_suspend_timeout,  VERY_HIGH_VAL );
}

static void
fini( config_t * config,
      int        pre_init FD_PARAM_UNUSED ) {
    sysfs_net_set( config->tiles.net.interface, setting_napi_defer_hard_irqs, 0U );
    sysfs_net_set( config->tiles.net.interface, setting_gro_flush_timeout,    0U );
    sysfs_net_set( config->tiles.net.interface, setting_irq_suspend_timeout,  0U );
}

static configure_result_t
check( config_t * config ) {
    static char const enoent_msg[] = "Interface not found or XDP preferred busy polling not supported:";

    char path[ PATH_MAX ];
    fd_cstr_printf_check( path, PATH_MAX, NULL, "/sys/class/net/%s/%s", config->tiles.net.interface, setting_napi_defer_hard_irqs );
    if( read_uint_file( path, enoent_msg ) != VERY_HIGH_VAL ) {
        NOT_CONFIGURED("Setting napi_defer_hard_irqs failed.");
    }

    fd_cstr_printf_check( path, PATH_MAX, NULL, "/sys/class/net/%s/%s", config->tiles.net.interface, setting_gro_flush_timeout );
    if( read_uint_file( path, enoent_msg ) != VERY_HIGH_VAL ) {
        NOT_CONFIGURED("Setting gro_flush_timeout failed.");
    }
    
    fd_cstr_printf_check( path, PATH_MAX, NULL, "/sys/class/net/%s/%s", config->tiles.net.interface, setting_irq_suspend_timeout );
    if( read_uint_file( path, enoent_msg ) != VERY_HIGH_VAL ) {
        NOT_CONFIGURED("Setting irq_suspend_timeout failed.");
    }

    CONFIGURE_OK();
}

configure_stage_t fd_cfg_sysfs_poll = {
    .name      = NAME,
    .enabled   = enabled,
    .init_perm = init_perm,
    .fini_perm = init_perm,
    .init      = init,
    .fini      = fini,
    .check     = check,
};
