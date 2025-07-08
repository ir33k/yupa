/* Browsing undo history */

void undo_load(char *path);
void undo_add(char *uri, char *path);
char *undo_go(int offset);
