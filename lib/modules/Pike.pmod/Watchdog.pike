#pike __REAL_VERSION__

//! A Watchdog that ensures that the process is not hung for an
//! extended period of time. The definition of 'hung' is: Has not used
//! the default backend.
//!
//! An important and useful side-effect of this class is that the
//! process will start to respond to kill -QUIT by printing a lot of
//! debug information to stderr, including memory usage, and if pike
//! is compiled with profiling, the CPU used since the last time kill
//! -QUIT was called.



//! Create a new watchdog, with the intended delay.
//!
//! Even though the actual watchdog functionality is currently not
//! available on systems without sigalarm, such as Windows, the
//! functionality can still be triggered by adding probe functions
//!
//! @seealso
//!  @[add_probe()] and @[set_delay()]
protected void create(int t)
{
    delay = t;
#ifndef __NT__
#if constant(alarm)
    signal(signum("SIGALRM"), alarm_alarm_alarm);
#endif
    signal(signum("SIGQUIT"), print_debug);
#endif
    update_watchdog();
}

//! Set the watchdog interval to @[t] seconds.
//!
//! The change will not take effect until the previous probe has been
//! triggered.
void set_delay( int t )
{
    delay = t;
}

protected object backend_thread = this_thread();
protected int delay, last_seen;
protected int expected_cpu_time;
protected array(function(void:void)) debug_funcs = ({});
protected array(function(void:bool)) probes = ({});

//! Output thread stacktraces, memory and profiling (if available)
//! debug information to stderr.
void print_debug()
{
    // Disable all threads to avoid potential locking problems while we
    // have the backtraces. It also gives an atomic view of the state.
    object threads_disabled = _disable_threads();
    gc();

    Stdio.stderr.write("### Describing all Pike threads:\n\n");
    int n;
    foreach( all_threads(), Thread.Thread t )
    {
        array bt = t->backtrace();
        Stdio.stderr.write("### Thread %O%s:\n",
            t, t == backend_thread ? " (backend thread)" : "");
        Stdio.stderr.write(describe_backtrace(bt) + "\n");
        n++;
    }

    Stdio.stderr.write("Total %d Pike threads\n", n);

    gc();
    Stdio.stderr.write("\nMemory usage:\n\n");
    Stdio.stderr.write(Debug.pp_memory_usage()+"\n");

    gc();
    Stdio.stderr.write(Debug.pp_object_usage()+"\n");

#if constant(get_profiling_info)
    Debug.Profiling.display();
#endif

    foreach( debug_funcs, function f )
        catch(f());

    threads_disabled = 0;
}

//! Explicitly trigger the watchdog, if enough CPU time has been
//! used. This is not normally called manually.
void alarm_alarm_alarm()
{
#if constant(System.getrusage)
    if( System.getrusage()->utime < expected_cpu_time && (time()-last_seen) < delay*2 )
    {
        float cpus = (System.getrusage()->utime - expected_cpu_time + (delay*900))/1000.0;

        Stdio.stderr.write("\n"
                           "****************************************\n");
        Stdio.stderr.write("**        WATCHDOG PENDING.           **\n");
        Stdio.stderr.write("**   Last activity %4d seconds ago   **\n", time()-last_seen);
        Stdio.stderr.write("**   %4.1f CPU seconds have been used  **\n", cpus);
        Stdio.stderr.write("****************************************\n");
#if constant(alarm)
        alarm( 1 );
#endif
    }
#endif
    really_trigger_watchdog_promise();
}


//! Really trigger the watchdog, killing the current process.
//! This is not normally called manually.
void really_trigger_watchdog_promise()
{
    Stdio.stderr.write("\n\n***************************************************\n");
    Stdio.stderr.write("***************************************************\n");
    Stdio.stderr.write("*****            WATCHDOG TRIGGERED            ****\n");
    Stdio.stderr.write("*****       Last activity %4d seconds ago     ****\n", time()-last_seen);
    Stdio.stderr.write("***************************************************\n");
    Stdio.stderr.write("***************************************************\n");
    // Catch for paranoia reasons.
    catch(print_debug());
    _exit(1);
}


//! The function @[f] will be called if the watchdog triggers, before
//! the normal watchdog output is written.
void add_debug( function(void:void) f )
{
    debug_funcs += ({ f });
}

//! Add additional functions to be called each time the watchdog is
//! checked. If any of the probes return false, the watchdog will
//! trigger.
void add_probe( function(void:bool) f )
{
    probes += ({ f });
}

//! Tell the watchdog that all is well, and the CPU is not really
//! blocked.  Can be used during long calculations that block the
//! normal backend, note that this basically bypasses the main
//! functionality of the watchdog, that is, detecting blocked backend
//! threads.
void ping()
{
    if( (time()-last_seen) > delay/2.0 )
        update_watchdog();
}

protected void update_watchdog()
{
    last_seen = time();
    call_out( update_watchdog, min(delay/2.0,2.0) );

#if constant(System.getrusage)
    // Do not trigger if we have not been using CPU as well.
    expected_cpu_time = System.getrusage()->utime + delay*900;
#endif

    foreach( probes, function(void:bool) probe )
    {
        if( !probe() )
        {
            Stdio.stderr.write("%O did not return true.\n", probe);
            catch(really_trigger_watchdog_promise());
            _exit(1);
        }
    }
#if constant(alarm)
    alarm( delay );
#endif
}
