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

#ifndef _srpc_mock_h
#define _srpc_mock_h

#include <gmock/gmock.h>
#include <supla-common/proto.h>
#include <supla-common/srpc.h>

#include <vector>

class SrpcInterface {
public:
  SrpcInterface();
  virtual ~SrpcInterface();

  virtual _supla_int_t
  valueChanged(void *srpc, unsigned char channelNumber, std::vector<char> value,
               unsigned char offline,
               unsigned _supla_int_t validity_time_sec) = 0;
  virtual _supla_int_t valueChanged(void *srpc, unsigned char channelNumber,
      std::vector<char> value) = 0;

  virtual _supla_int_t srpc_dcs_async_set_activity_timeout(
      void *_srpc, TDCS_SuplaSetActivityTimeout *dcs_set_activity_timeout) = 0;
  virtual void srpc_params_init(TsrpcParams *params) = 0;
  virtual _supla_int_t
  srpc_ds_async_set_channel_result(void *_srpc, unsigned char ChannelNumber,
                                   _supla_int_t SenderID, char Success) = 0;
  virtual _supla_int_t
  srpc_ds_async_device_calcfg_result(void *_srpc,
                                    _supla_int_t ReceiverID,
                                    _supla_int_t ChannelNumber,
                                    _supla_int_t Command,
                                    _supla_int_t Result,
                                    _supla_int_t DataSize) = 0;
  unsigned _supla_int_t DataSize;
  virtual void *srpc_init(TsrpcParams *params) = 0;
  virtual void srpc_rd_free(TsrpcReceivedData *rd) = 0;
  virtual char srpc_getdata(void *_srpc, TsrpcReceivedData *rd,
                            unsigned _supla_int_t rr_id) = 0;
  virtual char srpc_iterate(void *_srpc) = 0;
  virtual void srpc_set_proto_version(void *_srpc, unsigned char version) = 0;
  virtual _supla_int_t
  srpc_ds_async_registerdevice_e(void *_srpc,
                                 TDS_SuplaRegisterDevice_E *registerdevice) = 0;
  virtual _supla_int_t srpc_dcs_async_ping_server(void *_srpc) = 0;
  virtual _supla_int_t
  srpc_csd_async_channel_state_result(void *_srpc,
                                      TDSC_ChannelState *state) = 0;
  virtual _supla_int_t srpc_dcs_async_get_user_localtime(void *_srpc) = 0;
  virtual unsigned char srpc_out_queue_item_count(void *srpc) = 0;
  virtual char srpc_output_dataexists(void *_srpc) = 0;
  virtual _supla_int_t srpc_ds_async_get_channel_functions(void *_srpc) = 0;
  virtual _supla_int_t
    srpc_ds_async_channel_value_changed_b(void *_srpc, unsigned char channel_number,
        char *value, unsigned char offline) = 0;
  virtual void srpc_free(void *_srpc) = 0;
  virtual _supla_int_t
    srpc_sd_async_get_firmware_update_url(void *_srpc,
        TDS_FirmwareUpdateParams *params) = 0;
  virtual _supla_int_t srpc_ds_async_channel_extendedvalue_changed(
      void *_srpc, unsigned char channel_number,
      TSuplaChannelExtendedValue *value) = 0;
  virtual _supla_int_t
    srpc_ds_async_action_trigger(unsigned char channel_number, 
        _supla_int_t action_trigger) = 0;

  static SrpcInterface *instance;
  _func_srpc_event_OnRemoteCallReceived on_remote_call_received;
};

class SrpcMock : public SrpcInterface {
public:
  MOCK_METHOD(_supla_int_t, valueChanged,
              (void *, unsigned char, std::vector<char>, unsigned char,
               unsigned _supla_int_t),
              (override));
  MOCK_METHOD(_supla_int_t, valueChanged,
              (void *, unsigned char, std::vector<char>), (override));
  MOCK_METHOD(_supla_int_t, srpc_dcs_async_set_activity_timeout,
              (void *, TDCS_SuplaSetActivityTimeout *), (override));
  MOCK_METHOD(void, srpc_params_init, (TsrpcParams *), (override));
  MOCK_METHOD(_supla_int_t, srpc_ds_async_set_channel_result,
              (void *, unsigned char, _supla_int_t, char), (override));
  MOCK_METHOD(_supla_int_t, srpc_ds_async_device_calcfg_result,
      (void *, _supla_int_t, _supla_int_t, _supla_int_t, _supla_int_t,
       _supla_int_t),
      (override));
  MOCK_METHOD((void *), srpc_init, (TsrpcParams *), (override));
  MOCK_METHOD(void, srpc_rd_free, (TsrpcReceivedData *), (override));
  MOCK_METHOD(char, srpc_getdata,
              (void *, TsrpcReceivedData *, unsigned _supla_int_t), (override));
  MOCK_METHOD(char, srpc_iterate, (void *), (override));
  MOCK_METHOD(void, srpc_set_proto_version, (void *, unsigned char),
              (override));
  MOCK_METHOD(_supla_int_t, srpc_ds_async_registerdevice_e,
              (void *, TDS_SuplaRegisterDevice_E *), (override));
  MOCK_METHOD(_supla_int_t, srpc_dcs_async_ping_server, (void *), (override));
  MOCK_METHOD(_supla_int_t, srpc_csd_async_channel_state_result,
              (void *, TDSC_ChannelState *), (override));
  MOCK_METHOD(_supla_int_t, srpc_dcs_async_get_user_localtime, (void *),
              (override));
  MOCK_METHOD(unsigned char, srpc_out_queue_item_count, (void *), (override));
  MOCK_METHOD(char, srpc_output_dataexists, (void *), (override));
  MOCK_METHOD(_supla_int_t, srpc_ds_async_get_channel_functions, (void *),
      (override));
  MOCK_METHOD(_supla_int_t, srpc_ds_async_channel_value_changed_b,
      (void *, unsigned char, char *, unsigned char), (override));
  MOCK_METHOD(void, srpc_free, (void *), (override));
  MOCK_METHOD(_supla_int_t, srpc_sd_async_get_firmware_update_url,
      (void *, TDS_FirmwareUpdateParams *), (override));
  MOCK_METHOD(_supla_int_t, srpc_ds_async_channel_extendedvalue_changed,
      (void *_srpc, unsigned char channel_number,
       TSuplaChannelExtendedValue *value),
      (override));
  MOCK_METHOD(_supla_int_t, srpc_ds_async_action_trigger, 
      (unsigned char channel_number, _supla_int_t action_trigger), (override));

};

#endif
