#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="call_out handling";

constant m = 5000; /* the target size of the mapping */

constant funs = ({ write, werror, file_stat, Stdio.cp, Array.uniq, master()->compile_error, Stdio.stdin->read, Stdio.stdout->write });

int perform()
{
   for (int i=0; i<m; i++)
   {
       call_out(funs[i & 7], m+(i*((i&1)*2 - 1)));
   }

   for (int i = 0; i<m; i++) {
       find_call_out(funs[i&7]);
   }

   for (int i = 0; i<m; i++) {
       remove_call_out(funs[i&7]);
   }
   return m*3;
}
