#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>

/*
 * Function prototypes for the adler driver.
 */
static int adler_open(devminor_t minor, int access, endpoint_t user_endpt);
static int adler_close(devminor_t minor);
static ssize_t adler_write(devminor_t minor, u64_t position, endpoint_t endpt,
    cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);
static ssize_t adler_read(devminor_t minor, u64_t position, endpoint_t endpt,
    cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int, int);
static int lu_state_restore(void);

/* Entry points to the adler driver. */
static struct chardriver hello_tab =
{
    .cdr_open	= adler_open,
    .cdr_close	= adler_close,
    .cdr_write	= adler_write,
    .cdr_read = adler_read
};

/** State variable to count the number of times the device has been opened.
 * Note that this is not the regular type of open counter: it never decreases.
 */
static u32_t A, B;

static void adler(char* buffer, size_t buffer_size)
{
    int adler_mod = 65521;
    int i = 0;

    while (i < buffer_size) {
        A += buffer[i++];
        B += A;
        A %= adler_mod;
        B %= adler_mod;
    }

    return (B << 16) | A;
}

static int adler_open(devminor_t UNUSED(minor), int UNUSED(access),
    endpoint_t UNUSED(user_endpt))
{
    return OK;
}

static int adler_close(devminor_t UNUSED(minor))
{
    return OK;
}

static ssize_t adler_write(devminor_t UNUSED(minor), u64_t position,
    endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
    cdev_id_t UNUSED(id))
{

    char buffer[4096];
    size_t buff_size = 0;
    int ret;
    u64_t dev_size;
    size_t offset = 0;
    char* ptr;

    while (offset < size) {

        buff_size = size - offset > 4096 ? 4096 : size - offset;

        ptr = buffer;
        if ((ret = sys_safecopyfrom(endpt, grant, 0, (vir_bytes) ptr, buff_size)) != OK)
            return ret;

        adler(ptr, buff_size);
        offset += buff_size;
    }
    /* Return the number of bytes read. */
    return size;
}

static ssize_t adler_read(devminor_t minor, u64_t position, endpoint_t endpt,
    cp_grant_id_t grant, size_t size, int flags, cdev_id_t id)
{
    char ptr[9];
    int ret;
    u32_t checksum;
    char checksum_str[9];
    size_t ch_str_len;

    if (size < 8) {
        return EINVAL;
    }

    size = 8;

    memset(ptr, 0 , 9);
    memset(checksum_str, 0, 9);

    checksum = (B << 16) | A;
    sprintf(checksum_str, "%x", checksum);
    ch_str_len = strlen(ch_str_len);
    memcpy(ptr + 8  - ch_str_len, checksum_str, ch_str_len);
    A = 1;
    B = 0;

    /* Copy the requested part to the caller. */
    ptr = buf + (size_t)position;
    if ((ret = sys_safecopyto(endpt, grant, 0, (vir_bytes) ptr, size)) != OK)
        return ret;

    /* Return the number of bytes read. */
    return size;
}

static int sef_cb_lu_state_save(int UNUSED(state), int UNUSED(flags)) {
/* Save the state. */
    ds_publish_u32("A", A, DSF_OVERWRITE);
    ds_publish_u32("B", B, DSF_OVERWRITE);
    return OK;
}

static int lu_state_restore() {
/* Restore the state. */
    u32_t value_A;
    u32_t value_B;
    ds_retrieve_u32("A", &value_A);
    ds_delete_u32("A");
    ds_retrieve_u32("B", &value_B);
    ds_delete_u32("B");
    A = value_A;
    B = value_B;

    return OK;
}

static void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
/* Initialize the hello driver. */
    int do_announce_driver = TRUE;

    A = 1;
    B = 0;
    switch(type) {
        case SEF_INIT_FRESH:
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;
        break;

        case SEF_INIT_RESTART:
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        chardriver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

int main(void)
{
    /*
     * Perform initialization.
     */
    sef_local_startup();

    /*
     * Run the main loop.
     */
    chardriver_task(&hello_tab);
    return OK;
}
