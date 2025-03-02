/**
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <app/clusters/messages-server/messages-server.h>

#include <iostream>
#include <list>

#include <jni.h>
#include <lib/support/JniReferences.h>

class MessagesManager : public chip::app::Clusters::Messages::Delegate
{
public:
    static void NewManager(jint endpoint, jobject manager);
    void InitializeWithObjects(jobject managerObject);

    // Commands
    CHIP_ERROR HandlePresentMessagesRequest(
        const chip::ByteSpan & messageId, const chip::app::Clusters::Messages::MessagePriorityEnum & priority,
        const chip::BitMask<chip::app::Clusters::Messages::MessageControlBitmap> & messageControl,
        const chip::app::DataModel::Nullable<uint32_t> & startTime, const chip::app::DataModel::Nullable<uint16_t> & duration,
        const chip::CharSpan & messageText,
        const chip::Optional<
            chip::app::DataModel::DecodableList<chip::app::Clusters::Messages::Structs::MessageResponseOptionStruct::Type>> &
            responses) override;
    CHIP_ERROR HandleCancelMessagesRequest(const chip::app::DataModel::DecodableList<chip::ByteSpan> & messageIds) override;

    // Attributes
    CHIP_ERROR HandleGetMessages(chip::app::AttributeValueEncoder & aEncoder) override;
    CHIP_ERROR HandleGetActiveMessageIds(chip::app::AttributeValueEncoder & aEncoder) override;

    // Global Attributes
    uint32_t GetFeatureMap(chip::EndpointId endpoint) override;
    // uint16_t GetClusterRevision(chip::EndpointId endpoint) override;

private:
    chip::JniGlobalReference mMessagesManagerObject;
    jmethodID mGetMessagesMethod = nullptr;

    jmethodID mPresentMessagesMethod = nullptr;
    jmethodID mCancelMessagesMethod  = nullptr;

    // TODO: set this based upon meta data from app
    static constexpr uint32_t kEndpointFeatureMap = 15;
    // static constexpr uint16_t kClusterRevision    = 1;
};
