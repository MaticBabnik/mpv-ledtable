/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <config.h>

#if HAVE_POSIX
#include <sys/ioctl.h>
#endif

#include <libswscale/swscale.h>

#include "options/m_config.h"
#include "config.h"
#include "osdep/terminal.h"
#include "osdep/io.h"
#include "osdep/CSerial/c_serial.h"
#include "vo.h"
#include "sub/osd.h"
#include "video/sws_utils.h"
#include "video/mp_image.h"

#define IMGFMT IMGFMT_RGB24

const uint8_t mpv_logo[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x11, 0x44, 0x48, 0x88, 0xaa, 0xab, 0xbb, 0xbb, 0xba, 0xba, 0x88, 0x84,
    0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x29, 0x99, 0xb9,
    0xb9, 0x7a, 0x85, 0x88, 0x48, 0x84, 0x89, 0x69, 0xb9, 0xbc, 0xcc, 0x99, 0x92, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0xba, 0xb8, 0x58, 0x61, 0x66, 0x16, 0x61, 0x66, 0x16, 0x61, 0x66, 0x16, 0x61, 0x67,
    0x37, 0xb9, 0xbc, 0xcc, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x5a, 0x8a, 0x62, 0x66, 0x16, 0x62,
    0x66, 0x16, 0x51, 0x55, 0x15, 0x61, 0x66, 0x26, 0x62, 0x66, 0x16, 0x61, 0x69, 0x69, 0xdd, 0xd4, 0x44, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x22, 0xa8, 0xa6, 0x26, 0x61, 0x65, 0x15, 0x40, 0x44, 0x04, 0x30, 0x33, 0x03, 0x30, 0x44, 0x04, 0x51, 0x56,
    0x26, 0x62, 0x65, 0x16, 0x96, 0x9c, 0xcc, 0x11, 0x10, 0x00, 0x11, 0x18, 0x78, 0x73, 0x76, 0x16, 0x51, 0x54, 0x04, 0x30,
    0x34, 0x14, 0x85, 0x8a, 0x9a, 0xa9, 0xa8, 0x58, 0x41, 0x44, 0x04, 0x62, 0x66, 0x26, 0x61, 0x6b, 0xab, 0x88, 0x80, 0x00,
    0x44, 0x49, 0x69, 0x61, 0x65, 0x15, 0x40, 0x43, 0x03, 0x52, 0x5c, 0xbc, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xcb, 0xc5,
    0x25, 0x40, 0x46, 0x26, 0x61, 0x67, 0x48, 0xcc, 0xc3, 0x33, 0x76, 0x77, 0x37, 0x51, 0x54, 0x04, 0x30, 0x44, 0x04, 0xba,
    0xbe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xeb, 0xab, 0x40, 0x45, 0x15, 0x62, 0x66, 0x16, 0xca, 0xc6, 0x76,
    0x87, 0x86, 0x27, 0x50, 0x54, 0x04, 0x30, 0x37, 0x47, 0xee, 0xed, 0xdd, 0x97, 0x9b, 0x9b, 0xa8, 0xa9, 0x69, 0xdd, 0xde,
    0xee, 0x63, 0x64, 0x04, 0x62, 0x66, 0x16, 0xa8, 0xa9, 0x99, 0x86, 0x86, 0x26, 0x40, 0x44, 0x04, 0x30, 0x38, 0x68, 0xee,
    0xeb, 0xab, 0x61, 0x66, 0x16, 0x61, 0x66, 0x16, 0xb9, 0xbe, 0xee, 0x86, 0x84, 0x04, 0x61, 0x66, 0x16, 0x96, 0x9a, 0xaa,
    0x86, 0x86, 0x16, 0x40, 0x44, 0x04, 0x30, 0x38, 0x58, 0xee, 0xed, 0xde, 0xca, 0xc6, 0x16, 0x61, 0x6a, 0x8a, 0xdd, 0xde,
    0xee, 0x85, 0x83, 0x03, 0x61, 0x66, 0x16, 0x96, 0x9a, 0xaa, 0x76, 0x76, 0x26, 0x40, 0x44, 0x04, 0x30, 0x35, 0x25, 0xdd,
    0xdd, 0xde, 0xee, 0xeb, 0xab, 0xcb, 0xce, 0xee, 0xdd, 0xed, 0xdd, 0x52, 0x54, 0x04, 0x61, 0x66, 0x16, 0xa7, 0xa9, 0x99,
    0x65, 0x67, 0x37, 0x40, 0x44, 0x04, 0x40, 0x43, 0x03, 0x97, 0x9e, 0xee, 0xee, 0xee, 0xde, 0xdd, 0xee, 0xee, 0xee, 0xe9,
    0x79, 0x30, 0x34, 0x04, 0x62, 0x66, 0x16, 0xb9, 0xb6, 0x66, 0x33, 0x37, 0x47, 0x51, 0x54, 0x04, 0x40, 0x44, 0x04, 0x40,
    0x48, 0x68, 0xcc, 0xcd, 0xdd, 0xdd, 0xdc, 0xcc, 0x86, 0x84, 0x04, 0x30, 0x45, 0x15, 0x61, 0x67, 0x37, 0xba, 0xb2, 0x22,
    0x00, 0x06, 0x46, 0x72, 0x74, 0x04, 0x40, 0x44, 0x04, 0x40, 0x43, 0x03, 0x51, 0x56, 0x36, 0x63, 0x64, 0x15, 0x30, 0x33,
    0x04, 0x40, 0x46, 0x26, 0x61, 0x6a, 0x8a, 0x77, 0x70, 0x00, 0x00, 0x01, 0x11, 0x75, 0x76, 0x16, 0x40, 0x44, 0x04, 0x40,
    0x44, 0x04, 0x30, 0x43, 0x03, 0x30, 0x33, 0x04, 0x40, 0x44, 0x04, 0x61, 0x66, 0x16, 0x84, 0x8a, 0xaa, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x32, 0x37, 0x48, 0x61, 0x64, 0x04, 0x40, 0x44, 0x04, 0x40, 0x44, 0x04, 0x40, 0x44, 0x04, 0x40, 0x46,
    0x16, 0x61, 0x67, 0x48, 0xba, 0xb3, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x23, 0x75, 0x77, 0x37, 0x51,
    0x65, 0x05, 0x40, 0x54, 0x05, 0x51, 0x55, 0x15, 0x61, 0x66, 0x26, 0x95, 0x9a, 0x9a, 0x22, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x15, 0x45, 0x74, 0x77, 0x37, 0x72, 0x76, 0x27, 0x72, 0x77, 0x37, 0x85, 0x89,
    0x79, 0x66, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,
    0x25, 0x55, 0x76, 0x78, 0x78, 0x87, 0x87, 0x77, 0x55, 0x52, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define TABLE_W 20
#define TABLE_H 20

#define SERIAL_SEQ_LENGTH 3
const char *serial_start_sequence = "!BF";
const char *serial_end_sequence = "!EF";

int ledtable_serial_size = 606;

struct vo_ledtable_opts
{
    char *port;
};

#define OPT_BASE_STRUCT struct vo_ledtable_opts

static const struct m_sub_options vo_ledtable_conf = {
    .opts = (const m_option_t[]){
        {"vo-ledtable-port", OPT_STRING(port)},
        {0}},

#ifdef _WIN32
    .defaults = &(const struct vo_ledtable_opts){.port = "COM 0"},
#else
    .defaults = &(const struct vo_ledtable_opts){.port = "/dev/ttyUSB0"},
#endif

    .size = sizeof(struct vo_ledtable_opts),
};

struct priv
{
    uint8_t *serialData;
    struct c_serial_port *cserial;
    struct vo_ledtable_opts *opts;
    int swidth, sheight;
    size_t buffer_size;
    struct mp_image *frame;
    struct mp_rect src;
    struct mp_rect dst;
    struct mp_sws_context *sws;
};

#ifdef LEDTABLE_DEBUG
void printSerialBuf(struct priv *p)
{
    fprintf(stderr, "START: %c%c%c\n", p->serialData[0], p->serialData[1], p->serialData[2]);
    for (int i = 3; i < 603; i++)
    {
        fprintf(stderr, "%x", p->serialData[i]);
        if (i - 3 % 30 == 0)
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "END: %c%c%c\n", p->serialData[603], p->serialData[604], p->serialData[605]);
}
#endif

static int reconfig(struct vo *vo, struct mp_image_params *params)
{
    struct priv *p = vo->priv;

    vo->dwidth = TABLE_W;
    vo->dheight = TABLE_H;

    struct mp_osd_res osd;
    vo_get_src_dst_rects(vo, &p->src, &p->dst, &osd);
    p->swidth = p->dst.x1 - p->dst.x0;
    p->sheight = p->dst.y1 - p->dst.y0;

    p->sws->src = *params;
    p->sws->dst = (struct mp_image_params){
        .imgfmt = IMGFMT,
        .w = p->swidth,
        .h = p->sheight,
        .p_w = 1,
        .p_h = 1,
    };

    if (p->frame)
        talloc_free(p->frame);
    p->frame = mp_image_alloc(IMGFMT, p->swidth, p->sheight);
    if (!p->frame)
        return -1;

    if (mp_sws_reinit(p->sws) < 0)
        return -1;

    vo->want_redraw = true;
    return 0;
}

static void draw_image(struct vo *vo, mp_image_t *mpi)
{
    struct priv *p = vo->priv;
    struct mp_image src = *mpi;

    // todo: add an option to fill the whole table

    mp_sws_scale(p->sws, p->frame, &src);
    talloc_free(mpi);
}

static void convert_frame(struct priv *p)
{
    const unsigned char *plane = p->frame->planes[0];
    const int stride = p->frame->stride[0];

    bool wroteFullByte = true;
    uint8_t currentByte = 0x00;
    int index = 3;

    for (int x = 0; x < 20; x++)
    {
        for (int y = 0; y < 20; y++)
        {
            const unsigned char *pixel = plane + y * stride + x * 3;

            uint8_t r = (*pixel++) & 0xF0;
            uint8_t g = (*pixel++) & 0xF0;
            uint8_t b = (*pixel) & 0xF0;

            if (wroteFullByte)
            {
                p->serialData[index] = r | g >> 4;
                currentByte = b;
                index++;
                wroteFullByte = false;
            }
            else
            {
                p->serialData[index] = currentByte | r >> 4;
                p->serialData[index + 1] = g | b >> 4;
                index += 2;
                wroteFullByte = true;
            }
        }
    }
}

static void flip_page(struct vo *vo)
{
    struct priv *p = vo->priv;

    if (vo->dwidth != TABLE_W || vo->dheight != TABLE_H) // prolly useless
        reconfig(vo, vo->params);

    convert_frame(p);

#ifdef LEDTABLE_DEBUG
    printSerialBuf(p);
#endif

    int status = c_serial_write_data(p->cserial, p->serialData, &ledtable_serial_size);
    if (status != CSERIAL_OK)
    {
        fprintf(stderr, "Error writing  fprintf(stderr,  serial: %d\n", status);
    }
}

static void uninit(struct vo *vo)
{
    struct priv *p = vo->priv;

    c_serial_close(p->cserial);

    if (p->frame)
        talloc_free(p->frame);
}

static int preinit(struct vo *vo)
{

    struct priv *p = vo->priv;

    p->serialData = malloc(ledtable_serial_size);

    //fill the buffer
    memcpy(p->serialData, serial_start_sequence, 3);
    memcpy(p->serialData + SERIAL_SEQ_LENGTH, mpv_logo, 600); //todo: --vo-ledtable-nologo option?
    memcpy(p->serialData + (ledtable_serial_size - SERIAL_SEQ_LENGTH), serial_end_sequence, 3);

    p->opts = mp_get_config_group(vo, vo->global, &vo_ledtable_conf);
    p->sws = mp_sws_alloc(vo);
    p->sws->log = vo->log;
    mp_sws_enable_cmdline_opts(p->sws, vo->global);

    struct vo_ledtable_opts *opts = p->opts;
    fprintf(stderr, "Using serial port: %s\n", opts->port);

    c_serial_set_global_log_function(c_serial_stderr_log_function);

    if (c_serial_new(&p->cserial, NULL) < 0)
    {
        fprintf(stderr, "ERROR: Unable to create new serial port\n");
        return 1;
    }

    if (c_serial_set_port_name(p->cserial, opts->port) < 0)
    {
        fprintf(stderr, "ERROR: can't set port name\n");
    }

    c_serial_set_baud_rate(p->cserial, 576000);
    c_serial_set_data_bits(p->cserial, CSERIAL_BITS_8);
    c_serial_set_stop_bits(p->cserial, CSERIAL_STOP_BITS_1);
    c_serial_set_parity(p->cserial, CSERIAL_PARITY_NONE);
    c_serial_set_flow_control(p->cserial, CSERIAL_FLOW_NONE);

    fprintf(stderr, "Baud rate is %d\n", c_serial_get_baud_rate(p->cserial));

    c_serial_set_serial_line_change_flags(p->cserial, CSERIAL_LINE_FLAG_ALL);

    int status = c_serial_open(p->cserial);
    if (status != 0)
    {
        fprintf(stderr, "ERROR: Can't open serial port! Make sure \"%s\" exists and is available\n", opts->port);
        return 1;
    }

    status = c_serial_write_data(p->cserial, p->serialData, &ledtable_serial_size);
    if (status != 0)
        fprintf(stderr, "ERROR: Can't clear the display\n");
    return 0;
}

static int query_format(struct vo *vo, int format)
{
    return format == IMGFMT;
}

static int control(struct vo *vo, uint32_t request, void *data)
{
    return VO_NOTIMPL;
}

const struct vo_driver video_out_ledtable = {
    .name = "ledtable",
    .description = "20x20 led table",
    .preinit = preinit,
    .query_format = query_format,
    .reconfig = reconfig,
    .control = control,
    .draw_image = draw_image,
    .flip_page = flip_page,
    .uninit = uninit,
    .priv_size = sizeof(struct priv),
    .global_opts = &vo_ledtable_conf,
};