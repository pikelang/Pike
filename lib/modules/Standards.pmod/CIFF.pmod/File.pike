#pike __REAL_VERSION__

import ".";

Stdio.File fd;

array(Record) records;
mapping header;

// --- internal use ---
string read4c,read2c;
#define READ32(fd) (array_sscanf(fd->read(4),read4c)[0])
#define READ16(fd) (array_sscanf(fd->read(2),read2c)[0])
// --- 


void create(string|Stdio.File file)
{
   if (stringp(file)) error("no pseudofile class yet\n");
   fd=file;

   getheader();

   records=getrecords(header->headerLength,
		      fd->stat()->size-header->headerLength);   
   werror("%O\n",records);
   records[0]->read_d30_sensor_data();
}

void getheader()
{
   fd->seek(0);
   switch (fd->read(2))
   {
      case "II": read4c="%-4c"; read2c="%-2c"; break;
      case "MM": read4c="%4c"; read2c="%2c"; break;
      default: error("Not a CIFF file: Illegal header\n");
   }
   fd->seek(0);
   array v=array_sscanf(fd->read(26),"%2s"+read4c+"%4s%4s"+read4c+"%4s%4s");
   if (sizeof(v)<7)
      error("Not a CIFF file: header too short\n");

   header=
      mkmapping("byteorder,headerLength,type,subType,version,"
		"reserved1,reserved2"/",",v);

   if (header->version!=0x10002)
      error("CIFF file: unknown version %d.%d (expected 1.2)\n",
	    header->version>>16, header->version&0xffff);
}

array(Record) getrecords(int offset,int len)
{
   fd->seek(offset+len-4);
   int offsetTblOffset=READ32(fd);
   fd->seek(offsetTblOffset+offset);
   int numRecords=READ16(fd);

   array(Record) res=({});
   while (numRecords--)
   {
      string s;
      array v=
	 array_sscanf(s=fd->read(10),read2c+read4c+read4c);

      if (v[0]&kStgFormatMask==kStg_InRecordEntry)
	 res+=({Record(v[0],s[2..])});
      else
	 res+=({Record(offset,@v)});
   }
   return res;
}

class Record
{
   int heapoffset;
   int type;
   int offset;
   int length;
   string data;

   int storage;
   int datatype;
   int typeid;
   int idcode;

   int create(int|string ... v)
   {
      if (sizeof(v)==2)
	 [type,data]=v;
      else
	 [heapoffset,type,length,offset]=v;

      storage=type&kStgFormatMask;
      datatype=type&kDataTypeMask;
      typeid=type&kTypeIDCodeMask;
      idcode=type&kIDCodeMask;
      _o=0;
   }

   string _sprintf(int t,mapping z)
   {
     if(t=='O')
       return sprintf(
         "%O(%s:%s%s)", this_program,
         _stgtoid[storage]||sprintf("%xh",storage),
//       _ctoid[datatype]||sprintf("%xh",datatype),
         _dttoid[typeid]||sprintf("%xh",typeid),
         (storage==kStg_InRecordEntry)?"":
           sprintf(" %d+%d",offset,length)
         );
   }

   void seek(void|int n)
   {
      if (storage!=kStg_InRecordEntry)
	 fd->seek(heapoffset+offset+n);
      _o=n;
   }

   int _o;

   string read(void|int n)
   {
      string res;
      if (storage==kStg_InRecordEntry)
	 res=n?data[_o.._o+n-1]:data[_o..];
      else
      {
	 seek(_o);
	 if (!n) n=length-_o;
	 if (n<0) res="";
	 else res=fd->read(n);
      }
      _o+=n;
      return res;
   }

   mapping(string:mixed) read_d30_sensor_data()
   {
      seek();
      mapping z=([
	 "low_bits":read(808980),
	 "unknown":read(1070),
	 "compressed":read()
      ]);

      return z;
   }
}


