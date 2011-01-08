/*
 * Copyright (c) 2010, LSI Corp.
 * All rights reserved.
 * Author : Manjunath Ranganathaiah
 * Support: freebsdraid@lsi.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the <ORGANIZATION> nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/tws/tws_cam.c,v 1.3 2007/05/09 04:16:32 mrangana Exp $
 */

#include <dev/raid/tws/tws.h>
#include <dev/raid/tws/tws_services.h>
#include <dev/raid/tws/tws_hdm.h>
#include <dev/raid/tws/tws_user.h>
#include <bus/cam/cam.h>
#include <bus/cam/cam_ccb.h>
#include <bus/cam/cam_sim.h>
#include <bus/cam/cam_xpt_sim.h>
#include <bus/cam/cam_debug.h>
#include <bus/cam/cam_periph.h>

#include <bus/cam/scsi/scsi_all.h>
#include <bus/cam/scsi/scsi_message.h>

static int tws_cam_depth=(TWS_MAX_REQS - TWS_RESERVED_REQS);
static char tws_sev_str[5][8]={"","ERROR","WARNING","INFO","DEBUG"};

static void  tws_action(struct cam_sim *sim, union ccb *ccb);
static void  tws_poll(struct cam_sim *sim);
static void tws_bus_scan_cb(struct cam_periph *periph, union ccb *ccb);
static void tws_scsi_complete(struct tws_request *req);



void tws_unmap_request(struct tws_softc *sc, struct tws_request *req);
int32_t tws_map_request(struct tws_softc *sc, struct tws_request *req);
int tws_bus_scan(struct tws_softc *sc);
int tws_cam_attach(struct tws_softc *sc);
void tws_cam_detach(struct tws_softc *sc);
void tws_reset(void *arg);

static void tws_reset_cb(void *arg);
static void tws_reinit(void *arg);
static int32_t tws_execute_scsi(struct tws_softc *sc, union ccb *ccb);
static void tws_freeze_simq(struct tws_softc *sc);
static void tws_release_simq(struct tws_softc *sc);
static void tws_dmamap_data_load_cbfn(void *arg, bus_dma_segment_t *segs,
                            int nseg, int error);
static void tws_fill_sg_list(struct tws_softc *sc, void *sgl_src,
                            void *sgl_dest, u_int16_t num_sgl_entries);
static void tws_err_complete(struct tws_softc *sc, u_int64_t mfa);
static void tws_scsi_err_complete(struct tws_request *req,
                                               struct tws_command_header *hdr);
static void tws_passthru_err_complete(struct tws_request *req,
                                               struct tws_command_header *hdr);


static void tws_timeout(void *arg);
static void tws_intr_attn_aen(struct tws_softc *sc);
static void tws_intr_attn_error(struct tws_softc *sc);
static void tws_intr_resp(struct tws_softc *sc);
void tws_intr(void *arg);
void tws_cmd_complete(struct tws_request *req);
void tws_aen_complete(struct tws_request *req);
int tws_send_scsi_cmd(struct tws_softc *sc, int cmd);
void tws_getset_param_complete(struct tws_request *req);
int tws_set_param(struct tws_softc *sc, u_int32_t table_id, u_int32_t param_id,
              u_int32_t param_size, void *data);
int tws_get_param(struct tws_softc *sc, u_int32_t table_id, u_int32_t param_id,
              u_int32_t param_size, void *data);


extern struct tws_request *tws_get_request(struct tws_softc *sc,
                                            u_int16_t type);
extern void *tws_release_request(struct tws_request *req);
extern int tws_submit_command(struct tws_softc *sc, struct tws_request *req);
extern boolean tws_get_response(struct tws_softc *sc,
                                           u_int16_t *req_id, u_int64_t *mfa);
extern void tws_q_insert_tail(struct tws_softc *sc, struct tws_request *req,
                                u_int8_t q_type );
extern struct tws_request * tws_q_remove_request(struct tws_softc *sc,
                                   struct tws_request *req, u_int8_t q_type );
extern void tws_send_event(struct tws_softc *sc, u_int8_t event);

extern struct tws_sense *
tws_find_sense_from_mfa(struct tws_softc *sc, u_int64_t mfa);

extern void tws_fetch_aen(void *arg);
extern void tws_disable_db_intr(struct tws_softc *sc);
extern void tws_enable_db_intr(struct tws_softc *sc);
extern void tws_passthru_complete(struct tws_request *req);
extern void tws_aen_synctime_with_host(struct tws_softc *sc);
extern void tws_circular_aenq_insert(struct tws_softc *sc,
                    struct tws_circular_q *cq, struct tws_event_packet *aen);
extern int tws_use_32bit_sgls;
extern boolean tws_ctlr_reset(struct tws_softc *sc);
extern struct tws_request * tws_q_remove_tail(struct tws_softc *sc,
                                                           u_int8_t q_type );
extern void tws_turn_off_interrupts(struct tws_softc *sc);
extern void tws_turn_on_interrupts(struct tws_softc *sc);
extern int tws_init_connect(struct tws_softc *sc, u_int16_t mc);
extern void tws_init_obfl_q(struct tws_softc *sc);
extern uint8_t tws_get_state(struct tws_softc *sc);
extern void tws_assert_soft_reset(struct tws_softc *sc);
extern boolean tws_ctlr_ready(struct tws_softc *sc);
extern u_int16_t tws_poll4_response(struct tws_softc *sc, u_int64_t *mfa);



int
tws_cam_attach(struct tws_softc *sc)
{
    struct cam_devq *devq;
    int error;

    TWS_TRACE_DEBUG(sc, "entry", 0, sc);
    /* Create a device queue for sim */

    /*
     * if the user sets cam depth to less than 1
     * cam may get confused
     */
    if ( tws_cam_depth < 1 )
        tws_cam_depth = 1;
    if ( tws_cam_depth > (tws_queue_depth - TWS_RESERVED_REQS)  )
        tws_cam_depth = tws_queue_depth - TWS_RESERVED_REQS;

    TWS_TRACE_DEBUG(sc, "depths,ctlr,cam", tws_queue_depth, tws_cam_depth);

    if ((devq = cam_simq_alloc(tws_cam_depth)) == NULL) {
        tws_log(sc, CAM_SIMQ_ALLOC);
        return(ENOMEM);
    }

   /*
    * Create a SIM entry.  Though we can support tws_cam_depth
    * simultaneous requests, we claim to be able to handle only
    * (tws_cam_depth), so that we always have reserved  requests
    * packet available to service ioctls and internal commands.
    */
    sc->sim = cam_sim_alloc(tws_action, tws_poll, "tws", sc,
                      device_get_unit(sc->tws_dev),
                      &sc->sim_lock,
                      tws_cam_depth, 1, devq);
                      /* 1, 1, devq); */
    cam_simq_release(devq);
    if (sc->sim == NULL) {
        tws_log(sc, CAM_SIM_ALLOC);
    }
    /* Register the bus. */
    lockmgr(&sc->sim_lock, LK_EXCLUSIVE);
    if (xpt_bus_register(sc->sim, 0) != CAM_SUCCESS) {
        cam_sim_free(sc->sim);
        sc->sim = NULL; /* so cam_detach will not try to free it */
        lockmgr(&sc->sim_lock, LK_RELEASE);
        tws_log(sc, TWS_XPT_BUS_REGISTER);
        return(ENXIO);
    }
    if (xpt_create_path(&sc->path, NULL, cam_sim_path(sc->sim),
                         CAM_TARGET_WILDCARD,
                         CAM_LUN_WILDCARD) != CAM_REQ_CMP) {
        xpt_bus_deregister(cam_sim_path(sc->sim));
        cam_sim_free(sc->sim);
        tws_log(sc, TWS_XPT_CREATE_PATH);
        lockmgr(&sc->sim_lock, LK_RELEASE);
        return(ENXIO);
    }
    if ((error = tws_bus_scan(sc))) {
        tws_log(sc, TWS_BUS_SCAN_REQ);
        lockmgr(&sc->sim_lock, LK_RELEASE);
        return(error);
    }
    lockmgr(&sc->sim_lock, LK_RELEASE);

    return(0);
}

void
tws_cam_detach(struct tws_softc *sc)
{
    TWS_TRACE_DEBUG(sc, "entry", 0, 0);
    lockmgr(&sc->sim_lock, LK_EXCLUSIVE);
    if (sc->path)
        xpt_free_path(sc->path);
    if (sc->sim) {
        xpt_bus_deregister(cam_sim_path(sc->sim));
        cam_sim_free(sc->sim);
    }
    lockmgr(&sc->sim_lock, LK_RELEASE);
}

int
tws_bus_scan(struct tws_softc *sc)
{
    struct cam_path *path;
    union ccb       *ccb;

    TWS_TRACE_DEBUG(sc, "entry", sc, 0);
    KASSERT(sc->sim, ("sim not allocated"));
    KKASSERT(lockstatus(&sc->sim_lock, curthread) != 0);

    ccb = sc->scan_ccb;

    bzero(ccb, sizeof(union ccb));
    if (xpt_create_path(&path, xpt_periph, cam_sim_path(sc->sim),
                  CAM_TARGET_WILDCARD, CAM_LUN_WILDCARD) != CAM_REQ_CMP) {
        kfree(ccb, M_TEMP);
        /* lockmgr(&sc->sim_lock, LK_RELEASE); */
        return(EIO);
    }
    xpt_setup_ccb(&ccb->ccb_h, path, 5);
    ccb->ccb_h.func_code = XPT_SCAN_BUS;
    ccb->ccb_h.cbfcnp = tws_bus_scan_cb;
    ccb->crcn.flags = CAM_FLAG_NONE;
    xpt_action(ccb);

    return(0);
}

static void
tws_bus_scan_cb(struct cam_periph *periph, union ccb *ccb)
{
    struct tws_softc *sc = periph->softc;

    /* calling trace results in non-sleepable lock head panic
       using printf to debug */

    if (ccb->ccb_h.status != CAM_REQ_CMP) {
        kprintf("cam_scan failure\n");

        lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
        tws_send_event(sc, TWS_SCAN_FAILURE);
        lockmgr(&sc->gen_lock, LK_RELEASE);
    }

    xpt_free_path(ccb->ccb_h.path);
}

static void
tws_action(struct cam_sim *sim, union ccb *ccb)
{
    struct tws_softc *sc = (struct tws_softc *)cam_sim_softc(sim);

    switch( ccb->ccb_h.func_code ) {
        case XPT_SCSI_IO:
        {
            if ( tws_execute_scsi(sc, ccb) )
                TWS_TRACE_DEBUG(sc, "execute scsi failed", 0, 0);
            break;
        }
        case XPT_ABORT:
        {
            TWS_TRACE_DEBUG(sc, "abort i/o", 0, 0);
            ccb->ccb_h.status = CAM_UA_ABORT;
            xpt_done(ccb);
            break;
        }
        case XPT_RESET_BUS:
        {
            TWS_TRACE_DEBUG(sc, "reset bus", sim, ccb);
            break;
        }
        case XPT_SET_TRAN_SETTINGS:
        {
            TWS_TRACE_DEBUG(sc, "set tran settings", sim, ccb);
            ccb->ccb_h.status = CAM_FUNC_NOTAVAIL;
            xpt_done(ccb);

            break;
        }
        case XPT_GET_TRAN_SETTINGS:
        {
            TWS_TRACE_DEBUG(sc, "get tran settings", sim, ccb);

            ccb->cts.protocol = PROTO_SCSI;
            ccb->cts.protocol_version = SCSI_REV_2;
            ccb->cts.transport = XPORT_SPI;
            ccb->cts.transport_version = 2;

            ccb->cts.xport_specific.spi.valid = CTS_SPI_VALID_DISC;
            ccb->cts.xport_specific.spi.flags = CTS_SPI_FLAGS_DISC_ENB;
            ccb->cts.proto_specific.scsi.valid = CTS_SCSI_VALID_TQ;
            ccb->cts.proto_specific.scsi.flags = CTS_SCSI_FLAGS_TAG_ENB;
            ccb->ccb_h.status = CAM_REQ_CMP;
            xpt_done(ccb);

            break;
        }
        case XPT_CALC_GEOMETRY:
        {
            TWS_TRACE_DEBUG(sc, "calc geometry(ccb,block-size)", ccb,
                                          ccb->ccg.block_size);
            cam_calc_geometry(&ccb->ccg, 1/* extended */);
            xpt_done(ccb);

            break;
        }
        case XPT_PATH_INQ:
        {
            TWS_TRACE_DEBUG(sc, "path inquiry", sim, ccb);
            ccb->cpi.version_num = 1;
            ccb->cpi.hba_inquiry = 0;
            ccb->cpi.target_sprt = 0;
            ccb->cpi.hba_misc = 0;
            ccb->cpi.hba_eng_cnt = 0;
            ccb->cpi.max_target = TWS_MAX_NUM_UNITS;
            ccb->cpi.max_lun = TWS_MAX_NUM_LUNS - 1;
            ccb->cpi.unit_number = cam_sim_unit(sim);
            ccb->cpi.bus_id = cam_sim_bus(sim);
            ccb->cpi.initiator_id = TWS_SCSI_INITIATOR_ID;
            ccb->cpi.base_transfer_speed = 300000;
            strncpy(ccb->cpi.sim_vid, "FreeBSD", SIM_IDLEN);
            strncpy(ccb->cpi.hba_vid, "3ware", HBA_IDLEN);
            strncpy(ccb->cpi.dev_name, cam_sim_name(sim), DEV_IDLEN);
            ccb->cpi.transport = XPORT_SPI;
            ccb->cpi.transport_version = 2;
            ccb->cpi.protocol = PROTO_SCSI;
            ccb->cpi.protocol_version = SCSI_REV_2;
            ccb->ccb_h.status = CAM_REQ_CMP;
            xpt_done(ccb);

            break;
        }
        default:
            TWS_TRACE_DEBUG(sc, "default", sim, ccb);
            ccb->ccb_h.status = CAM_REQ_INVALID;
            xpt_done(ccb);
            break;
    }
}

static void
tws_scsi_complete(struct tws_request *req)
{
    struct tws_softc *sc = req->sc;

    lockmgr(&sc->q_lock, LK_EXCLUSIVE);
    tws_q_remove_request(sc, req, TWS_BUSY_Q);
    lockmgr(&sc->q_lock, LK_RELEASE);

    callout_stop(&req->ccb_ptr->ccb_h.timeout_ch);
    tws_unmap_request(req->sc, req);


    lockmgr(&sc->sim_lock, LK_EXCLUSIVE);
    req->ccb_ptr->ccb_h.status = CAM_REQ_CMP;
    xpt_done(req->ccb_ptr);
    lockmgr(&sc->sim_lock, LK_RELEASE);

    lockmgr(&sc->q_lock, LK_EXCLUSIVE);
    tws_q_insert_tail(sc, req, TWS_FREE_Q);
    lockmgr(&sc->q_lock, LK_RELEASE);

}

void
tws_getset_param_complete(struct tws_request *req)
{
    struct tws_softc *sc = req->sc;

    TWS_TRACE_DEBUG(sc, "getset complete", req, req->request_id);

    callout_stop(&req->thandle);
    tws_unmap_request(sc, req);

    kfree(req->data, M_TWS);

    lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
    req->state = TWS_REQ_STATE_FREE;
    lockmgr(&sc->gen_lock, LK_RELEASE);

}

void
tws_aen_complete(struct tws_request *req)
{
    struct tws_softc *sc = req->sc;
    struct tws_command_header *sense;
    struct tws_event_packet event;
    u_int16_t aen_code=0;

    TWS_TRACE_DEBUG(sc, "aen complete", 0, req->request_id);

    callout_stop(&req->thandle);
    tws_unmap_request(sc, req);

    sense = (struct tws_command_header *)req->data;

    TWS_TRACE_DEBUG(sc,"sense code, key",sense->sense_data[0],
                                   sense->sense_data[2]);
    TWS_TRACE_DEBUG(sc,"sense rid, seve",sense->header_desc.request_id,
                                   sense->status_block.res__severity);
    TWS_TRACE_DEBUG(sc,"sense srcnum, error",sense->status_block.srcnum,
                                   sense->status_block.error);
    TWS_TRACE_DEBUG(sc,"sense shdr, ssense",sense->header_desc.size_header,
                                   sense->header_desc.size_sense);

    aen_code = sense->status_block.error;

    switch ( aen_code ) {
        case TWS_AEN_SYNC_TIME_WITH_HOST :
            tws_aen_synctime_with_host(sc);
            break;
        case TWS_AEN_QUEUE_EMPTY :
            break;
        default :
            bzero(&event, sizeof(struct tws_event_packet));
            event.sequence_id = sc->seq_id;
            event.time_stamp_sec = (u_int32_t)TWS_LOCAL_TIME;
            event.aen_code = sense->status_block.error;
            event.severity = sense->status_block.res__severity & 0x7;
            event.event_src = TWS_SRC_CTRL_EVENT;
            strcpy(event.severity_str, tws_sev_str[event.severity]);
            event.retrieved = TWS_AEN_NOT_RETRIEVED;

            bcopy(sense->err_specific_desc, event.parameter_data,
                                    TWS_ERROR_SPECIFIC_DESC_LEN);
            event.parameter_data[TWS_ERROR_SPECIFIC_DESC_LEN - 1] = '\0';
            event.parameter_len = (u_int8_t)strlen(event.parameter_data)+1;

            if ( event.parameter_len < TWS_ERROR_SPECIFIC_DESC_LEN ) {
                event.parameter_len += ((u_int8_t)strlen(event.parameter_data +
                                                event.parameter_len) + 1);
            }

            device_printf(sc->tws_dev, "%s: (0x%02X: 0x%04X): %s: %s\n",
                event.severity_str,
                event.event_src,
                event.aen_code,
                event.parameter_data +
                     (strlen(event.parameter_data) + 1),
                event.parameter_data);

            lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
            tws_circular_aenq_insert(sc, &sc->aen_q, &event);
            sc->seq_id++;
            lockmgr(&sc->gen_lock, LK_RELEASE);
            break;

    }

    kfree(req->data, M_TWS);

    lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
    req->state = TWS_REQ_STATE_FREE;
    lockmgr(&sc->gen_lock, LK_RELEASE);

    if ( aen_code != TWS_AEN_QUEUE_EMPTY ) {
        /* timeout(tws_fetch_aen, sc, 1);*/
        sc->stats.num_aens++;
        tws_fetch_aen((void *)sc);
    }

}

void
tws_cmd_complete(struct tws_request *req)
{
    struct tws_softc *sc = req->sc;

    callout_stop(&req->ccb_ptr->ccb_h.timeout_ch);
    tws_unmap_request(sc, req);

}

static void
tws_err_complete(struct tws_softc *sc, u_int64_t mfa)
{

    struct tws_command_header *hdr;
    struct tws_sense *sen;
    struct tws_request *req;
    u_int16_t req_id;
    u_int32_t reg, status;

    if ( !mfa ) {
        TWS_TRACE_DEBUG(sc, "null mfa", 0, mfa);
        return;
    } else {
        /* lookup the sense */
        sen = tws_find_sense_from_mfa(sc, mfa);
        if ( sen == NULL ) {
            TWS_TRACE_DEBUG(sc, "found null req", 0, mfa);
            return;
        }
        hdr = sen->hdr;
        TWS_TRACE_DEBUG(sc, "sen, hdr", sen, hdr);
        req_id = hdr->header_desc.request_id;
        req = &sc->reqs[req_id];
        TWS_TRACE_DEBUG(sc, "req, id", req, req_id);
        if ( req->error_code != TWS_REQ_SUBMIT_SUCCESS )
            TWS_TRACE_DEBUG(sc, "submit failure?", 0, req->error_code);
    }

    switch (req->type) {
        case TWS_PASSTHRU_REQ :
            tws_passthru_err_complete(req, hdr);
            break;
        case TWS_GETSET_PARAM_REQ :
            tws_getset_param_complete(req);
            break;
        case TWS_SCSI_IO_REQ :
            tws_scsi_err_complete(req, hdr);
            break;

    }

    lockmgr(&sc->io_lock, LK_EXCLUSIVE);
    hdr->header_desc.size_header = 128;
    reg = (u_int32_t)( mfa>>32);
    tws_write_reg(sc, TWS_I2O0_HOBQPH, reg, 4);
    reg = (u_int32_t)(mfa);
    tws_write_reg(sc, TWS_I2O0_HOBQPL, reg, 4);

    status = tws_read_reg(sc, TWS_I2O0_STATUS, 4);
    if ( status & TWS_BIT13 ) {
        TWS_TRACE_DEBUG(sc, "OBFL Overrun", status, TWS_I2O0_STATUS);
        sc->obfl_q_overrun = true;
        sen->posted = false;
    }
    lockmgr(&sc->io_lock, LK_RELEASE);

}

static void
tws_scsi_err_complete(struct tws_request *req, struct tws_command_header *hdr)
{
    u_int8_t *sense_data;
    struct tws_softc *sc = req->sc;
    union ccb *ccb = req->ccb_ptr;

    TWS_TRACE_DEBUG(sc, "sbe, cmd_status", hdr->status_block.error,
                                 req->cmd_pkt->cmd.pkt_a.status);
    if ( hdr->status_block.error == TWS_ERROR_LOGICAL_UNIT_NOT_SUPPORTED ||
         hdr->status_block.error == TWS_ERROR_UNIT_OFFLINE ) {

        if ( ccb->ccb_h.target_lun ) {
            TWS_TRACE_DEBUG(sc, "invalid lun error",0,0);
            ccb->ccb_h.status |= CAM_LUN_INVALID;
        } else {
            TWS_TRACE_DEBUG(sc, "invalid target error",0,0);
            ccb->ccb_h.status |= CAM_TID_INVALID;
        }

    } else {
        TWS_TRACE_DEBUG(sc, "scsi status  error",0,0);
        ccb->ccb_h.status |= CAM_SCSI_STATUS_ERROR;
        if (((ccb->csio.cdb_io.cdb_bytes[0] == 0x1A) &&
              (hdr->status_block.error == TWS_ERROR_NOT_SUPPORTED))) {
            ccb->ccb_h.status |= CAM_SCSI_STATUS_ERROR | CAM_AUTOSNS_VALID;
            TWS_TRACE_DEBUG(sc, "page mode not supported",0,0);
        }
    }

    /* if there were no error simply mark complete error */
    if (ccb->ccb_h.status == 0)
        ccb->ccb_h.status = CAM_REQ_CMP_ERR;

    sense_data = (u_int8_t *)&ccb->csio.sense_data;
    if (sense_data) {
        memcpy(sense_data, hdr->sense_data, TWS_SENSE_DATA_LENGTH );
        ccb->csio.sense_len = TWS_SENSE_DATA_LENGTH;
        ccb->ccb_h.status |= CAM_AUTOSNS_VALID;
    }
    ccb->csio.scsi_status = req->cmd_pkt->cmd.pkt_a.status;

    ccb->ccb_h.status &= ~CAM_SIM_QUEUED;
    lockmgr(&sc->sim_lock, LK_EXCLUSIVE);
    xpt_done(ccb);
    lockmgr(&sc->sim_lock, LK_RELEASE);

    callout_stop(&req->ccb_ptr->ccb_h.timeout_ch);
    tws_unmap_request(req->sc, req);
    lockmgr(&sc->q_lock, LK_EXCLUSIVE);
    tws_q_remove_request(sc, req, TWS_BUSY_Q);
    tws_q_insert_tail(sc, req, TWS_FREE_Q);
    lockmgr(&sc->q_lock, LK_RELEASE);

}

static void
tws_passthru_err_complete(struct tws_request *req,
                                          struct tws_command_header *hdr)
{

    TWS_TRACE_DEBUG(req->sc, "entry", hdr, req->request_id);
    req->error_code = hdr->status_block.error;
    memcpy(&(req->cmd_pkt->hdr), hdr, sizeof(struct tws_command_header));
    tws_passthru_complete(req);
}

static void
tws_drain_busy_queue(struct tws_softc *sc)
{

    struct tws_request *req;
    TWS_TRACE_DEBUG(sc, "entry", 0, 0);

    lockmgr(&sc->q_lock, LK_EXCLUSIVE);
    req = tws_q_remove_tail(sc, TWS_BUSY_Q);
    lockmgr(&sc->q_lock, LK_RELEASE);
    while ( req ) {
	callout_stop(&req->ccb_ptr->ccb_h.timeout_ch);
        tws_unmap_request(req->sc, req);

        TWS_TRACE_DEBUG(sc, "drained", 0, req->request_id);

        lockmgr(&sc->sim_lock, LK_EXCLUSIVE);
        req->ccb_ptr->ccb_h.status = CAM_REQUEUE_REQ;
        xpt_done(req->ccb_ptr);
        lockmgr(&sc->sim_lock, LK_RELEASE);

        lockmgr(&sc->q_lock, LK_EXCLUSIVE);
        tws_q_insert_tail(sc, req, TWS_FREE_Q);
        req = tws_q_remove_tail(sc, TWS_BUSY_Q);
        lockmgr(&sc->q_lock, LK_RELEASE);
    }

}

static void
tws_drain_reserved_reqs(struct tws_softc *sc)
{

    struct tws_request *r;

    r = &sc->reqs[1];
    if ( r->state != TWS_REQ_STATE_FREE ) {
        TWS_TRACE_DEBUG(sc, "drained aen req", 0, 0);
	callout_stop(&r->thandle);
        tws_unmap_request(sc, r);
        kfree(r->data, M_TWS);
        lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
        r->state = TWS_REQ_STATE_FREE;
        lockmgr(&sc->gen_lock, LK_RELEASE);
    }
    r = &sc->reqs[2];
    if ( r->state != TWS_REQ_STATE_FREE ) {
        TWS_TRACE_DEBUG(sc, "drained passthru req", 0, 0);
        r->error_code = TWS_REQ_REQUEUE;
        tws_passthru_complete(r);
    }
    r = &sc->reqs[3];
    if ( r->state != TWS_REQ_STATE_FREE ) {
        TWS_TRACE_DEBUG(sc, "drained set param req", 0, 0);
        tws_getset_param_complete(r);
    }

}

static void
tws_drain_response_queue(struct tws_softc *sc)
{
    tws_intr_resp(sc);
}


static int32_t
tws_execute_scsi(struct tws_softc *sc, union ccb *ccb)
{
    struct tws_command_packet *cmd_pkt;
    struct tws_request *req;
    struct ccb_hdr *ccb_h = &(ccb->ccb_h);
    struct ccb_scsiio *csio = &(ccb->csio);
    int error;
    u_int16_t lun;

    KKASSERT(lockstatus(&sc->sim_lock, curthread) != 0);
    if (ccb_h->target_id >= TWS_MAX_NUM_UNITS) {
        TWS_TRACE_DEBUG(sc, "traget id too big", ccb_h->target_id, ccb_h->target_lun);
        ccb_h->status |= CAM_TID_INVALID;
        xpt_done(ccb);
        return(0);
    }
    if (ccb_h->target_lun >= TWS_MAX_NUM_LUNS) {
        TWS_TRACE_DEBUG(sc, "target lun 2 big", ccb_h->target_id, ccb_h->target_lun);
        ccb_h->status |= CAM_LUN_INVALID;
        xpt_done(ccb);
        return(0);
    }

    if(ccb_h->flags & CAM_CDB_PHYS) {
        TWS_TRACE_DEBUG(sc, "cdb phy", ccb_h->target_id, ccb_h->target_lun);
        ccb_h->status = CAM_REQ_CMP_ERR;
        xpt_done(ccb);
        return(0);
    }

    /*
     * We are going to work on this request.  Mark it as enqueued (though
     * we don't actually queue it...)
     */
    ccb_h->status |= CAM_SIM_QUEUED;

    req = tws_get_request(sc, TWS_SCSI_IO_REQ);
    if ( !req ) {
        TWS_TRACE_DEBUG(sc, "no reqs", ccb_h->target_id, ccb_h->target_lun);
        /* tws_freeze_simq(sc); */
        ccb_h->status |= CAM_REQUEUE_REQ;
        xpt_done(ccb);
        return(0);
    }

    if((ccb_h->flags & CAM_DIR_MASK) != CAM_DIR_NONE) {
        if(ccb_h->flags & CAM_DIR_IN)
            req->flags = TWS_DIR_IN;
        else
            req->flags = TWS_DIR_OUT;
    } else {
        req->flags = TWS_DIR_NONE; /* no data */
    }

    req->type = TWS_SCSI_IO_REQ;
    req->cb = tws_scsi_complete;

    cmd_pkt = req->cmd_pkt;
    /* cmd_pkt->hdr.header_desc.size_header = 128; */
    cmd_pkt->cmd.pkt_a.res__opcode = TWS_FW_CMD_EXECUTE_SCSI;
    cmd_pkt->cmd.pkt_a.unit = ccb_h->target_id;
    cmd_pkt->cmd.pkt_a.status = 0;
    cmd_pkt->cmd.pkt_a.sgl_offset = 16;

    /* lower nibble */
    lun = ccb_h->target_lun & 0XF;
    lun = lun << 12;
    cmd_pkt->cmd.pkt_a.lun_l4__req_id = lun | req->request_id;
    /* upper nibble */
    lun = ccb_h->target_lun & 0XF0;
    lun = lun << 8;
    cmd_pkt->cmd.pkt_a.lun_h4__sgl_entries = lun;

#ifdef TWS_DEBUG
    if ( csio->cdb_len > 16 )
         TWS_TRACE(sc, "cdb len too big", ccb_h->target_id, csio->cdb_len);
#endif

    if(ccb_h->flags & CAM_CDB_POINTER)
        bcopy(csio->cdb_io.cdb_ptr, cmd_pkt->cmd.pkt_a.cdb, csio->cdb_len);
    else
        bcopy(csio->cdb_io.cdb_bytes, cmd_pkt->cmd.pkt_a.cdb, csio->cdb_len);

    if (!(ccb_h->flags & CAM_DATA_PHYS)) {
         /* Virtual data addresses.  Need to convert them... */
         if (!(ccb_h->flags & CAM_SCATTER_VALID)) {
             if (csio->dxfer_len > TWS_MAX_IO_SIZE) {
                 TWS_TRACE(sc, "I/O is big", csio->dxfer_len, 0);
                 tws_release_request(req);
                 ccb_h->status = CAM_REQ_TOO_BIG;
                 xpt_done(ccb);
                 return(0);
             }

             req->length = csio->dxfer_len;
             if (req->length) {
                 req->data = csio->data_ptr;
                 /* there is 1 sgl_entrie */
                 /* cmd_pkt->cmd.pkt_a.lun_h4__sgl_entries |= 1; */
             }
         } else {
             TWS_TRACE_DEBUG(sc, "got sglist", ccb_h->target_id, ccb_h->target_lun);
             tws_release_request(req);
             ccb_h->status = CAM_REQ_CMP_ERR;
             xpt_done(ccb);
             return(0);
         }
    } else {
         /* Data addresses are physical. */
         TWS_TRACE_DEBUG(sc, "Phy data addr", ccb_h->target_id, ccb_h->target_lun);
         tws_release_request(req);
         ccb_h->status = CAM_REQ_CMP_ERR;
         ccb_h->status |= CAM_RELEASE_SIMQ;
         ccb_h->status &= ~CAM_SIM_QUEUED;
         xpt_done(ccb);
         return(0);
    }
    /* save ccb ptr */
    req->ccb_ptr = ccb;
    /*
     * tws_map_load_data_callback will fill in the SGL,
     * and submit the I/O.
     */
    sc->stats.scsi_ios++;
    callout_reset(&ccb_h->timeout_ch, (ccb_h->timeout * hz)/1000, tws_timeout,
	req);
    error = tws_map_request(sc, req);
    return(error);
}


int
tws_send_scsi_cmd(struct tws_softc *sc, int cmd)
{

    struct tws_request *req;
    struct tws_command_packet *cmd_pkt;
    int error;

    TWS_TRACE_DEBUG(sc, "entry",sc, cmd);
    req = tws_get_request(sc, TWS_AEN_FETCH_REQ);

    if ( req == NULL )
        return(ENOMEM);

    req->type = TWS_AEN_FETCH_REQ;
    req->cb = tws_aen_complete;

    cmd_pkt = req->cmd_pkt;
    cmd_pkt->cmd.pkt_a.res__opcode = TWS_FW_CMD_EXECUTE_SCSI;
    cmd_pkt->cmd.pkt_a.status = 0;
    cmd_pkt->cmd.pkt_a.unit = 0;
    cmd_pkt->cmd.pkt_a.sgl_offset = 16;
    cmd_pkt->cmd.pkt_a.lun_l4__req_id = req->request_id;

    cmd_pkt->cmd.pkt_a.cdb[0] = (u_int8_t)cmd;
    cmd_pkt->cmd.pkt_a.cdb[4] = 128;

    req->length = TWS_SECTOR_SIZE;
    req->data = kmalloc(TWS_SECTOR_SIZE, M_TWS, M_NOWAIT);
    if ( req->data == NULL )
        return(ENOMEM);
    bzero(req->data, TWS_SECTOR_SIZE);
    req->flags = TWS_DIR_IN;

    callout_reset(&req->thandle, (TWS_IO_TIMEOUT * hz), tws_timeout, req);
    error = tws_map_request(sc, req);
    return(error);

}

int
tws_set_param(struct tws_softc *sc, u_int32_t table_id, u_int32_t param_id,
              u_int32_t param_size, void *data)
{
    struct tws_request *req;
    struct tws_command_packet *cmd_pkt;
    union tws_command_giga *cmd;
    struct tws_getset_param *param;
    int error;

    req = tws_get_request(sc, TWS_GETSET_PARAM_REQ);
    if ( req == NULL ) {
        TWS_TRACE_DEBUG(sc, "null req", 0, 0);
        return(ENOMEM);
    }

    req->length = TWS_SECTOR_SIZE;
    req->data = kmalloc(TWS_SECTOR_SIZE, M_TWS, M_NOWAIT);
    if ( req->data == NULL )
        return(ENOMEM);
    bzero(req->data, TWS_SECTOR_SIZE);
    param = (struct tws_getset_param *)req->data;

    req->cb = tws_getset_param_complete;
    req->flags = TWS_DIR_OUT;
    cmd_pkt = req->cmd_pkt;

    cmd = &cmd_pkt->cmd.pkt_g;
    cmd->param.sgl_off__opcode =
            BUILD_SGL_OFF__OPCODE(2, TWS_FW_CMD_SET_PARAM);
    cmd->param.request_id = (u_int8_t)req->request_id;
    cmd->param.host_id__unit = 0;
    cmd->param.param_count = 1;
    cmd->param.size = 2; /* map routine will add sgls */

    /* Specify which parameter we want to set. */
    param->table_id = (table_id | TWS_9K_PARAM_DESCRIPTOR);
    param->parameter_id = (u_int8_t)(param_id);
    param->parameter_size_bytes = (u_int16_t)param_size;
    memcpy(param->data, data, param_size);

    callout_reset(&req->thandle, (TWS_IO_TIMEOUT * hz), tws_timeout, req);
    error = tws_map_request(sc, req);
    return(error);

}

int
tws_get_param(struct tws_softc *sc, u_int32_t table_id, u_int32_t param_id,
              u_int32_t param_size, void *data)
{
    struct tws_request *req;
    struct tws_command_packet *cmd_pkt;
    union tws_command_giga *cmd;
    struct tws_getset_param *param;
    u_int16_t reqid;
    u_int64_t mfa;
    int error = SUCCESS;


    req = tws_get_request(sc, TWS_GETSET_PARAM_REQ);
    if ( req == NULL ) {
        TWS_TRACE_DEBUG(sc, "null req", 0, 0);
        return(FAILURE);
    }

    req->length = TWS_SECTOR_SIZE;
    req->data = kmalloc(TWS_SECTOR_SIZE, M_TWS, M_NOWAIT);
    if ( req->data == NULL )
        return(FAILURE);
    bzero(req->data, TWS_SECTOR_SIZE);
    param = (struct tws_getset_param *)req->data;

    req->cb = NULL;
    req->flags = TWS_DIR_IN;
    cmd_pkt = req->cmd_pkt;

    cmd = &cmd_pkt->cmd.pkt_g;
    cmd->param.sgl_off__opcode =
            BUILD_SGL_OFF__OPCODE(2, TWS_FW_CMD_GET_PARAM);
    cmd->param.request_id = (u_int8_t)req->request_id;
    cmd->param.host_id__unit = 0;
    cmd->param.param_count = 1;
    cmd->param.size = 2; /* map routine will add sgls */

    /* Specify which parameter we want to set. */
    param->table_id = (table_id | TWS_9K_PARAM_DESCRIPTOR);
    param->parameter_id = (u_int8_t)(param_id);
    param->parameter_size_bytes = (u_int16_t)param_size;

    tws_map_request(sc, req);
    reqid = tws_poll4_response(sc, &mfa);
    tws_unmap_request(sc, req);

    if ( reqid == TWS_GETSET_PARAM_REQ ) {
        memcpy(data, param->data, param_size);
    } else {
        error = FAILURE;

    }

    kfree(req->data, M_TWS);
    lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
    req->state = TWS_REQ_STATE_FREE;
    lockmgr(&sc->gen_lock, LK_RELEASE);
    return(error);

}

void
tws_unmap_request(struct tws_softc *sc, struct tws_request *req)
{

    if (req->data != NULL) {
        if ( req->flags & TWS_DIR_IN )
            bus_dmamap_sync(sc->data_tag, req->dma_map,
                                            BUS_DMASYNC_POSTREAD);
        if ( req->flags & TWS_DIR_OUT )
            bus_dmamap_sync(sc->data_tag, req->dma_map,
                                            BUS_DMASYNC_POSTWRITE);
        lockmgr(&sc->io_lock, LK_EXCLUSIVE);
        bus_dmamap_unload(sc->data_tag, req->dma_map);
        lockmgr(&sc->io_lock, LK_RELEASE);
    }
}

int32_t
tws_map_request(struct tws_softc *sc, struct tws_request *req)
{
    int32_t error = 0;


    /* If the command involves data, map that too. */
    if (req->data != NULL) {
        /*
         * Map the data buffer into bus space and build the SG list.
         */
        lockmgr(&sc->io_lock, LK_EXCLUSIVE);
        error = bus_dmamap_load(sc->data_tag, req->dma_map,
                                req->data, req->length,
                                tws_dmamap_data_load_cbfn, req,
                                BUS_DMA_WAITOK);
        lockmgr(&sc->io_lock, LK_RELEASE);

        if (error == EINPROGRESS) {
            TWS_TRACE(sc, "in progress", 0, error);
            /* tws_freeze_simq(sc); */
            error = TWS_REQ_ERR_INPROGRESS;
        }
    } else { /* no data involved */
        error = tws_submit_command(sc, req);
    }
    req->error_code = error;
    return(error);
}


static void
tws_dmamap_data_load_cbfn(void *arg, bus_dma_segment_t *segs,
                            int nseg, int error)
{

    struct tws_request *req = (struct tws_request *)arg;
    struct tws_softc *sc = req->sc;
    u_int16_t sgls = nseg;
    void *sgl_ptr;
    struct tws_cmd_generic *gcmd;

    if ( error == EFBIG )
        TWS_TRACE(sc, "not enough data segs", 0, nseg);


    if ( req->flags & TWS_DIR_IN )
        bus_dmamap_sync(req->sc->data_tag, req->dma_map,
                                            BUS_DMASYNC_PREREAD);
    if ( req->flags & TWS_DIR_OUT )
        bus_dmamap_sync(req->sc->data_tag, req->dma_map,
                                        BUS_DMASYNC_PREWRITE);
    if ( segs ) {
        if ( (req->type == TWS_PASSTHRU_REQ &&
             GET_OPCODE(req->cmd_pkt->cmd.pkt_a.res__opcode) !=
                            TWS_FW_CMD_EXECUTE_SCSI) ||
              req->type == TWS_GETSET_PARAM_REQ) {
            gcmd = &req->cmd_pkt->cmd.pkt_g.generic;
            sgl_ptr = (u_int32_t *)(gcmd) + gcmd->size;
            gcmd->size += sgls *
                          ((req->sc->is64bit && !tws_use_32bit_sgls) ? 4 :2 );
            tws_fill_sg_list(req->sc, (void *)segs, sgl_ptr, sgls);

        } else {
            tws_fill_sg_list(req->sc, (void *)segs,
                      (void *)req->cmd_pkt->cmd.pkt_a.sg_list, sgls);
            req->cmd_pkt->cmd.pkt_a.lun_h4__sgl_entries |= sgls ;
        }
    }


    req->error_code = tws_submit_command(req->sc, req);

}


static void
tws_fill_sg_list(struct tws_softc *sc, void *sgl_src, void *sgl_dest,
                          u_int16_t num_sgl_entries)
{
    int i;

    if ( sc->is64bit ) {
        struct tws_sg_desc64 *sgl_s = (struct tws_sg_desc64 *)sgl_src;

        if ( !tws_use_32bit_sgls ) {
            struct tws_sg_desc64 *sgl_d = (struct tws_sg_desc64 *)sgl_dest;
            if ( num_sgl_entries > TWS_MAX_64BIT_SG_ELEMENTS )
                TWS_TRACE(sc, "64bit sg overflow", num_sgl_entries, 0);
            for (i = 0; i < num_sgl_entries; i++) {
                sgl_d[i].address = sgl_s->address;
                sgl_d[i].length = sgl_s->length;
                sgl_d[i].flag = 0;
                sgl_d[i].reserved = 0;
                sgl_s = (struct tws_sg_desc64 *) (((u_int8_t *)sgl_s) +
                                               sizeof(bus_dma_segment_t));
            }
        } else {
            struct tws_sg_desc32 *sgl_d = (struct tws_sg_desc32 *)sgl_dest;
            if ( num_sgl_entries > TWS_MAX_32BIT_SG_ELEMENTS )
                TWS_TRACE(sc, "32bit sg overflow", num_sgl_entries, 0);
            for (i = 0; i < num_sgl_entries; i++) {
                sgl_d[i].address = sgl_s->address;
                sgl_d[i].length = sgl_s->length;
                sgl_d[i].flag = 0;
                sgl_s = (struct tws_sg_desc64 *) (((u_int8_t *)sgl_s) +
                                               sizeof(bus_dma_segment_t));
            }
        }
    } else {
        struct tws_sg_desc32 *sgl_s = (struct tws_sg_desc32 *)sgl_src;
        struct tws_sg_desc32 *sgl_d = (struct tws_sg_desc32 *)sgl_dest;

        if ( num_sgl_entries > TWS_MAX_32BIT_SG_ELEMENTS )
            TWS_TRACE(sc, "32bit sg overflow", num_sgl_entries, 0);


        for (i = 0; i < num_sgl_entries; i++) {
            sgl_d[i].address = sgl_s[i].address;
            sgl_d[i].length = sgl_s[i].length;
            sgl_d[i].flag = 0;
        }
    }
}


void
tws_intr(void *arg)
{
    struct tws_softc *sc = (struct tws_softc *)arg;
    u_int32_t histat=0, db=0;

    KASSERT(sc, ("null softc"));

    sc->stats.num_intrs++;
    histat = tws_read_reg(sc, TWS_I2O0_HISTAT, 4);
    if ( histat & TWS_BIT2 ) {
        TWS_TRACE_DEBUG(sc, "door bell :)", histat, TWS_I2O0_HISTAT);
        db = tws_read_reg(sc, TWS_I2O0_IOBDB, 4);
        if ( db & TWS_BIT21 ) {
            tws_intr_attn_error(sc);
            return;
        }
        if ( db & TWS_BIT18 ) {
            tws_intr_attn_aen(sc);
        }
    }

    if ( histat & TWS_BIT3 ) {
        tws_intr_resp(sc);
    }
}

static void
tws_intr_attn_aen(struct tws_softc *sc)
{
    u_int32_t db=0;

    /* maskoff db intrs untill all the aens are fetched */
    /* tws_disable_db_intr(sc); */
    tws_fetch_aen((void *)sc);
    tws_write_reg(sc, TWS_I2O0_HOBDBC, TWS_BIT18, 4);
    db = tws_read_reg(sc, TWS_I2O0_IOBDB, 4);

}

static void
tws_intr_attn_error(struct tws_softc *sc)
{
    u_int32_t db=0;

    TWS_TRACE(sc, "attn error", 0, 0);
    tws_write_reg(sc, TWS_I2O0_HOBDBC, ~0, 4);
    db = tws_read_reg(sc, TWS_I2O0_IOBDB, 4);
    device_printf(sc->tws_dev, "Micro controller error.\n");
    tws_reset(sc);
}

static void
tws_intr_resp(struct tws_softc *sc)
{
    u_int16_t req_id;
    u_int64_t mfa;

    while ( tws_get_response(sc, &req_id, &mfa) ) {
        sc->stats.reqs_out++;
        if ( req_id == TWS_INVALID_REQID ) {
            TWS_TRACE_DEBUG(sc, "invalid req_id", mfa, req_id);
            sc->stats.reqs_errored++;
            tws_err_complete(sc, mfa);
            continue;
        }

        sc->reqs[req_id].cb(&sc->reqs[req_id]);
    }

}


static void
tws_poll(struct cam_sim *sim)
{
    struct tws_softc *sc = (struct tws_softc *)cam_sim_softc(sim);
    TWS_TRACE_DEBUG(sc, "entry", 0, 0);
    tws_intr((void *) sc);
}

void
tws_timeout(void *arg)
{
    struct tws_request *req = (struct tws_request *)arg;
    struct tws_softc *sc = req->sc;


    if ( tws_get_state(sc) != TWS_RESET ) {
        device_printf(sc->tws_dev, "Request timed out.\n");
        tws_reset((void *)sc);
    }
}

void
tws_reset(void *arg)
{

    struct tws_softc *sc = (struct tws_softc *)arg;

    if ( tws_get_state(sc) == TWS_RESET ) {
        return;
    }
    device_printf(sc->tws_dev,  "Resetting controller\n");
    lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
    tws_send_event(sc, TWS_RESET_START);
    lockmgr(&sc->gen_lock, LK_RELEASE);

    tws_turn_off_interrupts(sc);
    lockmgr(&sc->sim_lock, LK_EXCLUSIVE);
    tws_freeze_simq(sc);
    lockmgr(&sc->sim_lock, LK_RELEASE);

    tws_assert_soft_reset(sc);
    callout_reset(&sc->reset_cb_handle, hz/10, tws_reset_cb, sc);
}

static void
tws_reset_cb(void *arg)
{

    struct tws_softc *sc = (struct tws_softc *)arg;
    u_int32_t reg;

    if ( tws_get_state(sc) != TWS_RESET ) {
        return;
    }
    reg = tws_read_reg(sc, TWS_I2O0_SCRPD3, 4);
    if (!( reg & TWS_BIT13 )) {
	callout_reset(&sc->reset_cb_handle, hz/10, tws_reset_cb, sc);
        return;
    }
    tws_drain_response_queue(sc);
    tws_drain_busy_queue(sc);
    tws_drain_reserved_reqs(sc);
    callout_reset(&sc->reinit_handle, 5*hz, tws_reinit, sc);
}

static void
tws_reinit(void *arg)
{

    struct tws_softc *sc = (struct tws_softc *)arg;
    static int timeout_val=0, try=2  ;

    if ( !tws_ctlr_ready(sc) ) {
        timeout_val += 5;
        if ( timeout_val >= TWS_RESET_TIMEOUT ) {
           timeout_val = 0;
           if ( try )
               tws_assert_soft_reset(sc);
           try--;
        }
	callout_reset(&sc->reinit_handle, 5*hz, tws_reinit, sc);
        return;
    }

    timeout_val=0;
    try = 2;
    sc->obfl_q_overrun = false;
    if ( tws_init_connect(sc, tws_queue_depth) ) {
        TWS_TRACE_DEBUG(sc, "initConnect failed", 0, sc->is64bit);
    }
    tws_init_obfl_q(sc);

    lockmgr(&sc->sim_lock, LK_EXCLUSIVE);
    tws_release_simq(sc);
    lockmgr(&sc->sim_lock, LK_RELEASE);
    tws_turn_on_interrupts(sc);

    lockmgr(&sc->gen_lock, LK_EXCLUSIVE);
    tws_send_event(sc, TWS_RESET_COMPLETE);
    lockmgr(&sc->gen_lock, LK_RELEASE);
    if ( sc->chan ) {
        sc->chan = 0;
        wakeup((void *)&sc->chan);
    }

}


static void
tws_freeze_simq(struct tws_softc *sc)
{

    TWS_TRACE_DEBUG(sc, "freezeing", 0, 0);
    KKASSERT(lockstatus(&sc->sim_lock, curthread) != 0);
    xpt_freeze_simq(sc->sim, 1);

}
static void
tws_release_simq(struct tws_softc *sc)
{

    TWS_TRACE_DEBUG(sc, "unfreezeing", 0, 0);
    KKASSERT(lockstatus(&sc->sim_lock, curthread) != 0);
    xpt_release_simq(sc->sim, 1);

}


TUNABLE_INT("hw.tws.cam_depth", &tws_cam_depth);
