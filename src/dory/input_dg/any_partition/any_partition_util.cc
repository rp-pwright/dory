/* <dory/input_dg/any_partition/any_partition_util.cc>

   ----------------------------------------------------------------------------
   Copyright 2013-2014 if(we)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   ----------------------------------------------------------------------------

   Implements <dory/input_dg/any_partition/any_partition_util.h>.
 */

#include <cassert>

#include <syslog.h>

#include <dory/input_dg/any_partition/any_partition_util.h>
#include <dory/input_dg/any_partition/v0/v0_input_dg_reader.h>
#include <dory/util/time_util.h>
#include <server/counter.h>

using namespace Capped;
using namespace Dory;
using namespace Dory::InputDg;
using namespace Dory::InputDg::AnyPartition;
using namespace Dory::Util;

SERVER_COUNTER(InputAgentDiscardAnyPartitionMsgUnsupportedApiVersion);
SERVER_COUNTER(InputAgentProcessAnyPartitionMsg);

TMsg::TPtr Dory::InputDg::AnyPartition::BuildAnyPartitionMsgFromDg(
    const uint8_t *dg_bytes, size_t dg_size, int16_t api_version,
    const uint8_t *versioned_part_begin, const uint8_t *versioned_part_end,
    TPool &pool, TAnomalyTracker &anomaly_tracker,
    TMsgStateTracker &msg_state_tracker, bool no_log_discard) {
  assert(dg_bytes);
  assert(versioned_part_begin > dg_bytes);
  assert(versioned_part_end > versioned_part_begin);
  InputAgentProcessAnyPartitionMsg.Increment();

  switch (api_version) {
    case 0: {
      return V0::TV0InputDgReader(dg_bytes, versioned_part_begin,
          versioned_part_end, pool, anomaly_tracker,
          msg_state_tracker, no_log_discard).BuildMsg();
    }
    default: {
      break;
    }
  }

  anomaly_tracker.TrackUnsupportedMsgVersionDiscard(dg_bytes,
      dg_bytes + dg_size, api_version);
  InputAgentDiscardAnyPartitionMsgUnsupportedApiVersion.Increment();

  if (!no_log_discard) {
    static TLogRateLimiter lim(std::chrono::seconds(30));

    if (lim.Test()) {
      syslog(LOG_ERR,
          "Discarding AnyPartition message with unsupported API version: %d",
          static_cast<int>(api_version));
    }
  }

  return TMsg::TPtr();
}
