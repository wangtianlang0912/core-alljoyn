/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <ApplicationMonitor.h>
#include <BusAttachment.h>
#include <Common.h>

#include <algorithm>
#include <iostream>

#include <SecLibDef.h>

#include <vector>
#include <qcc/Debug.h>

#define QCC_MODULE "SEC_MGR"
#define SECINFO_MEMBER "SecInfo"
#define SECINFO_SIG "(ayay)ya(ayay)"
#define SECINFO_ARGS "publicKey, claimableState, rotPublicKeys"

using namespace ajn;
using namespace ajn::services;
using namespace ajn::securitymgr;

ApplicationMonitor::ApplicationMonitor(ajn::BusAttachment* ba) :
    pinger(new AutoPinger(*ba)), busAttachment(ba)
{
    QStatus status = ER_FAIL;
    InterfaceDescription* intf;

    do {
        if (NULL == busAttachment) {
            QCC_LogError(status, ("NULL busAttachment !"));
            return;
        }

        if (pinger == NULL) {
            QCC_LogError(status, ("Could not get a pinger !"));
            return;
        }
        pinger->AddPingGroup(qcc::String(AUTOPING_GROUPNAME), *this, 5);

        status = busAttachment->CreateInterface(INFO_INTF_NAME, intf);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface '%s' on securitymgr bus attachment", INFO_INTF_NAME));
            return;
        }

        if (NULL == intf) {
            QCC_LogError(status, ("NULL intf !"));
            return;
        }

        intf->AddSignal(SECINFO_MEMBER, SECINFO_SIG, SECINFO_ARGS, 0);
        intf->Activate();

        status = busAttachment->RegisterSignalHandler(
            this, static_cast<MessageReceiver::SignalHandler>(&ApplicationMonitor::StateChangedSignalHandler),
            intf->GetMember(SECINFO_MEMBER), NULL);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to register a security signal handler."));
            break;
        }

        status = busAttachment->AddMatch(
            "type='signal',interface='" INFO_INTF_NAME "',member='" SECINFO_MEMBER "',sessionless='t'");
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to add match rule for security info signal."));
            break;
        }
    } while (0);
}

ApplicationMonitor::~ApplicationMonitor()
{
    delete pinger;
}

void ApplicationMonitor::StateChangedSignalHandler(const InterfaceDescription::Member* member,
                                                   const char* sourcePath,
                                                   Message& msg)
{
    QStatus status = ER_FAIL;

    PrettyPrintStateChangeSignal(sourcePath, msg);
    qcc::String busName = qcc::String(msg->GetSender());
    std::map<qcc::String, SecurityInfo>::iterator it = applications.find(busName);

    AllJoynArray array = msg->GetArg(2)->v_array;
    size_t rotCount = array.GetNumElements();
    std::vector<PublicKey> rotList;
    const MsgArg* rots = array.GetElements();
    for (size_t i = 0; i < rotCount; i++) {
        uint8_t* xCoord;
        size_t xLen;
        uint8_t* yCoord;
        size_t yLen;

        status = rots[i].Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
        if (ER_OK != status) {
            QCC_LogError(status, ("Invalid RoT."));
            return;
        }
        if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
            QCC_LogError(status, ("Invalid RoT."));
            return;
        }
        PublicKey rot;
        rot.SetData(xLen, xCoord, yLen, yCoord);
        rotList.push_back(rot);
    }

    if (it != applications.end()) {
        /* we already know this application */
        SecurityInfo old = it->second;

        uint8_t* xCoord;
        size_t xLen;
        uint8_t* yCoord;
        size_t yLen;

        status = msg->GetArg(0)->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);

        if (ER_OK != status) {
            QCC_LogError(status, ("Invalid public key."));
            return;
        }
        if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
            QCC_LogError(status, ("Invalid public key."));
            return;
        }

        it->second.publicKey.SetData(xLen, xCoord, yLen, yCoord);
        it->second.runningState = ApplicationRunningState::RUNNING;
        it->second.claimState = ToClaimState(msg->GetArg(1)->v_byte);
        it->second.rotList = rotList;
        for (SecurityInfoListener* listener : listeners) {
            listener->OnSecurityStateChange(&old, &it->second);
        }
    } else {
        SecurityInfo old;
        old.busName = busName;
        old.runningState = ApplicationRunningState::NOT_RUNNING;
        old.claimState = ApplicationClaimState::UNKNOWN_CLAIM_STATE;
        old.rotList = rotList;
        SecurityInfo info;
        info.busName = busName;
        info.runningState = ApplicationRunningState::RUNNING;
        info.claimState = ToClaimState(msg->GetArg(1)->v_byte);

        uint8_t* xCoord;
        size_t xLen;
        uint8_t* yCoord;
        size_t yLen;

        status = msg->GetArg(0)->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);

        if (ER_OK != status) {
            QCC_LogError(status, ("Invalid public key."));
            return;
        }
        if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
            QCC_LogError(status, ("Invalid public key."));
            return;
        }

        info.publicKey.SetData(xLen, xCoord, yLen, yCoord);

        info.rotList = rotList;
        old.publicKey = info.publicKey;
        applications[busName] = info;
        for (SecurityInfoListener* listener : listeners) {
            listener->OnSecurityStateChange(&old, &info);
        }
        pinger->AddDestination(AUTOPING_GROUPNAME, busName);
    }
}

;

std::vector<SecurityInfo> ApplicationMonitor::GetApplications() const
{
    if (!applications.empty()) {
        std::vector<SecurityInfo> apps;
        std::map<qcc::String, SecurityInfo>::const_iterator it = applications.begin();
        for (; it != applications.end(); ++it) {
            const SecurityInfo& appInfo = it->second;
            apps.push_back(appInfo);
        }
        return apps;
    }
    return std::vector<SecurityInfo>();
}

void ApplicationMonitor::RegisterSecurityInfoListener(SecurityInfoListener* al)
{
    if (NULL != al) {
        listeners.push_back(al);
        std::map<qcc::String, SecurityInfo>::const_iterator it = applications.begin();
        for (; it != applications.end(); ++it) {
            const SecurityInfo& appInfo = it->second;
            al->OnSecurityStateChange(&appInfo, &appInfo);
        }
    }
}

void ApplicationMonitor::UnregisterSecurityInfoListener(SecurityInfoListener* al)
{
    std::vector<SecurityInfoListener*>::iterator it = std::find(listeners.begin(), listeners.end(), al);
    if (listeners.end() != it) {
        listeners.erase(it);
    }
}

void ApplicationMonitor::DestinationLost(const qcc::String& group, const qcc::String& destination)
{
    //printf("DestinationLost %s\n", destination.data());
    std::map<qcc::String, SecurityInfo>::iterator it = applications.find(destination);

    if (it != applications.end()) {
        /* we already know this application */
        if (it->second.runningState != ApplicationRunningState::NOT_RUNNING) {
            SecurityInfo old = it->second;
            it->second.runningState = ApplicationRunningState::NOT_RUNNING;
            for (SecurityInfoListener* listener : listeners) {
                listener->OnSecurityStateChange(&old, &it->second);
            }
        }
    } else {
        /* We are monitoring an app not in the list. Remove it. */
        pinger->RemoveDestination(AUTOPING_GROUPNAME, destination);
    }
}

void ApplicationMonitor::DestinationFound(const qcc::String& group, const qcc::String& destination)
{
    //printf("DestinationFound %s\n", destination.data());
    std::map<qcc::String, SecurityInfo>::iterator it = applications.find(destination);

    if (it != applications.end()) {
        /* we already know this application */
        if (it->second.runningState != ApplicationRunningState::RUNNING) {
            SecurityInfo old = it->second;
            it->second.runningState = ApplicationRunningState::RUNNING;
            for (SecurityInfoListener* listener : listeners) {
                listener->OnSecurityStateChange(&old, &it->second);
            }
        }
    } else {
        /* We are monitoring an app not in the list. Remove it. */
        pinger->RemoveDestination(AUTOPING_GROUPNAME, destination);
    }
}

#undef QCC_MODULE
