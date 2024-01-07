#include "walter.h"
#include "../lib/tab.h"

TEST("tab_new")
{
	struct tab tab = {0};
	OK(tab.head == 0);
	OK(tab.open == 0);
	OK(tab.n == 0);
	OK(tab.i == 0);

	tab_new(&tab);
	OK(tab.head != 0);
	OK(tab.open != 0);
	OK(tab.open == tab.head);
	OK(tab.n == 1);
	OK(tab.i == 0);
	OK(tab.open->raw[0]);
	OK(tab.open->fmt[0]);

	tab_new(&tab);
	tab_new(&tab);
	OK(tab.open == tab.head);
	OK(tab.n == 3);
	OK(tab.i == 0);
}

TEST("tab_open")
{
	struct tab tab = {0};

	tab_open(&tab);
	OK(tab.head != 0);
	OK(tab.open != 0);
	OK(tab.open == tab.head);
	OK(tab.n == 1);
	OK(tab.i == 0);

	tab_open(&tab);
	tab_open(&tab);
	tab_open(&tab);
	OK(tab.open != tab.head);
	OK(tab.n == 4);
	OK(tab.i == 3);
}

TEST("tab_goto")
{
	struct tab tab = {0};

	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	OK(tab.head != 0);
	OK(tab.open != 0);
	OK(tab.open == tab.head);
	OK(tab.n == 4);
	OK(tab.i == 0);

	tab_goto(&tab, 1);
	OK(tab.open != tab.head);
	OK(tab.n == 4);
	OK(tab.i == 1);

	tab_goto(&tab, 0);
	OK(tab.open == tab.head);
	OK(tab.n == 4);
	OK(tab.i == 0);

	tab_goto(&tab, 2);
	OK(tab.i == 2);

	tab_goto(&tab, 3);
	OK(tab.i == 3);

	tab_goto(&tab, 4);
	OK(tab.i == 3);

	tab_goto(&tab, 0);
	tab_goto(&tab, 5);
	OK(tab.i == 0);

	tab_goto(&tab, 2);
	tab_goto(&tab, -1);
	OK(tab.i == 2);
}

TEST("tab_close")
{
	struct tab tab = {0};

	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	OK(tab.head != 0);
	OK(tab.open != 0);
	OK(tab.open == tab.head);
	OK(tab.open->raw[0]);
	OK(tab.open->fmt[0]);
	OK(tab.n == 4);
	OK(tab.i == 0);
	tab_close(&tab, 0);
	OK(tab.head != 0);
	OK(tab.open != 0);
	OK(tab.open == tab.head);
	OK(tab.n == 3);
	OK(tab.i == 0);
	tab_close(&tab, 0);
	tab_close(&tab, 0);
	OK(tab.head != 0);
	OK(tab.open != 0);
	OK(tab.open == tab.head);
	OK(tab.head->next == 0);
	OK(tab.open->next == 0);
	OK(tab.n == 1);
	OK(tab.i == 0);
	tab_close(&tab, 0);
	OK(tab.head == 0);
	OK(tab.open == 0);
	OK(tab.n == 0);
	OK(tab.i == 0);

	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	tab_goto(&tab, tab.n-1);
	OK(tab.head != 0);
	OK(tab.open != 0);
	OK(tab.open != tab.head);
	OK(tab.n == 4);
	OK(tab.i == 3);
	tab_close(&tab, tab.n-1);
	OK(tab.n == 3);
	OK(tab.i == 2);
	tab_close(&tab, tab.n-1);
	OK(tab.n == 2);
	OK(tab.i == 1);
	tab_close(&tab, tab.n-1);
	OK(tab.n == 1);
	OK(tab.i == 0);
	tab_close(&tab, tab.n-1);
	OK(tab.head == 0);
	OK(tab.open == 0);
	OK(tab.n == 0);
	OK(tab.i == 0);

	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	tab_new(&tab);
	tab_goto(&tab, 1);
	tab_close(&tab, tab.i);
	OK(tab.n == 3);
	OK(tab.i == 1);
	tab_close(&tab, tab.i);
	OK(tab.n == 2);
	OK(tab.i == 1);
	tab_close(&tab, tab.i);
	OK(tab.n == 1);
	OK(tab.i == 0);
	tab_close(&tab, tab.i);
	OK(tab.n == 0);
	OK(tab.i == 0);
}

SKIP("tab_print")
{
	// TODO(irek): Hmm I can't test this function as don't have a
	// way in walter.h to test STDOUT at spot.
}
