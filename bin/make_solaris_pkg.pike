#! /usr/bin/env pike

/*
# this script prepares a pike package from a previously created
# pike installation.
#
# usage: make_solaris_pkg.pike --prefix=installprefix \
#   --pkgdest=pkgdestdir --installroot=installroot
#
# assumptions:
#    1. we have previously prepared an installed pike in a buildroot using
#         cd $BUILDDIR; make buildroot=/path/to/installroot install
#    2. the return value of version() is "majornum.minornum release buildnum"
#    3. we have enough room to do the whole package operation under $BUILDDIR
#    4. that we call this script using the same version of pike as to be 
#         packaged.
#
# to do:
#    1. do we want to have dependencies? currently we only depend on Java 
#         and GMP.
#    2. do we want to split the package up for non-default modules such
#         as Oracle, etc?
*/

constant packagename="IDApike";

string installprefix;
string pkgdest;
string installroot;
string arch;

int main(int argc, array argv)
{

  if(!parse_options(argv)) return 1;
  arch=get_pike_arch();
  prepare_tree();
  Stdio.write_file("pkginfo", make_pkginfo());
  Stdio.write_file("depend", make_depend());
  Stdio.write_file("prototype", make_prototype());
  make_package();
  return 0;
}


void usage()
{
       werror("Usage: make_solaris_pkg.pike [--help] --prefix=pikeprefix"
	" --installroot=tempimginstallroot --pkgdest=pkgdestinationdir\n");
}

int parse_options(array args)
{
    if(Getopt.find_option(args, "", "help") || sizeof(args)==0)
    {
	usage();
	return 0;
    }

    installprefix=Getopt.find_option(args, "", "prefix");
    installroot=Getopt.find_option(args, "", "installroot");
    pkgdest=Getopt.find_option(args, "", "pkgdest");

    if(!(installprefix && installroot && pkgdest))
    {
	usage();
	return 0;
    }
    if(!Stdio.is_dir(installroot))
    {
        werror("Error: install image root " + installroot + 
		" is not a valid path.\n");
	return 0;
    }
    if(!Stdio.is_dir(pkgdest))
    {
        werror("Error: package destination " + installroot + 
		" is not a valid path.\n");
	return 0;
    }

    return 1;
}

string get_pike_prefix()
{
   return (installprefix);
}

string get_pike_version()
{
   return (version()/"v")[1];
}

string get_pike_arch()
{
    return (Process.popen("uname -p")-"\n");
}

void prepare_tree()
{
  //
  // we want to remove the bin directory from where it is (pkgroot/pike), 
  // and move it under pkgroot
  // also, we want a pike-major.minor in the bin directory.
  //
  string ver=get_pike_version();
  ver=replace(ver, " release ", ".");
  Stdio.recursive_rm(installroot + "/" + installprefix + "/bin");
  Process.system("mkdir " + installroot + "/" + 
	installprefix + "/pike/bin");
  Process.system("ln -s ../" 
	+ ver + "/bin/pike " + 
	installroot + "/" + installprefix + "/pike/bin/pike");
  Process.system("ln -s ../" 
	+ ver + "/bin/pike " + 
	installroot + "/" + installprefix + "/pike/bin/pike-" + ((ver/".")[0..1]*"."));
}

string make_pkginfo()
{
  string pkginfo="";

  pkginfo+="PKG=" + packagename + "\n";
  pkginfo+="NAME=Pike Programming Language\n";
  pkginfo+="BASEDIR=" + get_pike_prefix() + "\n";
  pkginfo+="CATEGORY=application\n";
  pkginfo+="ARCH=" + arch + "\n";
  pkginfo+="VENDOR=The Pike Project\n";
  pkginfo+="EMAIL=pike@pike.ida.liu.se\n";
  pkginfo+="MAXINST=255\n";
  pkginfo+="VERSION=" + get_pike_version() + "\n";

  return pkginfo;
}

string make_depend()
{
  string depend="";
/*
echo "P SFWftype Freetype Font Rendering Library" >> depend
echo " (sparc)" >> depend
echo "P SMCzlib zlib compression Library" >> depend
echo " (sparc)" >> depend
echo "P SFWpng PNG graphics Library" >> depend
echo " (sparc)" >> depend
*/
  depend+="P GNUgmp GNU Multi Precision Library\n";
  depend+=" (" + arch + ")\n";
  depend+="P SUNWjvrt Java Run Time Environment\n";
  depend+=" (" + arch + ")\n";
/*
echo "P SFWjpg JPEG graphics Library" >> depend
echo " (sparc)" >> depend
echo "P SFWtiff TIFF graphics Library" >> depend
echo " (sparc)" >> depend
*/
return depend;
}

string make_prototype()
{
  string prototype="";
  
  prototype+="i pkginfo\n";
  prototype+="i depend\n";

  prototype+=Process.popen("pkgproto " + installroot + "/" + 
	installprefix + "=");

  return prototype;
}



int make_package()
{
  string ver=get_pike_version();
  ver=replace(ver, " release ", ".");
  Stdio.mkdirhier(installroot + "/../pkgbuild");
  Process.popen("pkgmk -o -d " + installroot + "/../pkgbuild");
  Process.popen("pkgtrans -s " + installroot + "/../pkgbuild " +
     pkgdest + "/" + "pike-" + ver +"-"+ arch + ".pkg " + packagename);

  if(!Stdio.recursive_rm(installroot + "/../pkgbuild"))
    werror("error deleting pkgbuild temporary directory.\n");
  if(!Stdio.recursive_rm("depend"))
    werror("error deleting depend.\n");
  if(!Stdio.recursive_rm("pkginfo"))
    werror("error deleting pkginfo.\n");
  if(!Stdio.recursive_rm("prototype"))
    werror("error deleting prototype.\n");
  
  return 1;
}
