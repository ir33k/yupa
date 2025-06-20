/* Cache some number of files for later reuse */

void cache_add(char *key, char *path);
char *cache_get(char *key);
