/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <Qt3DSRenderThreadPool.h>
#include <Qt3DSThread.h>
#include <Qt3DSMutex.h>
#include <Qt3DSContainers.h>
#include <Qt3DSAtomic.h>
#include <Qt3DSBroadcastingAllocator.h>
#include <Qt3DSSync.h>
#include <Qt3DSMutex.h>
#include <Qt3DSPool.h>
#include <Qt3DSInvasiveLinkedList.h>

using namespace render;

namespace {
struct STask
{
    void *m_UserData;
    TTaskFunction m_Function;
    TTaskFunction m_CancelFunction;
    quint64 m_Id;
    TaskStates::Enum m_TaskState;
    STask *m_NextTask;
    STask *m_PreviousTask;

    STask(void *ud, TTaskFunction func, TTaskFunction cancelFunc, quint64 inId)
        : m_UserData(ud)
        , m_Function(func)
        , m_CancelFunction(cancelFunc)
        , m_Id(inId)
        , m_TaskState(TaskStates::Queued)
        , m_NextTask(nullptr)
        , m_PreviousTask(nullptr)
    {
    }
    STask()
        : m_UserData(nullptr)
        , m_Function(nullptr)
        , m_CancelFunction(nullptr)
        , m_Id(0)
        , m_TaskState(TaskStates::UnknownTask)
        , m_NextTask(nullptr)
        , m_PreviousTask(nullptr)
    {
    }
    void CallFunction()
    {
        if (m_Function)
            m_Function(m_UserData);
    }
    void Cancel()
    {
        if (m_CancelFunction)
            m_CancelFunction(m_UserData);
    }
};

struct STaskHeadOp
{
    STask *get(STask &inTask) { return inTask.m_PreviousTask; }
    void set(STask &inTask, STask *inItem) { inTask.m_PreviousTask = inItem; }
};

struct STaskTailOp
{
    STask *get(STask &inTask) { return inTask.m_NextTask; }
    void set(STask &inTask, STask *inItem) { inTask.m_NextTask = inItem; }
};

typedef InvasiveLinkedList<STask, STaskHeadOp, STaskTailOp> TTaskList;

class IInternalTaskManager
{
protected:
    virtual ~IInternalTaskManager() {}
public:
    virtual STask GetNextTask() = 0;
    virtual void TaskFinished(quint64 inId) = 0;
};

struct SThreadPoolThread : public Thread
{
    IInternalTaskManager &m_Mgr;
    SThreadPoolThread(NVFoundationBase &foundation, IInternalTaskManager &inMgr)
        : Thread(foundation)
        , m_Mgr(inMgr)
    {
    }
    void execute(void) override
    {
        setName("Qt3DSRender Thread manager thread");
        while (!quitIsSignalled()) {
            STask task = m_Mgr.GetNextTask();
            if (task.m_Function) {
                task.CallFunction();
                m_Mgr.TaskFinished(task.m_Id);
            }
        }
        quit();
    }
};

struct SThreadPool : public IThreadPool, public IInternalTaskManager
{
    typedef nvhash_map<quint64, STask *> TIdTaskMap;
    typedef Mutex::ScopedLock TLockType;
    typedef Pool<STask, ForwardingAllocator> TTaskPool;

    NVFoundationBase &m_Foundation;
    volatile qint32 mRefCount;
    nvvector<SThreadPoolThread *> m_Threads;
    TIdTaskMap m_Tasks;
    Sync m_TaskListEvent;
    volatile bool m_Running;
    Mutex m_TaskListMutex;
    TTaskPool m_TaskPool;
    TTaskList m_TaskList;

    quint64 m_NextId;

    SThreadPool(NVFoundationBase &inBase, quint32 inMaxThreads)
        : m_Foundation(inBase)
        , mRefCount(0)
        , m_Threads(inBase.getAllocator(), "SThreadPool::m_Threads")
        , m_Tasks(inBase.getAllocator(), "SThreadPool::m_Tasks")
        , m_TaskListEvent(inBase.getAllocator())
        , m_Running(true)
        , m_TaskListMutex(m_Foundation.getAllocator())
        , m_TaskPool(ForwardingAllocator(m_Foundation.getAllocator(), "SThreadPool::m_TaskPool"))
        , m_NextId(1)
    {
        // Fire up our little pools of chaos.
        for (quint32 idx = 0; idx < inMaxThreads; ++idx) {
            m_Threads.push_back(
                QDEMON_NEW(m_Foundation.getAllocator(), SThreadPoolThread)(m_Foundation, *this));
            m_Threads.back()->start(Thread::DEFAULT_STACK_SIZE);
        }
    }

    void MutexHeldRemoveTaskFromList(STask *theTask)
    {
        if (theTask)
            m_TaskList.remove(*theTask);
        Q_ASSERT(theTask->m_NextTask == nullptr);
        Q_ASSERT(theTask->m_PreviousTask == nullptr);
    }

    STask *MutexHeldNextTask()
    {
        STask *theTask = m_TaskList.front_ptr();
        if (theTask) {
            MutexHeldRemoveTaskFromList(theTask);
        }
        if (theTask) {
            Q_ASSERT(m_TaskList.m_Head != theTask);
            Q_ASSERT(m_TaskList.m_Tail != theTask);
        }
        return theTask;
    }

    virtual ~SThreadPool()
    {
        m_Running = false;

        m_TaskListEvent.set();

        for (quint32 idx = 0, end = m_Threads.size(); idx < end; ++idx)
            m_Threads[idx]->signalQuit();

        for (quint32 idx = 0, end = m_Threads.size(); idx < end; ++idx) {
            m_Threads[idx]->waitForQuit();
            NVDelete(m_Foundation.getAllocator(), m_Threads[idx]);
        }

        m_Threads.clear();

        TLockType __listMutexLocker(m_TaskListMutex);

        for (STask *theTask = MutexHeldNextTask(); theTask; theTask = MutexHeldNextTask()) {
            theTask->Cancel();
        }

        m_Tasks.clear();
    }

    QDEMON_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void VerifyTaskList()
    {
        STask *theLastTask = nullptr;
        for (STask *theTask = m_TaskList.m_Head; theTask; theTask = theTask->m_NextTask) {
            Q_ASSERT(theTask->m_PreviousTask == theLastTask);
            theLastTask = theTask;
        }
        theLastTask = nullptr;
        for (STask *theTask = m_TaskList.m_Tail; theTask; theTask = theTask->m_PreviousTask) {
            Q_ASSERT(theTask->m_NextTask == theLastTask);
            theLastTask = theTask;
        }
    }

    quint64 AddTask(void *inUserData, TTaskFunction inFunction,
                          TTaskFunction inCancelFunction) override
    {
        if (inFunction && m_Running) {
            TLockType __listMutexLocker(m_TaskListMutex);
            quint64 taskId = m_NextId;
            ++m_NextId;

            STask *theTask = (STask *)m_TaskPool.allocate(__FILE__, __LINE__);
            new (theTask) STask(inUserData, inFunction, inCancelFunction, taskId);
            TIdTaskMap::iterator theTaskIter =
                m_Tasks.insert(eastl::make_pair(taskId, theTask)).first;

            m_TaskList.push_back(*theTask);
            Q_ASSERT(m_TaskList.m_Tail == theTask);

#ifdef _DEBUG
            VerifyTaskList();
#endif
            m_TaskListEvent.set();
            m_TaskListEvent.reset();
            return taskId;
        }
        Q_ASSERT(false);
        return 0;
    }

    TaskStates::Enum GetTaskState(quint64 inTaskId) override
    {
        TLockType __listMutexLocker(m_TaskListMutex);
        TIdTaskMap::iterator theTaskIter = m_Tasks.find(inTaskId);
        if (theTaskIter != m_Tasks.end())
            return theTaskIter->second->m_TaskState;
        return TaskStates::UnknownTask;
    }

    CancelReturnValues::Enum CancelTask(quint64 inTaskId) override
    {
        TLockType __listMutexLocker(m_TaskListMutex);
        TIdTaskMap::iterator theTaskIter = m_Tasks.find(inTaskId);
        if (theTaskIter == m_Tasks.end())
            return CancelReturnValues::TaskCanceled;
        if (theTaskIter->second->m_TaskState == TaskStates::Running)
            return CancelReturnValues::TaskRunning;

        STask *theTask = theTaskIter->second;
        theTask->Cancel();
        MutexHeldRemoveTaskFromList(theTask);
        m_Tasks.erase(inTaskId);
        m_TaskPool.deallocate(theTask);

        return CancelReturnValues::TaskCanceled;
    }

    STask GetNextTask() override
    {

        if (m_Running) {
            {
                TLockType __listMutexLocker(m_TaskListMutex);
                STask *retval = MutexHeldNextTask();
                if (retval)
                    return *retval;
            }
            // If we couldn't get a task then wait.
            m_TaskListEvent.wait(1000);
        }
        return STask();
    }

    void TaskFinished(quint64 inId) override
    {
        TLockType __listMutexLocker(m_TaskListMutex);
        TIdTaskMap::iterator theTaskIter = m_Tasks.find(inId);
        if (theTaskIter == m_Tasks.end()) {
            Q_ASSERT(false);
            return;
        }

        STask *theTask(theTaskIter->second);

#ifdef _DEBUG
        Q_ASSERT(theTask->m_NextTask == nullptr);
        Q_ASSERT(theTask->m_PreviousTask == nullptr);
#endif
        m_TaskPool.deallocate(theTask);
        m_Tasks.erase(inId);
        return;
    }
};
}

IThreadPool &IThreadPool::CreateThreadPool(NVFoundationBase &inFoundation, quint32 inNumThreads)
{
    return *QDEMON_NEW(inFoundation.getAllocator(), SThreadPool)(inFoundation, inNumThreads);
}
