/* Cache some number of files for later reuse */

why_t cache_add(char *key, char *path);
char *cache_get(char *key);
void cache_cleanup();
