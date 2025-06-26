/* Links storage

In contrast to URI link don't have to be full URI, it can be relative
to URI of currently open page.  As convention the first link, with
index of 0, is always an URI to currently open page.
*/

void link_clear();
unsigned link_store(char *);
char *link_get(unsigned i);
