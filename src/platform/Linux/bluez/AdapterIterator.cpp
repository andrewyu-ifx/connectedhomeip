/*
 *
 *    Copyright (c) 2021-2022 Project CHIP Authors
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

#include "AdapterIterator.h"

#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/GLibTypeDeleter.h>
#include <platform/PlatformManager.h>

namespace chip {
namespace DeviceLayer {
namespace Internal {

AdapterIterator::~AdapterIterator()
{
    if (mManager != nullptr)
    {
        g_object_unref(mManager);
    }

    if (mObjectList != nullptr)
    {
        g_list_free_full(mObjectList, g_object_unref);
    }

    if (mCurrent.adapter != nullptr)
    {
        g_object_unref(mCurrent.adapter);
        mCurrent.adapter = nullptr;
    }
}

CHIP_ERROR AdapterIterator::Initialize(AdapterIterator * self)
{
    // When creating D-Bus proxy object, the thread default context must be initialized. Otherwise,
    // all D-Bus signals will be delivered to the GLib global default main context.
    VerifyOrDie(g_main_context_get_thread_default() != nullptr);

    CHIP_ERROR err = CHIP_NO_ERROR;
    GAutoPtr<GError> error;

    self->mManager = g_dbus_object_manager_client_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, BLUEZ_INTERFACE, "/",
        bluez_object_manager_client_get_proxy_type, nullptr /* unused user data in the Proxy Type Func */,
        nullptr /* destroy notify */, nullptr /* cancellable */, &error.GetReceiver());

    VerifyOrExit(self->mManager != nullptr, ChipLogError(DeviceLayer, "Failed to get DBUS object manager for listing adapters.");
                 err = CHIP_ERROR_INTERNAL);

    self->mObjectList      = g_dbus_object_manager_get_objects(self->mManager);
    self->mCurrentListItem = self->mObjectList;

exit:
    if (error != nullptr)
    {
        ChipLogError(DeviceLayer, "DBus error: %s", error->message);
    }

    return err;
}

bool AdapterIterator::Advance()
{
    if (mCurrentListItem == nullptr)
    {
        return false;
    }

    while (mCurrentListItem != nullptr)
    {
        BluezAdapter1 * adapter = bluez_object_get_adapter1(BLUEZ_OBJECT(mCurrentListItem->data));
        if (adapter == nullptr)
        {
            mCurrentListItem = mCurrentListItem->next;
            continue;
        }

        // PATH is of the for  BLUEZ_PATH / hci<nr>, i.e. like
        // '/org/bluez/hci0'
        // Index represents the number after hci
        const char * path = g_dbus_proxy_get_object_path(G_DBUS_PROXY(adapter));
        unsigned index    = 0;

        if (sscanf(path, BLUEZ_PATH "/hci%u", &index) != 1)
        {
            ChipLogError(DeviceLayer, "Failed to extract HCI index from '%s'", StringOrNullMarker(path));
            index = 0;
        }

        if (mCurrent.adapter != nullptr)
        {
            g_object_unref(mCurrent.adapter);
            mCurrent.adapter = nullptr;
        }

        mCurrent.index   = index;
        mCurrent.address = bluez_adapter1_get_address(adapter);
        mCurrent.alias   = bluez_adapter1_get_alias(adapter);
        mCurrent.name    = bluez_adapter1_get_name(adapter);
        mCurrent.powered = bluez_adapter1_get_powered(adapter);
        mCurrent.adapter = adapter;

        mCurrentListItem = mCurrentListItem->next;

        return true;
    }

    return false;
}

bool AdapterIterator::Next()
{
    if (mManager == nullptr)
    {
        CHIP_ERROR err = PlatformMgrImpl().GLibMatterContextInvokeSync(Initialize, this);
        VerifyOrReturnError(err == CHIP_NO_ERROR, false, ChipLogError(DeviceLayer, "Failed to initialize adapter iterator"));
    }

    return Advance();
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
