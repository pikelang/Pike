/*
Check the "signal" directives in all .pre files.

Uses the IMG: or TIMG: lines to instantiate objects, where possible.
*/

int main()
{
	mapping(string:array(string)) directives = ([]);
	multiset(string) failed = (<>);
	GTK2.setup_gtk();
	object mei = (object)"make_example_image.pike";
	foreach (sort(get_dir("source")), string fn)
	{
		string data; catch {data = Stdio.read_file("source/" + fn);};
		if (!data) continue;
		sscanf("\n"+data, "%*s\nclass %s;\n", string cls);
		if (!cls) continue; //Probably not a widget file (eg constants.pre)
		sscanf(cls, "%s.%s", string module, string name);
		//Internal class names are formatted differently.
		string gtkcls = sprintf("%c%s%s", module[0], lower_case(module[1..])-"2", name);
		directives[gtkcls] = array_sscanf((data/"\n")[*], "signal %s;") * ({ });
		if (sscanf(data, "%*sIMG: %s\n", string source))
		{
			//write("Creating object: %s\n", source);
			object obj = mei->get_widget_from(source);
			if (obj) obj->destroy();
		}
		else
		{
			//Attempt to create the object with default arguments.
			program pgm;
			if (mixed x=(["GTK2":GTK2, "Gnome2":Gnome2, "Pango":Pango, "GDK2":GDK2])[module])
				pgm = x[name]; //White list of easy ones.
			else if (module == "G")
				pgm = GTK2[name = "G"+name]; //G.Object --> GTK2.GObject
			else continue; //Don't know how to find this one.
			if (catch (pgm()) && catch (pgm(([]))))
			{
				write("Unable to instantiate %s\n", cls);
				failed[gtkcls] = 1;
			}
		}
	}
	write("===========\n");
	//Okay, we should now have (a) a mapping of class names to the signal
	//directives in the .pre file, and (b) as many as possible of them
	//instantiated, and thus the classes initialized.
	mapping(string:array(string)) discovered = ([]);
	foreach (GTK2.list_signals(), mapping sig)
		discovered[sig["class"]] += ({sig->name});
	foreach (directives; string cls; array expected)
	{
		array actual = m_delete(discovered, cls) || ({ });
		actual = replace(actual[*], "_", "-");
		expected = replace(expected[*], "_", "-");
		array missing = actual - expected;
		if (sizeof(missing)) write("%s: need to add%{ %s%}\n", cls, missing);
		if (failed[cls] && !sizeof(actual)) continue; //Don't list ones we didn't find when instantiation failed.
		array extra = expected - actual;
		if (sizeof(extra)) write("%s: didn't find%{ %s%}\n", cls, extra);
	}
	foreach (discovered; string cls; array actual)
		write("%s: Unknown class with signals%{ %s%}\n", cls, actual);
}
