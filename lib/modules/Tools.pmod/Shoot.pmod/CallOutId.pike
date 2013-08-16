#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="call_out handling (with id)";

constant m = 5000; /* the target size of the mapping */

constant funs = ({ write, werror, file_stat, Stdio.cp, Array.uniq, master()->compile_error, Stdio.stdin->read, Stdio.stdout->write });

int perform()
{
   array(array) ids = allocate(m);
   for (int i=0; i<m; i++)
   {
       ids[i] = call_out(funs[i & 7], m+(i*((i&1)*2 - 1)));
   }

   for (int i = 0; i<m; i++) {
       find_call_out(ids[i]);
   }

   for (int i = 0; i<m; i++) {
       remove_call_out(ids[i]);
   }
   return m * 3;
}
