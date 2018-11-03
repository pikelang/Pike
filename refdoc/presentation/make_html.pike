/*
 * Compat place-holder.
 */

#if 1
inherit Tools.Standalone.autodoc_to_html;
#else /* For dev purposes */
inherit __DIR__ + "/../../lib/modules/Tools.pmod/Standalone.pmod/autodoc_to_html.pike";
Tools.AutoDoc.Flags flags = Tools.AutoDoc.FLAG_NORMAL|Tools.AutoDoc.FLAG_NO_DYNAMIC;
#endif
