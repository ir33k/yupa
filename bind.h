/* Binds AKA shortcuts

Binds are uppercase characters in range from A to Z that hold any
string value.
*/

void bind_init(char *path);
void bind_set(char, char *str, char *path);
char *bind_get(char);
