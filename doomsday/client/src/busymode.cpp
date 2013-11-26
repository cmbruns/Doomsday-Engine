/** @file busymode.cpp Busy Mode 
 * @ingroup base
 *
 * @authors Copyright © 2007-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2007-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 2007 Jamie Jones <jamie_jones_au@yahoo.com.au>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#define DENG_NO_API_MACROS_BUSY

#include "de_base.h"
#include "de_platform.h"
#include "de_console.h"
#include "de_graphics.h"
#include "de_network.h"

#include "audio/s_main.h"
#include "ui/busyvisual.h"

#include <de/c_wrapper.h>
#include <de/Log>
#include <QEventLoop>

#ifdef __CLIENT__
#include "clientapp.h"
#include "ui/windowsystem.h"
#include "ui/widgets/busywidget.h"

static void BusyMode_Exit(void);

static QEventLoop* eventLoop;
static volatile boolean busyDoneCopy;
static timespan_t busyTime;
static boolean busyWillAnimateTransition;
static boolean busyWasIgnoringInput;

#endif // __CLIENT__

static boolean busyInited;
static volatile boolean busyDone;

static mutex_t busy_Mutex; // To prevent Data races in the busy thread.

static BusyTask* busyTask; // Current task.
static thread_t busyThread;
static timespan_t accumulatedBusyTime; // Never cleared.
static boolean busyTaskEndedWithError;
static char busyError[256];

#ifdef __CLIENT__
static boolean animatedTransitionActive(int busyMode)
{
    return (!novideo && !isDedicated && !netGame && !(busyMode & BUSYF_STARTUP) &&
            rTransitionTics > 0 && (busyMode & BUSYF_TRANSITION));
}
#endif

boolean BusyMode_Active(void)
{
    return busyInited;
}

timespan_t BusyMode_ElapsedTime(void)
{
    if(!BusyMode_Active()) return 0;
    return accumulatedBusyTime;
}

boolean BusyMode_IsWorkerThread(uint threadId)
{
    boolean result;
    if(!BusyMode_Active() || !busyThread) return false;

    /// @todo Is locking necessary?
    Sys_Lock(busy_Mutex);
    result = (Sys_ThreadId(busyThread) == threadId);
    Sys_Unlock(busy_Mutex);
    return result;
}

boolean BusyMode_InWorkerThread(void)
{
    return BusyMode_IsWorkerThread(Sys_CurrentThreadId());
}

BusyTask* BusyMode_CurrentTask(void)
{
    if(!BusyMode_Active()) return NULL;
    return busyTask;
}

#ifdef __CLIENT__

/**
 * Callback that is called from the busy worker thread when it exists.
 * @param status Exit status.
 */
static void busyWorkerTerminated(systhreadexitstatus_t status)
{
    DENG_ASSERT(BusyMode_Active());

    if(status == DENG_THREAD_STOPPED_WITH_EXCEPTION)
    {
        _api_Busy.WorkerError("Uncaught exception from busy thread.");
    }
}

/**
 * Sets up module state for running a busy task. After this the busy mode event
 * loop is started. The loop will run until the worker thread exits.
 */
static void beginTask(BusyTask* task)
{
    DENG_ASSERT(task);

    if(!busyInited)
    {
        busy_Mutex = Sys_CreateMutex("BUSY_MUTEX");
    }
    if(busyInited)
    {
        Con_Error("Con_Busy: Already busy.\n");
    }

    BusyVisual_PrepareResources();

    Sys_Lock(busy_Mutex);
    busyDone = false;
    busyTaskEndedWithError = false;
    // This is now the current task.
    busyTask = task;
    Sys_Unlock(busy_Mutex);
    busyInited = true;

    ProgressWidget &prog = ClientWindow::main().busy().progress();
    prog.show();
    prog.setText(task->name);
    prog.setMode(task->mode & BUSYF_ACTIVITY? ProgressWidget::Indefinite :
                                              ProgressWidget::Ranged);

    // Start the busy worker thread, which will process the task in the
    // background while we keep the user occupied with nice animations.
    busyThread = Sys_StartThread(busyTask->worker, busyTask->workerData);
    Thread_SetCallback(busyThread, busyWorkerTerminated);

    busyTask->_startTime = Timer_RealSeconds();
}

/**
 * Called after the busy mode worker thread and the event (sub-)loop has been stopped.
 */
static void endTask(BusyTask* task)
{
    DENG_ASSERT(task);
    DENG_ASSERT_IN_MAIN_THREAD();

    if(verbose)
    {
        Con_Message("Con_Busy: Was busy for %.2lf seconds.", busyTime);
    }

    if(busyTaskEndedWithError)
    {
        Con_AbnormalShutdown(busyError);
    }

    if(busyWillAnimateTransition)
    {
        Con_TransitionBegin();
    }

    // Make sure that any remaining deferred content gets uploaded.
    if(!(task->mode & BUSYF_NO_UPLOADS))
    {
        GL_ProcessDeferredTasks(0);
    }

    Sys_DestroyMutex(busy_Mutex);
    busyInited = false;
}

#endif // __CLIENT__

/**
 * Runs the busy mode event loop. Execution blocks here until the worker thread exits.
 */
static int runTask(BusyTask* task)
{
    DENG_ASSERT(task);

#ifdef __CLIENT__

    // Let's get busy!
    beginTask(task);

    DENG_ASSERT(eventLoop == 0);

    // Run a local event loop since the primary event loop is blocked while
    // we're busy. This event loop is able to handle window and input events
    // just like the primary loop.
    eventLoop = new QEventLoop;
    int result = eventLoop->exec();
    delete eventLoop;
    eventLoop = 0;

    // Teardown.
    endTask(task);

    return result;

#else // __SERVER__

    // Don't bother to start a thread -- non-GUI mode.
    return task->worker(task->workerData);

#endif
}

#ifdef __CLIENT__
boolean BusyMode_IsTransitionAnimated(void)
{
    return busyWillAnimateTransition;
}
#endif

void BusyMode_FreezeGameForBusyMode(void)
{
#ifdef __CLIENT__
    ClientWindow::main().busy().renderTransitionFrame();
#endif
}

static void preBusySetup(int initialMode)
{
#ifdef __CLIENT__
    //ClientWindow::main().busy().renderTransitionFrame();

    // Are we doing a transition effect?
    busyWillAnimateTransition = animatedTransitionActive(initialMode);
    if(busyWillAnimateTransition)
    {
        Con_TransitionConfigure();
    }

    busyWasIgnoringInput = DD_IgnoreInput(true);

    // Load any resources needed beforehand.
    //BusyVisual_PrepareResources();

    //BusyVisual_PrepareFont();
    //BusyVisual_LoadTextures();

    // Limit frame rate to 60, no point pushing it any faster while busy.
    ClientApp::app().loop().setRate(60);   

    // Switch the window to busy mode UI.
    WindowSystem::main().setMode(ClientWindow::Busy);

#else
    DENG_UNUSED(initialMode);
#endif
}

static void postBusyCleanup()
{
#ifdef __CLIENT__
    // Discard input events so that any and all accumulated input events are ignored.
    DD_IgnoreInput(busyWasIgnoringInput);
    DD_ResetTimer();

    //BusyVisual_ReleaseTextures();

    // Back to unlimited frame rate.
    ClientApp::app().loop().setRate(0);

    // Switch the window to normal UI.
    WindowSystem::main().setMode(ClientWindow::Normal);

    if(!Con_TransitionInProgress())
    {
        ClientWindow::main().busy().releaseTransitionFrame();
    }
#endif
}

/**
 * @param mode          Busy mode flags @ref busyModeFlags
 * @param worker        Worker thread that does processing while in busy mode.
 * @param workerData    Data context for the worker thread.
 * @param taskName      Optional task name (drawn with the progress bar).
 */
static BusyTask* newTask(int mode, busyworkerfunc_t worker, void* workerData,
    const char* taskName)
{
    // Initialize the task.
    BusyTask* task = (BusyTask*) calloc(1, sizeof(*task));
    task->mode = mode;
    task->worker = worker;
    task->workerData = workerData;
    // Take a copy of the task name.
    if(taskName && taskName[0])
    {
        task->name = strdup(taskName);
    }
    return task;
}

static void deleteTask(BusyTask* task)
{
    DENG_ASSERT(task);
    if(task->name) free((void*)task->name);
    free(task);
}

int BusyMode_RunTasks(BusyTask* tasks, int numTasks)
{
    const char* currentTaskName = NULL;
    BusyTask* task;
    int i, mode;
    int result = 0;

    if(BusyMode_Active())
    {
        Con_Error("BusyMode: Internal error, already busy...");
        exit(1); // Unreachable.
    }

    if(!tasks || numTasks <= 0) return result; // Hmm, no work?

    // Pick the first task.
    task = tasks;

    int initialMode = task->mode;
    preBusySetup(initialMode);

    // Process tasks.
    for(i = 0; i < numTasks; ++i, task++)
    {
        // If no new task name is specified, continue using the name of the previous task.
        if(task->name)
        {
            if(task->name[0])
                currentTaskName = task->name;
            else // Clear the name.
                currentTaskName = NULL;
        }

        mode = task->mode;
        /// @todo Kludge: Force BUSYF_STARTUP here so that the animation of one task
        ///       is not drawn on top of the last frame of the previous.
        if(numTasks > 1)
        {
            mode |= BUSYF_STARTUP;
        }
        // kludge end

        // Null tasks are not processed (implicit success).
        if(!task->worker) continue;

        /**
         * Process the work.
         */
#ifdef __CLIENT__
        // Is the worker updating its progress?
        if(task->maxProgress > 0)
            Con_InitProgress2(task->maxProgress, task->progressStart, task->progressEnd);
#endif
        // Invoke the worker in a new thread.
        /// @todo Kludge: Presently a temporary local task is needed so that we can modify
        ///       the task name and mode flags.
        { BusyTask* tmp = newTask(mode, task->worker, task->workerData, currentTaskName);
        result = runTask(tmp);
        // We are now done with this task.
        deleteTask(tmp);

        if(result) break;
        }
        // kludge end.
    }

    postBusyCleanup();

    return result;
}

int BusyMode_RunTask(BusyTask* task)
{
    return BusyMode_RunTasks(task, 1);
}

int BusyMode_RunNewTaskWithName(int mode, busyworkerfunc_t worker, void* workerData, const char* taskName)
{
    BusyTask* task = newTask(mode, worker, workerData, taskName);
    int result = BusyMode_RunTask(task);
    deleteTask(task);
    return result;
}

int BusyMode_RunNewTask(int mode, busyworkerfunc_t worker, void* workerData)
{
    return BusyMode_RunNewTaskWithName(mode, worker, workerData, NULL/*no task name*/);
}

#ifdef __CLIENT__

/**
 * Ends the busy event loop and sets its return value. The loop callback, which
 * during busy mode points to the busy loop callback, is reset to NULL.
 */
static void stopEventLoopWithValue(int result)
{
    DENG_ASSERT(eventLoop != 0);
    eventLoop->exit(result);
}

/**
 * Exits the busy mode event loop. Called in the main thread, does not return
 * until the worker thread is stopped.
 */
static void BusyMode_Exit(void)
{
    int result;
    systhreadexitstatus_t status;

    DENG_ASSERT_IN_MAIN_THREAD();

    busyDone = true;

    // Make sure the worker finishes before we continue.
    result = Sys_WaitThread(busyThread, busyTaskEndedWithError? 100 : 5000, &status);
    busyThread = NULL;
    busyTask   = NULL;

    stopEventLoopWithValue(result);
}

/**
 * The busy loop callback function. Called periodically in the main (UI) thread
 * while the busy worker is running.
 */
void BusyMode_Loop(void)
{
    if(!busyTask || !BusyMode_Active()) return;

    boolean canUpload = !(busyTask->mode & BUSYF_NO_UPLOADS);
    timespan_t oldTime;

    // Post and discard all input events.
    DD_ProcessEvents(0);
    DD_ProcessSharpEvents(0);

    if(canUpload)
    {
        WindowSystem::main().glActivate();

        // Any deferred content needs to get uploaded.
        GL_ProcessDeferredTasks(15);
    }

    // We accumulate time in the busy loop so that the animation of a task
    // sequence doesn't jump around but remains continuous.
    oldTime = busyTime;
    busyTime = Timer_RealSeconds() - busyTask->_startTime;
    if(busyTime > oldTime)
    {
        accumulatedBusyTime += busyTime - oldTime;
    }

    Sys_Lock(busy_Mutex);
    busyDoneCopy = busyDone;
    Sys_Unlock(busy_Mutex);

    if(!busyDoneCopy || (canUpload && GL_DeferredTaskCount() > 0) ||
       !Con_IsProgressAnimationCompleted())
    {
        // Let's keep running the busy loop.
        WindowSystem::main().draw();
        return;
    }

    // Stop the loop.
    BusyMode_Exit();
}

#endif // __CLIENT__

void BusyMode_WorkerError(const char* message)
{
    busyTaskEndedWithError = true;
    strncpy(busyError, message, sizeof(busyError) - 1);
    _api_Busy.WorkerEnd();
}

void BusyMode_WorkerEnd(void)
{
    if(!busyInited) return;

    Sys_Lock(busy_Mutex);
    busyDone = true;
    Sys_Unlock(busy_Mutex);
}

DENG_DECLARE_API(Busy) =
{
    { DE_API_BUSY },
    BusyMode_Active,
    BusyMode_ElapsedTime,
    BusyMode_FreezeGameForBusyMode,
    BusyMode_RunTask,
    BusyMode_RunTasks,
    BusyMode_RunNewTask,
    BusyMode_RunNewTaskWithName,
    BusyMode_WorkerEnd,
    BusyMode_WorkerError
};
