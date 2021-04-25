/*
* //******************************************************************
* //
* // Copyright 2015 Samsung Electronics All Rights Reserved.
* //
* //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
* //
* // Licensed under the Apache License, Version 2.0 (the "License");
* // you may not use this file except in compliance with the License.
* // You may obtain a copy of the License at
* //
* //      http://www.apache.org/licenses/LICENSE-2.0
* //
* // Unless required by applicable law or agreed to in writing, software
* // distributed under the License is distributed on an "AS IS" BASIS,
* // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* // See the License for the specific language governing permissions and
* // limitations under the License.
* //
* //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/
#include <jni.h>
#include "JniOcStack.h"

#ifndef _Included_org_iotivity_base_OcCloudProvisioning_JniOcCloudResultListener
#define _Included_org_iotivity_base_OcCloudProvisioning_JniOcCloudResultListener

typedef std::function<void(JNIEnv* env, jobject jListener)> RemoveCallback;

enum class ListenerFunc
{
    REQUEST_CERTIFICATE = 1,
    GET_ACLINFO,
    GET_CRL,
    POST_CRL
};

class JniOcCloudResultListener
{
    public:
        JniOcCloudResultListener(JNIEnv *env, jobject jListener,
                RemoveCallback removeCloudResultListener);
        ~JniOcCloudResultListener();

        void CloudResultListenerCB(int result, void *data, ListenerFunc func);

    private:
        RemoveCallback m_removeCloudResultListener;
        jweak m_jwListener;
        void checkExAndRemoveListener(JNIEnv* env);
};
#endif
