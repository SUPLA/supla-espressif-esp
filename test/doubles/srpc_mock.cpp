/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "srpc_mock.h"
#include <supla-common/srpc.h>
#include <vector>

_supla_int_t
srpc_ds_async_channel_extendedvalue_changed(void *_srpc,
                                            unsigned char channel_number,
                                            TSuplaChannelExtendedValue *value) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_ds_async_channel_extendedvalue_changed(
      _srpc, channel_number, value);
}

_supla_int_t
srpc_ds_async_channel_value_changed_c(void *_srpc, unsigned char channel_number,
                                      char *value, unsigned char offline,
                                      unsigned _supla_int_t validity_time_sec) {
  assert(SrpcInterface::instance);
  std::vector<char> vec(value, value + 8);
  return SrpcInterface::instance->valueChanged(_srpc, channel_number, vec,
                                               offline, validity_time_sec);
}

_supla_int_t srpc_dcs_async_set_activity_timeout(
    void *_srpc, TDCS_SuplaSetActivityTimeout *dcs_set_activity_timeout) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_dcs_async_set_activity_timeout(
      _srpc, dcs_set_activity_timeout);
}

void srpc_params_init(TsrpcParams *params) {
  assert(SrpcInterface::instance);
  SrpcInterface::instance->srpc_params_init(params);
}

_supla_int_t srpc_ds_async_set_channel_result(void *_srpc,
                                              unsigned char ChannelNumber,
                                              _supla_int_t SenderID,
                                              char Success) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_ds_async_set_channel_result(
      _srpc, ChannelNumber, SenderID, Success);
}

_supla_int_t
srpc_ds_async_device_calcfg_result(void *_srpc,
                                   TDS_DeviceCalCfgResult *result) {
  assert(SrpcInterface::instance);
  // last parameter - data - is ignored. Add it if you need to check it
  return SrpcInterface::instance->srpc_ds_async_device_calcfg_result(
      _srpc, result->ReceiverID, result->ChannelNumber, result->Command,
      result->Result, result->DataSize);
}

void *srpc_init(TsrpcParams *params) {
  assert(SrpcInterface::instance);
  SrpcInterface::instance->on_remote_call_received = params->on_remote_call_received;
  return SrpcInterface::instance->srpc_init(params);
}

void srpc_rd_free(TsrpcReceivedData *rd) {
  assert(SrpcInterface::instance);
  SrpcInterface::instance->srpc_rd_free(rd);
}

char srpc_getdata(void *_srpc, TsrpcReceivedData *rd,
                  unsigned _supla_int_t rr_id) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_getdata(_srpc, rd, rr_id);
}

char srpc_iterate(void *_srpc) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_iterate(_srpc);
}

void srpc_set_proto_version(void *_srpc, unsigned char version) {
  assert(SrpcInterface::instance);
  SrpcInterface::instance->srpc_set_proto_version(_srpc, version);
}

_supla_int_t
srpc_ds_async_registerdevice_e(void *_srpc,
                               TDS_SuplaRegisterDevice_E *registerdevice) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_ds_async_registerdevice_e(
      _srpc, registerdevice);
}

_supla_int_t srpc_dcs_async_ping_server(void *_srpc) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_dcs_async_ping_server(_srpc);
}

_supla_int_t srpc_csd_async_channel_state_result(void *_srpc,
                                                 TDSC_ChannelState *state) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_csd_async_channel_state_result(_srpc,
                                                                      state);
}

_supla_int_t srpc_dcs_async_get_user_localtime(void *_srpc) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_dcs_async_get_user_localtime(_srpc);
}

unsigned char srpc_out_queue_item_count(void *srpc) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_out_queue_item_count(srpc);
}

char srpc_output_dataexists(void *_srpc) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_output_dataexists(_srpc);
}

_supla_int_t srpc_ds_async_channel_value_changed(void *_srpc,
                                                 unsigned char channel_number,
                                                 char *value) {
  assert(SrpcInterface::instance);
  std::vector<char> vec(value, value + 8);
  return SrpcInterface::instance->valueChanged(_srpc, channel_number, vec);
}

_supla_int_t srpc_ds_async_get_channel_functions(void *_srpc) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_ds_async_get_channel_functions(_srpc);
}

_supla_int_t srpc_ds_async_channel_value_changed_b(void *_srpc,
                                                   unsigned char channel_number,
                                                   char *value,
                                                   unsigned char offline) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_ds_async_channel_value_changed_b(
      _srpc, channel_number, value, offline);
}

void SRPC_ICACHE_FLASH srpc_free(void *_srpc) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_free(_srpc);
}

_supla_int_t
srpc_sd_async_get_firmware_update_url(void *_srpc,
                                      TDS_FirmwareUpdateParams *params) {
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_sd_async_get_firmware_update_url(_srpc,
                                                                        params);
}

_supla_int_t 
srpc_ds_async_action_trigger(void *_srpc, TDS_ActionTrigger *action_trigger) {
  assert(action_trigger);
  assert(SrpcInterface::instance);
  return SrpcInterface::instance->srpc_ds_async_action_trigger(
      action_trigger->ChannelNumber, action_trigger->ActionTrigger);
}

SrpcInterface::SrpcInterface() {
  instance = this;
  on_remote_call_received = nullptr;
}

SrpcInterface::~SrpcInterface() {
  instance = nullptr;
  on_remote_call_received = nullptr;
}

SrpcInterface *SrpcInterface::instance = nullptr;
