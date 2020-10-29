/*
 *  Copyright (c) 2020, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "link_metrics.h"

#include <openthread/link_metrics.h>

#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/pool.hpp"
#include "thread/link_quality.hpp"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

using namespace ot;

static int8_t sNoiseFloor; ///< Noise floor, currently it should be set to the radio receive sensitivity value

class LinkMetricsDataInfo : public LinkedListEntry<LinkMetricsDataInfo>, public Clearable<LinkMetricsDataInfo>
{
    friend class LinkedList<LinkMetricsDataInfo>;
    friend class LinkedListEntry<LinkMetricsDataInfo>;

public:
    /**
     * Construtor.
     *
     */
    LinkMetricsDataInfo(void) { Clear(); };

    /**
     * Set the information for this object.
     *
     * @param[in]  aLinkMetrics     Flags specifying what metrics to query.
     * @param[in]  aShortAddress    Short Address of the Probing Initiator tracked by this object.
     * @param[in]  aExtAddress      A reference to the Extended Address of the Probing Initiator tracked by this
     * object.
     *
     */
    void Set(const otLinkMetrics aLinkMetrics, const otShortAddress aShortAddress, const otExtAddress &aExtAddress)
    {
        mLinkMetrics  = aLinkMetrics;
        mShortAddress = aShortAddress;
        memcpy(mExtAddress.m8, aExtAddress.m8, sizeof(aExtAddress));
    }

    /**
     * This method adds a new received signal strength (RSS) value to the average.
     *
     * @param[in]  aRss    A new received signal strength value (in dBm) to be added to the average.
     *
     */
    void AddRss(int8_t aRss) { mRssAverager.Add(aRss); }

    /**
     * This method returns the current average received signal strength value.
     *
     * @returns The current average value or @c OT_RADIO_RSSI_INVALID if no average is available.
     *
     */
    int8_t GetAverageRss(void) const { return mRssAverager.GetAverage(); }

    /**
     * This method adds a link quality indicator (LQI) value to the average.
     *
     * @param[in]  aLqi    Link Quality Indicator value to be added to the average.
     *
     */
    void AddLqi(uint8_t aLqi) { mLqiAverager.Add(aLqi); }

    /**
     * This method gets the average LQI.
     *
     * @returns  The average LQI.
     *
     */
    uint8_t GetAverageLqi(void) const { return mLqiAverager.GetAverage(); }

    /**
     * This method gets the average Link Margin.
     *
     * @returns  The average Link Margin.
     *
     */
    uint8_t GetAverageLinkMargin(void) const
    {
        return LinkQualityInfo::ConvertRssToLinkMargin(sNoiseFloor, GetAverageRss());
    }

    /**
     * This method gets Link Metrics data stored in this object.
     *
     * @param[out]  aData    A pointer to the output buffer. @p aData MUST NOT be `nullptr`. The buffer should be larger
     * than 2 bytes at least. Otherwise the behavior would be undefined.
     *
     * @returns  The number of bytes written. If the writing fails, `0` would be returned.
     *
     */
    uint8_t GetEnhAckData(uint8_t *aData) const
    {
        uint8_t bytes = 0;

        VerifyOrExit(aData != nullptr);

        if (mLinkMetrics.mLqi)
        {
            aData[bytes++] = GetAverageLqi();
        }
        if (mLinkMetrics.mLinkMargin)
        {
            aData[bytes++] = static_cast<uint8_t>(GetAverageLinkMargin() * 255 /
                                                  130); // Linear scale Link Margin from [0, 130] to [0, 255]
        }
        if (bytes < 2 && mLinkMetrics.mRssi)
        {
            aData[bytes++] = static_cast<uint8_t>((GetAverageRss() + 130) * 255 /
                                                  130); // Linear scale RSSI from [-130, 0] to [0, 255]
        }

    exit:
        return bytes;
    }

    /**
     * This method gets the metrics configured for the Enhanced-ACK Based Probing.
     *
     * @returns  The metrics configured.
     *
     */
    otLinkMetrics GetLinkMetrics(void) const { return mLinkMetrics; }

private:
    LinkMetricsDataInfo *mNext;

    otLinkMetrics mLinkMetrics;

    otShortAddress mShortAddress;
    otExtAddress   mExtAddress;

    RssAverager mRssAverager;
    LqiAverager mLqiAverager;

    bool Matches(const otShortAddress &aShortAddress) const { return mShortAddress == aShortAddress; };

    bool Matches(const otExtAddress &aExtAddress) const
    {
        return memcmp(&mExtAddress, &aExtAddress, sizeof(otExtAddress)) == 0;
    };
};

enum
{
    kMaxEnhAckProbingInitiator = 10,
};

typedef Pool<LinkMetricsDataInfo, kMaxEnhAckProbingInitiator> LinkMetricsDataInfoPool;

typedef LinkedList<LinkMetricsDataInfo> LinkMetricsDataInfoList;

static LinkMetricsDataInfoPool &GetLinkMetricsDataInfoPool(void)
{
    static LinkMetricsDataInfoPool sDataInfoPool;
    return sDataInfoPool;
}

static LinkMetricsDataInfoList &GetLinkMetricsDataInfoActiveList(void)
{
    static LinkMetricsDataInfoList sDataInfoActiveList;
    return sDataInfoActiveList;
}

static inline bool IsLinkMetricsClear(const otLinkMetrics aLinkMetrics)
{
    return !aLinkMetrics.mPduCount && !aLinkMetrics.mLqi && !aLinkMetrics.mLinkMargin && !aLinkMetrics.mRssi;
}

void otLinkMetricsInit(const int8_t aNoiseFloor)
{
    sNoiseFloor = aNoiseFloor;
}

otError otLinkMetricsConfigureEnhAckProbing(const otShortAddress aShortAddress,
                                            const otExtAddress * aExtAddress,
                                            otLinkMetrics        aLinkMetrics)
{
    otError              error    = OT_ERROR_NONE;
    LinkMetricsDataInfo *dataInfo = nullptr;

    VerifyOrExit(aExtAddress != nullptr, error = OT_ERROR_INVALID_ARGS);

    if (IsLinkMetricsClear(aLinkMetrics)) ///< Remove the entry
    {
        dataInfo = GetLinkMetricsDataInfoActiveList().RemoveMatching(aShortAddress);
        VerifyOrExit(dataInfo != nullptr, error = OT_ERROR_NOT_FOUND);
        GetLinkMetricsDataInfoPool().Free(*dataInfo);
    }
    else
    {
        dataInfo = GetLinkMetricsDataInfoActiveList().FindMatching(aShortAddress);

        if (dataInfo == nullptr)
        {
            dataInfo = GetLinkMetricsDataInfoPool().Allocate();
            VerifyOrExit(dataInfo != nullptr, error = OT_ERROR_NO_BUFS);
            dataInfo->Clear();
            GetLinkMetricsDataInfoActiveList().Push(*dataInfo);
        }

        dataInfo->Set(aLinkMetrics, aShortAddress, *aExtAddress);
    }

exit:
    return error;
}

LinkMetricsDataInfo *GetLinkMetricsInfoByMacAddress(const otMacAddress *aMacAddress)
{
    LinkMetricsDataInfo *dataInfo = nullptr;
    VerifyOrExit(aMacAddress != nullptr);

    if (aMacAddress->mType == OT_MAC_ADDRESS_TYPE_SHORT)
    {
        dataInfo = GetLinkMetricsDataInfoActiveList().FindMatching(aMacAddress->mAddress.mShortAddress);
    }
    else if (aMacAddress->mType == OT_MAC_ADDRESS_TYPE_EXTENDED)
    {
        dataInfo = GetLinkMetricsDataInfoActiveList().FindMatching(aMacAddress->mAddress.mExtAddress);
    }

exit:
    return dataInfo;
}

void otLinkMetricsAggregateDataByMacAddress(const otMacAddress *aMacAddress, uint8_t aLqi, int8_t aRssi)
{
    LinkMetricsDataInfo *dataInfo = GetLinkMetricsInfoByMacAddress(aMacAddress);

    VerifyOrExit(dataInfo != nullptr);

    dataInfo->AddLqi(aLqi);
    dataInfo->AddRss(aRssi);

exit:
    return;
}

uint8_t otLinkMetricsEnhAckGetDataByMacAddress(const otMacAddress *aMacAddress, uint8_t *aData)
{
    uint8_t              bytes    = 0;
    LinkMetricsDataInfo *dataInfo = GetLinkMetricsInfoByMacAddress(aMacAddress);

    VerifyOrExit(dataInfo != nullptr);

    bytes = dataInfo->GetEnhAckData(aData);

exit:
    return bytes;
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
