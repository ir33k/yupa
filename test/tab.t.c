#include "walter.h"
#define LOGERR_IMPLEMENTATION
#include "../lib/le.h"
#include "../lib/tab.h"

TEST("tab_new")
{
	struct tab tab = {0};
	OK(tab.head == 0);
	OK(tab.open == 0);
	OK(tab.n == 0);
	OK(tab.i == 0);
}
