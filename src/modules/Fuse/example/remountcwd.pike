// Re-mount CWD somewhere else.
//
// $Id: remountcwd.pike,v 1.2 2005/04/29 18:39:49 grubba Exp $
// 
// To test: mkdir /tmp/test; pike test.pike -d /tmp/test

class CBS
{
    Stdio.Stat|int getattr( string path )
    {
	return file_stat( path );
    }
    
    int readdir( string path, function(string,int,int:void) callback )
    {
	catch {
	    foreach( get_dir(path ), string q )
	    {
// 		Stdio.Stat f = file_stat( "/home/ph/"+path+"/"+q );
		callback( q, 0,0 );// dtype[f->type], d->ino );
	    }
	    return 0;
	};
	return System.ENOENT;
    }

    mapping statfs(string path)
    {
	return filesystem_stat( path );
    }

    int|string read( string path, int len, int offset )
    {
	catch 
	{
	    Stdio.File fd = Stdio.File( path );
	    fd->seek( offset );
	    string res = fd->read( len );
	    return res;
	};
	return System.ENOENT;
    }
}

void main( int c, array v )
{
    Fuse.run( CBS(), 10, v );
}
