#include "hsize.h"

void main()
{

  multiset dont = (<
    /* Script Specifics */
    0x0958, 0x0959, 0x095A, 0x095B, 0x095C, 0x095D, 0x095E, 0x095F,
    0x09DC, 0x09DD, 0x09DF, 0x0A33, 0x0A36, 0x0A59, 0x0A5A, 0x0A5B,
    0x0A5E, 0x0B5C, 0x0B5D, 0x0F43, 0x0F4D, 0x0F52, 0x0F57, 0x0F5C,
    0x0F69, 0x0F76, 0x0F78, 0x0F93, 0x0F9D, 0x0FA2, 0x0FA7, 0x0FAC,
    0x0FB9, 0xFB1D, 0xFB1F, 0xFB2A, 0xFB2B, 0xFB2C, 0xFB2D, 0xFB2E,
    0xFB2F, 0xFB30, 0xFB31, 0xFB32, 0xFB33, 0xFB34, 0xFB35, 0xFB36,
    0xFB38, 0xFB39, 0xFB3A, 0xFB3B, 0xFB3C, 0xFB3E, 0xFB40, 0xFB41,
    0xFB43, 0xFB44, 0xFB46, 0xFB47, 0xFB48, 0xFB49, 0xFB4A, 0xFB4B,
    0xFB4C, 0xFB4D, 0xFB4E,

    /*  Post Composition Version precomposed characters */
    0x1D15E, 0x1D15F, 0x1D160, 0x1D161, 0x1D162, 0x1D163, 0x1D164,
    0x1D1BB, 0x1D1BC, 0x1D1BD, 0x1D1BE, 0x1D1BF, 0x1D1C0, 0x02ADC,

    /* Non-Starter Decompositions */
    0x344, 0xf73, 0xf75, 0xf81,
  >);

  mapping decompose = ([]);
  mapping compose = ([]);
  mapping compat = ([]);

  int last_faked = 0xe000;
  
  int fake_decompose( int c, array(int) cc )
  {
    if( sizeof( cc ) == 2 )
    {
      decompose[c] = cc;
      return c;
    }
    last_faked++;
    decompose[c] = ({ cc[0], fake_decompose( last_faked, cc[1..] ) });
    if( last_faked >= 0xf800 )
      error("Error: Faked decompose overflow!\n");
    return c;
  };

  foreach( Stdio.stdin.read()/"\n", string line )
  {
    sscanf( line, "%s#", line );
    if( !strlen( line ) )
      continue;
    array data = line / ";";
    if( sizeof( data ) != 15 )
      continue;
    int c;
    sscanf( data[0], "%x", c );
    catch {
      int strip_lt( string cd ) {
	if( cd[0] == '<' )  compat[c]=1;
	return cd[0] != '<';
      };
#if constant(Gmp.mpz)
      array cc = map( filter(data[5]/" "-({""}), strip_lt),Gmp.mpz, 16)-({0});
#else /* !constant(Gmp.mpz) */
      array cc = map( filter(data[5]/" "-({""}), strip_lt),
		      array_sscanf, "%x")*({})-({0});
#endif /* constant(Gmp.mpz) */
      
      if( sizeof( cc ) )
      {
	if( sizeof( cc ) > 2 )
	  fake_decompose( c, cc );
	else
	{
	  decompose[c] = cc+({ 0 });
	  if( sizeof(cc)==2 && !dont[c] && !compat[c] )
	    compose[(cc[0]<<32)|cc[1]] = c;
	}
      }
    };
  }

  write( "static const struct decomp _d[] = {\n" );
  mapping top=([]);
  int i;
  foreach( reverse(sort( indices( decompose ) )), int c )
  {
    write( "{%d,%d,{%d,%d}},\n",  c, compat[c], decompose[c][0], decompose[c][1] );
//     top[h] = i+1;
//     i++;
  }
  write( "};\n" );

//   write( "const struct decomp *decomp_hash[] = {\n");
//   for( int i = 0; i<HSIZE; i++ )
//     if( top[i] )
//       write( "_d+"+(top[i]-1)+",\n" );
//     else
//       write( "0,\n" );
//   write( "};\n" );



  write( "static const struct comp _c[] = {\n" );
  top=([]);
  i=0;

  foreach( indices( compose ), int c )
  {
    int c1 = (int)(c>>32);
    int c2 = (int)(c&0xffffffff);
//     int h = ((int)((c1<<16)|c2)) % HSIZE;
    write( "{%d,%d,%d},\n", c1, c2, (int)compose[c]);
//     top[h] = i+1;
//     i++;
  }
  write( "};\n" );

//   write( "const struct comp *comp_hash[] = {\n");
//   for( int i = 0; i<HSIZE; i++ )
//     if( top[i] )
//       write( "_c+"+(top[i]-1)+",\n" );
//     else
//       write( "0,\n" );
//   write( "};\n" );
}
