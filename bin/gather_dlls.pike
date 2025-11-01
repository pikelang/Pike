/* Check which DLLs are imported by the pike binary and any dynamic modules,
   and copy them to the current directory if found on $PATH */

#if defined(__NT__)
#define PATH_SEPARATOR ";"
#else
#define PATH_SEPARATOR ":"
#endif

constant exclude_dlls = (<
  "pike.exe",

  "kernel32.dll",
  "shell32.dll",
  "ws2_32.dll",
  "advapi32.dll",
  "ole32.dll",
  "oleaut32.dll",
  "opengl32.dll"
>);

protected string load_rva(Stdio.File f, array(array(int)) sections,
                          int rva, int len)
{
  if (!len)
    return "";
  if (len < 0)
    len = 0;
  foreach(sections;; [int vsize, int vaddr, int size, int offs])
    if (rva >= vaddr && (rva-vaddr)+len <= vsize && (rva-vaddr)+len <= size) {
      f->seek(offs+(rva-vaddr));
      if (len)
        return f->read(len);
      if (vsize < size)
        size = vsize;
      size -= (rva-vaddr);
      string r = "";
      while (size > 0 && search(r, "\0") < 0) {
        string chunk = f->read((size > 256? 256 : size));
        if (!sizeof(chunk))
          break;
        r += chunk;
        size -= sizeof(chunk);
      }
      return (r/"\0")[0];
    }
  return "";
}

protected multiset(string) scan_pe_for_dll_imports(string filename)
{
  Stdio.File f = Stdio.File(filename, "r");
  string(8bit) dos_header = f->read(64);
  if (sizeof(dos_header) < 64 || dos_header[..1] != "MZ")
    return (<>);
  int pe_offs;
  sscanf(dos_header[60..], "%-4c", pe_offs);
  f->seek(pe_offs);
  string(8bit) nt_header = f->read(24);
  if (sizeof(nt_header) < 24 || nt_header[..3] != "PE\0\0")
    return (<>);
  int machine, num_sections, size_of_optional_headers;
  sscanf(nt_header[4..7], "%-2c%-2c", machine, num_sections);
  sscanf(nt_header[20..21], "%-2c", size_of_optional_headers);
  write("Scanning %s\n", filename);
  if (size_of_optional_headers < 2)
    return (<>);
  string optional_headers = f->read(size_of_optional_headers);
  if (sizeof(optional_headers) < size_of_optional_headers)
    return (<>);
  int magic, offs = size_of_optional_headers;
  sscanf(optional_headers[..1], "%-2c", magic);
  if (magic == 0x10b)
    offs = 92;
  else if (magic == 0x20b)
    offs = 108;
  if (offs + 4 > size_of_optional_headers)
    return (<>);
  array(array(int)) sections = allocate(num_sections);
  for (int i=0; i<num_sections; i++) {
    string(8bit) section_header = f->read(40);
    if (sizeof(section_header) < 40)
      break;
    string name = section_header[..7]-"\0";
    sections[i] = array_sscanf(section_header[8..], "%-4c%-4c%-4c%-4c");
  }
  int number_of_rva_and_sizes;
  sscanf(optional_headers[offs..offs+3], "%-4c", number_of_rva_and_sizes);
  array(array(int)) data_directories =
    map(optional_headers[offs+4..offs+3+8*number_of_rva_and_sizes]/8,
        lambda(string s) { return array_sscanf(s, "%-4c%-4c"); });
  if (sizeof(data_directories) < 2 || !data_directories[1][1])
    return (<>);
  string(8bit) import_directory = load_rva(f, sections, @data_directories[1]);
  multiset(string) dlls = (<>);
  foreach(import_directory/20;; string s) {
    if (s == "\0"*20)
      break;
    int othunk, stamp, chain, name, thunk;
    sscanf(s, "%-4c%-4c%-4c%-4c%-4c", othunk, stamp, chain, name, thunk);
    if (name) {
      string n = load_rva(f, sections, name, -1);
      if (sizeof(n) && !exclude_dlls[lower_case(n)] &&
          !has_prefix(lower_case(n), "api-ms-win-crt-"))
        dlls[n] = 1;
    }
  }
  return dlls;
}

int main(int argc, array(string) argv)
{
  multiset(string) dlls = (<>);
  foreach(argv[1..];; string filename)
    if (Stdio.is_dir(filename)) {
      foreach(Filesystem.Traversion(filename); string dir; string file)
        if(has_suffix(lower_case(file), ".so"))
          dlls |= scan_pe_for_dll_imports(dir + file);
    } else
      dlls |= scan_pe_for_dll_imports(filename);

  array(string) path = getenv("PATH")/PATH_SEPARATOR;
  multiset(string) processed = (<>);
  while(sizeof(dlls-processed)) {
    write("Needed dlls: %s\n", indices(dlls-processed)*", ");
    foreach(indices(dlls-processed);; string dll) {
      if (Stdio.is_file(dll)) {
        write("%s already exists\n", dll);
        processed[dll] = 1;
      } else {
        foreach(path;; string candidate)
          if (Stdio.is_file((candidate = combine_path(candidate, dll)))) {
            write("Found %s as %s\n", dll, candidate);
            Stdio.cp(candidate, dll);
            processed[dll] = 1;
            break;
          }
      }
      if (processed[dll])
        dlls |= scan_pe_for_dll_imports(dll);
      else {
        write("%s not found\n", dll);
        processed[dll] = 1;
      }
    }
  }

  return 0;
}
