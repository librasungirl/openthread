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

/**
 * @file
 *   This file implements managing DUA.
 */

#include "dua_manager.hpp"

#if (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_DUA_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "net/ip6_address.hpp"
#include "thread/thread_netif.hpp"
#include "utils/slaac_address.hpp"

namespace ot {

DuaManager::DuaManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mDadCounter(0)
{
    mDomainUnicastAddress.Clear();
    mDomainUnicastAddress.mPreferred          = true;
    mDomainUnicastAddress.mValid              = true;
    mDomainUnicastAddress.mScopeOverride      = Ip6::Address::kGlobalScope;
    mDomainUnicastAddress.mScopeOverrideValid = true;

    mFixedDuaInterfaceIdentifier.Clear();
}

void DuaManager::UpdateDomainUnicastAddress(BackboneRouter::Leader::DomainPrefixState aState)
{
    const otIp6Prefix *prefix;

    if ((aState == BackboneRouter::Leader::kDomainPrefixRemoved) ||
        (aState == BackboneRouter::Leader::kDomainPrefixRefreshed))
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mDomainUnicastAddress);
    }

    VerifyOrExit((aState == BackboneRouter::Leader::kDomainPrefixAdded) ||
                     (aState == BackboneRouter::Leader::kDomainPrefixRefreshed),
                 OT_NOOP);

    prefix = Get<BackboneRouter::Leader>().GetDomainPrefix();

    OT_ASSERT(prefix != NULL);

    mDomainUnicastAddress.mPrefixLength = prefix->mLength;
    mDomainUnicastAddress.GetAddress().Clear();
    mDomainUnicastAddress.GetAddress().SetPrefix(prefix->mPrefix.mFields.m8, prefix->mLength);

    // Apply cached DUA Interface Identifier manually specified.
    if (!mFixedDuaInterfaceIdentifier.IsUnspecified())
    {
        mDomainUnicastAddress.GetAddress().SetIid(mFixedDuaInterfaceIdentifier);
    }
    else
    {
        SuccessOrExit(GenerateDomainUnicastAddressIid());
    }

    Get<ThreadNetif>().AddUnicastAddress(mDomainUnicastAddress);

exit:
    return;
}

otError DuaManager::GenerateDomainUnicastAddressIid(void)
{
    otError error;

    if ((error = Get<Utils::Slaac>().GenerateIid(mDomainUnicastAddress, NULL, 0, &mDadCounter)) == OT_ERROR_NONE)
    {
        otLogInfoCore("Generated DUA: %s", mDomainUnicastAddress.GetAddress().ToString().AsCString());
    }
    else
    {
        otLogWarnCore("Failed to generate valid DUA");
    }

    return error;
}

otError DuaManager::SetFixedDuaInterfaceIdentifier(const Ip6::InterfaceIdentifier *aIid)
{
    otError error = OT_ERROR_NONE;

    // Clear the fixed Interface Identifier manually specified if there was.
    if (aIid == NULL)
    {
        // Nothing to clear.
        VerifyOrExit(!mFixedDuaInterfaceIdentifier.IsUnspecified(), OT_NOOP);

        if (GetDomainUnicastAddress().HasIid(mFixedDuaInterfaceIdentifier) &&
            Get<ThreadNetif>().IsUnicastAddress(GetDomainUnicastAddress()))
        {
            Get<ThreadNetif>().RemoveUnicastAddress(mDomainUnicastAddress);

            if (GenerateDomainUnicastAddressIid() == OT_ERROR_NONE)
            {
                Get<ThreadNetif>().AddUnicastAddress(mDomainUnicastAddress);
            }
        }

        otLogInfoCore("Cleared DUA IID: %s", mFixedDuaInterfaceIdentifier.ToString().AsCString());
        mFixedDuaInterfaceIdentifier.Clear();
    }
    else
    {
        VerifyOrExit(!aIid->IsReserved(), error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(mFixedDuaInterfaceIdentifier.IsUnspecified() || mFixedDuaInterfaceIdentifier != *aIid, OT_NOOP);

        mFixedDuaInterfaceIdentifier = *aIid;
        otLogInfoCore("Set DUA IID: %s", mFixedDuaInterfaceIdentifier.ToString().AsCString());

        if (Get<ThreadNetif>().IsUnicastAddress(GetDomainUnicastAddress()))
        {
            Get<ThreadNetif>().RemoveUnicastAddress(mDomainUnicastAddress);
            mDomainUnicastAddress.GetAddress().SetIid(mFixedDuaInterfaceIdentifier);
            Get<ThreadNetif>().AddUnicastAddress(mDomainUnicastAddress);
        }
    }

exit:
    return error;
}

const Ip6::InterfaceIdentifier *DuaManager::GetFixedDuaInterfaceIdentifier(void) const
{
    const Ip6::InterfaceIdentifier *iid = NULL;

    if (!mFixedDuaInterfaceIdentifier.IsUnspecified())
    {
        iid = &mFixedDuaInterfaceIdentifier;
    }

    return iid;
}

} // namespace ot

#endif // (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_DUA_ENABLE
