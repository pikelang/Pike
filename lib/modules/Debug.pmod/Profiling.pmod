/* -*- Mode: pike; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#if constant(get_profiling_info)
private multiset(program) object_programs() 
{
    multiset x = (<>);
    int orig_enabled = Pike.gc_parameters()->enabled;
    Pike.gc_parameters( (["enabled":0]) );
    program prog = this_program;

    void add_traverse( program start, function traverse )
    {
        while(1)
        {
            x[start] = 1;
            program next_prog;
            if(catch(next_prog=traverse(start)))
                break;
            start = next_prog;
        }
    };

    add_traverse( _next(prog), _next );
    add_traverse( prog, _prev );
    Pike.gc_parameters( (["enabled":orig_enabled]) );

    return x;
}

private string remove_cwd(string from)
{
    if( !from ) return 0;

    if( has_prefix( from, getcwd() ) )
        from = from[strlen(getcwd())..];
    return from;
}

private string normalize_name( string from )
{
    from = remove_cwd( from );
    from = replace( from, ".pmod/", "." );
    from = replace( from, ".pmod", "" );
    from = replace( from, ".pike", "" );
    from = replace( from, ".so", "" );
    from = replace( from, "()->", "." );
    return from;
}

private array (program) all_modules()
{
    return (array)object_programs();
}

private string filter_obj_name( string x )
{
    string a, b;
    while( sscanf( x, "%s(%*s)%s", a, b ) == 3 )
        x = a+b;
    return x;
}

private mapping(program|object:string) pnc = set_weak_flag(([]),Pike.WEAK_INDICES);
private string program_name(program|object what)
{
    if( pnc[what] ) return pnc[what];
    string p;
    if( (p = search(master()->programs,what)) && p != "main") 
        return pnc[what] = normalize_name(p);
    return pnc[what] = filter_obj_name(sprintf("%O",what)-".pike");
}

private string program_function_file( program prog, string key )
{
    return remove_cwd(Program.defined(prog, key)||"-");
}

private mapping get_prof(bool avoid_overhead)
{
    mapping res = ([]);
    foreach(all_modules(), program prog)
    {
        if( prog && programp( prog ) )
        {
            string n = pnc[prog] || program_name(prog);
            int cnt = 1;
            array pinf = get_profiling_info( prog );
            /* clones, ([ info }) */
            if( !avoid_overhead )
                foreach( pinf[1]; string key; array row )
                    pinf[1][key] += ({program_function_file(prog,key)});
            while( res[n] )
                n=pnc[prog]+"<"+(++cnt)+">";
            res[n] = pinf;
        }
    }
    return res;
}


private string trim_obj( string x )
{
    sscanf( x, "/mod/%s", x );
    sscanf( x, "/modules/%s", x );
    if( has_prefix( x, "/main." ) )
        x = "Backend"+x[5..];

    int l;
    if( sscanf( x, "%s__lambda_%*s_line_%d", x, l ) )
        x = x+" @ "+l;
    x = replace( x, "().", "." );
    return x;
};

private mapping oi = ([]);
private int last_cpu_used;
private mapping(string:mapping(string:string)) nc = ([]);

private mapping low_get_prof_info( bool avoid_overhead )
{
    mapping as_functions = ([]);
    mapping tmp = get_prof(avoid_overhead);

    foreach(indices(tmp), string c)
    {
        // if( has_prefix( c, "/master" ) ) 
        //     continue;
        if( has_value( c, "Profiling" ) )
            continue;
        mapping g = tmp[c][1];
        mapping ce = nc[c] || (nc[c]=([]));

        foreach(indices(g), string f)
        {
            string fn = ce[f];
            if( !fn )
            {
                fn = c+"."+f;
                if( has_prefix( f, "__lambda" ) )
                    fn = c+".lambda @ "+(f/"_")[-1];
                switch( f )
                {
                    case "cast":   fn = "(cast)"+c; break;
                    case "__INIT": fn = c;          break;
                    case "create": case "`()": fn = c+"()"; break;
                    case "`->":    fn = c+"->"; break;
                    case "`[]":    fn = c+"[]"; break;
                }
                fn = replace( fn, "()->", ".");
                fn = replace( fn, "->", ".");
                fn = replace( fn, "_static_modules.", "" );
                fn = replace( fn, ".Builtin", "" );
                fn = trim_obj( fn );
                fn = replace( fn, "\0", "");
                ce[f] = fn;
            }
            if( !has_suffix(fn,"Program.defined" ) )
                as_functions[fn] = ({ g[f][2],g[f][0],g[f][1],g[f][-1] });
        }
    }
    return as_functions;
}


private void hide_compile_time()
{
    get_prof_info();
}
private mixed __when_pike_backend_started = call_out( hide_compile_time, 0.0 );

//! Collect profiling data.
//!
//! This will return the CPU usage, by function, since the last time
//! the function was called.
//!
//! The returned array contains the following entries per entry:
//! @array
//! @item string name
//!   The name of the function
//! @item float number_of_calls
//! @item float self_time
//! @item float cpu_time
//! @item float self_time%
//! @item float cpu_time%
//! @item string function_line
//!   This is the location in the source of the start of the function
//! @endarray
array(array(string|float|int)) get_prof_info(string|array(string)|void include,
                                             string|array(string)|void exclude)
{
    array res = ({});
    int time_passed = gethrvtime()-last_cpu_used;
    mapping as_functions = low_get_prof_info( false );
    foreach( as_functions; string key; array v )
    {
        if( oi[key] )
        {
            v[0] -= oi[key][0];
            v[1] -= oi[key][1];
            v[2] -= oi[key][2];
        }
        if( v[1] && (v[0]||v[2]) )
        {
            res +=
                ({
                    ({key,
                      v[1],
                      (float)v[0],
                      (float)v[2],
                      (float)(v[0])/v[1] * 1000,
                      (float)(v[2])/v[1] * 1000,
                      (int)((v[0]*100) / (time_passed/1000.0)),
                      (int)((v[2]*100) / (time_passed/1000.0)),
                      v[-1],
                    })
                });
        }
    }
    if( include && sizeof(include) )
        res = filter( res, lambda( array q ) { return glob(include,q[0]); });

    if( exclude && sizeof(exclude) )
        res = filter( res, lambda( array q ) { return !glob(exclude,q[0]); });

    /* In order to avoid counting the act of profiling, get information again: */
    oi = low_get_prof_info( true );
    last_cpu_used = gethrvtime();
    return res;
}

int name_column_width = 32;

private string format_number(string|int|float x )
{
    if( stringp( x ) )
    {
        if( x[0..6] == "/master" )
            x = x[1..];
        x = replace((x-" "),"@",":");
        if( strlen( x ) > name_column_width )
            x = x[..5] + "[..]" + x[<(name_column_width-11)..];
        return x;
    }
    if( intp( x ) )
    {
        if( x > 100000 )
            return x/1000+"k";
        return (string)x;
    }

    constant units = ({ "k", "M", "G", "T", "" });
    int unit = -1;
    while( x >= 1000.0 )
    {
        x/= 1000.0;
        unit++;
    }

    if( (float)(int)x == x && (int)x == 0 )
        return "-";

    return sprintf("%.1f%s",x,units[unit] );
}


private void output_result( array rows, int percentage_column )
{
    string line = "-"*(79-32+name_column_width)+"\n";
    werror(line);
    werror("%-"+name_column_width+"s %7s %7s %7s %5s %7s %7s\n",
           "Function", "Calls", "Time","+Child","%", "/call", "ch/call" );
    werror("%-"+name_column_width+"s %7s %7s %7s %5s %7s %7s\n",
           "", "", "ms", "ms", "", "us", "us" );
    werror(line);
    foreach( rows, array r )
        werror( "%[0]-"+name_column_width+"s %[1]7s %[2]7s %[3]7s"
                " %["+percentage_column+"]5s %[4]7s %[5]7s\n",
                @format_number(r[*]) );
    werror(line);
}


//! Show profiling information in a more-or-less readable manner.
//! This only works if pike has been compiled with profiling support.
//!
//! The function will print to stderr using werror.
//!
//! This is mainly here for use from the @[Debug.Watchdog] class, if
//! you want to do your own formatting or output to some other channel
//! use @[get_prof_info] instead.
void display(int|void num,
             string|array(string)|void pattern,
             string|array(string)|void exclude)
{
    // Show top num.
    if( !exclude )
    {
        // Do not count ourselves by default.
        exclude = ({"Debug.*",
                    "/master.describe_program",
                    "/master.describe_module",
                    "/master.describe_function",
                    "/master.describe_backtrace",
                    "/master.program_path_to_name",
                    "/master.describe_object"
                  });
    }
    array rows = get_prof_info( pattern,exclude );
    sort( (array(float))column(rows,1+(__MINOR__==6?0:2)), rows );
    if( pattern )
        rows = filter( rows, lambda(array row) {
                                 return glob(pattern, row[0]);
                             });
    werror("Most CPU-consuming functions "
           "sorted by total (self + child) times.\n");
    output_result( reverse(rows)[..num||99], 7 );

    werror("\n"
           "Most CPU-consuming functions sorted by self-time.\n");
    sort( (array(float))column(rows,1+(__MINOR__==6?0:1)), rows );
    output_result( reverse(rows)[..num||99], 6 );
}
#else
constant this_program_does_not_exist = 1;
#endif
