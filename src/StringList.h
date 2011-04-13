/*
 * $Id: StringList.h,v 1.1.1.1 2008/10/10 11:05:51 scott Exp $
 *
 */

#ifndef STRING_LIST_H
#define STRING_LIST_H

class StringList
{
public:
	StringList();
	~StringList();

	StringList& operator=(StringList &rhs);

	int getNumStrings();
	
	int addString(const char *s);
	const char *getString(int index);
	bool removeString(int index);
	void clearList();
	int findString(const char *s);

private:
	bool growList(int new_size);

	int _maxStrings;
	int _numStrings;
	char ** _list;
};


#endif // ifndef STRING_LIST_H