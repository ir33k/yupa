/* Binds AKA shortcuts

Binds are uppercase characters in range from A to Z
that hold any string value.
*/

void bind_set(char, char *str);
char *bind_get(char);
void bind_load(char *path);
