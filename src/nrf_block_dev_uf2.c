/**
 * Copyright (c) 2016 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#include "nrf_block_dev_uf2.h"

/**@file
 *
 * @ingroup nrf_block_dev
 * @{
 *
 * @brief This module implements block device API. It should be used as a reference block device.
 */

static ret_code_t block_dev_uf2_init(nrf_block_dev_t const * p_blk_dev,
                                     nrf_block_dev_ev_handler ev_handler,
                                     void const * p_context)
{
    ASSERT(p_blk_dev);
    nrf_block_dev_uf2_t const * p_ram_dev = CONTAINER_OF(p_blk_dev, nrf_block_dev_uf2_t, block_dev);
    nrf_block_dev_uf2_work_t * p_work = p_ram_dev->p_work;

    p_work->geometry.blk_size = 512;
    p_work->geometry.blk_count = UF2_NUM_BLOCKS;
    p_work->p_context = p_context;
    p_work->ev_handler = ev_handler;

    if (p_work->ev_handler)
    {
        const nrf_block_dev_event_t ev = {
                NRF_BLOCK_DEV_EVT_INIT,
                NRF_BLOCK_DEV_RESULT_SUCCESS,
                NULL,
                p_work->p_context
        };

        p_work->ev_handler(p_blk_dev, &ev);
    }

    return NRF_SUCCESS;
}

static ret_code_t block_dev_uf2_uninit(nrf_block_dev_t const * p_blk_dev)
{
    ASSERT(p_blk_dev);
    nrf_block_dev_uf2_t const * p_ram_dev = CONTAINER_OF(p_blk_dev, nrf_block_dev_uf2_t, block_dev);
    nrf_block_dev_uf2_work_t * p_work = p_ram_dev->p_work;

    if (p_work->ev_handler)
    {
        /*Asynchronous operation (simulation)*/
        const nrf_block_dev_event_t ev = {
                NRF_BLOCK_DEV_EVT_UNINIT,
                NRF_BLOCK_DEV_RESULT_SUCCESS,
                NULL,
                p_work->p_context
        };

        p_work->ev_handler(p_blk_dev, &ev);
    }

    memset(p_work, 0, sizeof(nrf_block_dev_uf2_work_t));
    return NRF_SUCCESS;
}

void write_block(uint32_t block_no, uint8_t *data, bool quiet, WriteState *state);
void read_block(uint32_t block_no, uint8_t *data);


static WriteState wrState;

static ret_code_t block_dev_uf2_req(nrf_block_dev_t const * p_blk_dev,
                                    nrf_block_req_t const * p_blk,
                                    nrf_block_dev_event_type_t event)
{
    ASSERT(p_blk_dev);
    ASSERT(p_blk);
    nrf_block_dev_uf2_t const * p_ram_dev = CONTAINER_OF(p_blk_dev, nrf_block_dev_uf2_t, block_dev);
    nrf_block_dev_uf2_config_t const * p_ram_config = &p_ram_dev->ram_config;
    nrf_block_dev_uf2_work_t const * p_work = p_ram_dev->p_work;

    /*Synchronous operation*/
    uint8_t * p_buff = (void*)&p_ram_config;
    p_buff += p_blk->blk_id * p_work->geometry.blk_size;

    if (event == NRF_BLOCK_DEV_EVT_BLK_READ_DONE) {
        for (uint32_t i = 0; i < p_blk->blk_count; ++i)
            read_block(p_blk->blk_id + i, p_blk->p_buff + i * 512);
    } else {
        for (uint32_t i = 0; i < p_blk->blk_count; ++i)
            write_block(p_blk->blk_id + i, p_blk->p_buff + i * 512, false, &wrState);
    }

    if (p_work->ev_handler)
    {
        /*Asynchronous operation (simulation)*/
        const nrf_block_dev_event_t ev = {
                event,
                NRF_BLOCK_DEV_RESULT_SUCCESS,
                p_blk,
                p_work->p_context
        };

        p_work->ev_handler(p_blk_dev, &ev);
    }

    return NRF_SUCCESS;
}

static ret_code_t block_dev_uf2_read_req(nrf_block_dev_t const * p_blk_dev,
                                         nrf_block_req_t const * p_blk)
{
    return block_dev_uf2_req(p_blk_dev, p_blk, NRF_BLOCK_DEV_EVT_BLK_READ_DONE);
}

static ret_code_t block_dev_uf2_write_req(nrf_block_dev_t const * p_blk_dev,
                                          nrf_block_req_t const * p_blk)
{
    return block_dev_uf2_req(p_blk_dev, p_blk, NRF_BLOCK_DEV_EVT_BLK_WRITE_DONE);
}

static ret_code_t block_dev_uf2_ioctl(nrf_block_dev_t const * p_blk_dev,
                                      nrf_block_dev_ioctl_req_t req,
                                      void * p_data)
{
    nrf_block_dev_uf2_t const * p_ram_dev = CONTAINER_OF(p_blk_dev, nrf_block_dev_uf2_t, block_dev);
    switch (req)
    {
        case NRF_BLOCK_DEV_IOCTL_REQ_CACHE_FLUSH:
        {
            bool * p_flushing = p_data;
            if (p_flushing)
            {
                *p_flushing = false;
            }
            return NRF_SUCCESS;
        }
        case NRF_BLOCK_DEV_IOCTL_REQ_INFO_STRINGS:
        {
            if (p_data == NULL)
            {
                return NRF_ERROR_INVALID_PARAM;
            }

            nrf_block_dev_info_strings_t const * * pp_strings = p_data;
            *pp_strings = &p_ram_dev->info_strings;
            return NRF_SUCCESS;
        }
        default:
            break;
    }

    return NRF_ERROR_NOT_SUPPORTED;
}


static nrf_block_dev_geometry_t const * block_dev_uf2_geometry(nrf_block_dev_t const * p_blk_dev)
{
    ASSERT(p_blk_dev);
    nrf_block_dev_uf2_t const * p_ram_dev = CONTAINER_OF(p_blk_dev, nrf_block_dev_uf2_t, block_dev);
    nrf_block_dev_uf2_work_t const * p_work = p_ram_dev->p_work;

    return &p_work->geometry;
}

const nrf_block_dev_ops_t nrf_block_device_uf2_ops = {
        .init = block_dev_uf2_init,
        .uninit = block_dev_uf2_uninit,
        .read_req = block_dev_uf2_read_req,
        .write_req = block_dev_uf2_write_req,
        .ioctl = block_dev_uf2_ioctl,
        .geometry = block_dev_uf2_geometry,
};


/** @} */
