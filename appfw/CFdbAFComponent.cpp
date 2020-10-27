/*
 * Copyright (C) 2015   Jeremy Chen jeremy_cz@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <common_base/CFdbAFComponent.h>
#include <common_base/CMethodJob.h>
#include <common_base/CFdbAppFramework.h>
#include <common_base/CBaseClient.h>
#include <common_base/CBaseServer.h>

CFdbAFComponent::CFdbAFComponent(const char *name, CBaseWorker *worker)
{
    if (name)
    {
        mName = name;
    }
    if (worker)
    {
        mWorker = worker;
    }
    else
    {
        mWorker = CFdbAPPFramework::getInstance()->defaultWorker();
    }
}

class CQueryServiceJob : public CMethodJob<CFdbAFComponent>
{
public:
    CQueryServiceJob(CFdbAFComponent *comp,
                     const char *bus_name,
                     const CFdbEventDispatcher::CEvtHandleTbl &evt_tbl,
                     CFdbBaseObject::tConnCallbackFn &connect_callback,
                     CBaseClient *&client)
        : CMethodJob<CFdbAFComponent>(comp, &CFdbAFComponent::callQueryService, JOB_FORCE_RUN)
        , mBusName(bus_name)
        , mEvtTbl(evt_tbl)
        , mConnCallback(connect_callback)
        , mClient(client)
    {
    }
    const char *mBusName;
    const CFdbEventDispatcher::CEvtHandleTbl &mEvtTbl;
    CFdbBaseObject::tConnCallbackFn &mConnCallback;
    CBaseClient *&mClient;
};

void CFdbAFComponent::callQueryService(CBaseWorker *worker, CMethodJob<CFdbAFComponent> *job, CBaseJob::Ptr &ref)
{
    auto the_job = fdb_dynamic_cast_if_available<CQueryServiceJob *>(job);
    auto app_fw = CFdbAPPFramework::getInstance();
    auto client = app_fw->findClient(the_job->mBusName);
    if (!client)
    {
        client = new CBaseClient(CFdbAPPFramework::getInstance()->name().c_str(), worker);
        std::string url(FDB_URL_SVC);
        url += the_job->mBusName;
        client->connect(url.c_str());
        app_fw->registerClient(the_job->mBusName, client);
    }
    auto handle = client->registerConnNotification(the_job->mConnCallback, mWorker);
    mConnHandleTbl.push_back(handle);
    client->registerEventHandle(the_job->mEvtTbl, &mEventRegistryTbl);
    the_job->mClient = client;
}

CBaseClient *CFdbAFComponent::queryService(const char *bus_name,
                                            const CFdbEventDispatcher::CEvtHandleTbl &evt_tbl,
                                            CFdbBaseObject::tConnCallbackFn connect_callback)
{
    CBaseClient *client = 0;
    auto job = new CQueryServiceJob(this, bus_name, evt_tbl, connect_callback, client);
    CFdbAPPFramework::getInstance()->defaultWorker()->sendSyncEndeavor(job);
    return client;
}

class COfferServiceJob : public CMethodJob<CFdbAFComponent>
{
public:
    COfferServiceJob(CFdbAFComponent *comp,
                     const char *bus_name,
                     const CFdbMsgDispatcher::CMsgHandleTbl &msg_tbl,
                     CFdbBaseObject::tConnCallbackFn &connect_callback,
                     CBaseServer *&server)
        : CMethodJob<CFdbAFComponent>(comp, &CFdbAFComponent::callOfferService, JOB_FORCE_RUN)
        , mBusName(bus_name)
        , mMsgTbl(msg_tbl)
        , mConnCallback(connect_callback)
        , mServer(server)
    {
    }
    const char *mBusName;
    const CFdbMsgDispatcher::CMsgHandleTbl &mMsgTbl;
    CFdbBaseObject::tConnCallbackFn &mConnCallback;
    CBaseServer *&mServer;
};

void CFdbAFComponent::callOfferService(CBaseWorker *worker, CMethodJob<CFdbAFComponent> *job, CBaseJob::Ptr &ref)
{
    auto the_job = fdb_dynamic_cast_if_available<COfferServiceJob *>(job);
    auto app_fw = CFdbAPPFramework::getInstance();
    auto server = app_fw->findService(the_job->mBusName);
    if (!server)
    {
        server = new CBaseServer(CFdbAPPFramework::getInstance()->name().c_str(), worker);
        server->enableEventCache(true);
        std::string url(FDB_URL_SVC);
        url += the_job->mBusName;
        server->bind(url.c_str());
        app_fw->registerService(the_job->mBusName, server);
    }
    auto handle = server->registerConnNotification(the_job->mConnCallback, mWorker);
    mConnHandleTbl.push_back(handle);
    server->registerMsgHandle(the_job->mMsgTbl);
    the_job->mServer = server;
}

CBaseServer *CFdbAFComponent::offerService(const char *bus_name,
                                            const CFdbMsgDispatcher::CMsgHandleTbl &msg_tbl,
                                            CFdbBaseObject::tConnCallbackFn connect_callback)
{
    CBaseServer *server = 0;
    auto job = new COfferServiceJob(this, bus_name, msg_tbl, connect_callback, server);
    CFdbAPPFramework::getInstance()->defaultWorker()->sendSyncEndeavor(job);
    return server;
}
