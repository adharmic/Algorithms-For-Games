#pragma once
#include <windows.h>
#include <Windowsx.h>
#include <d2d1.h>

#include <list>
#include <memory>
#include "main.cpp";
using namespace std;

#pragma comment(lib, "d2d1")

#include "basewin.h"
#include "resource.h"
class QuickHull
{
	list<shared_ptr<MyEllipse>> original_list;
	list<shared_ptr<MyEllipse>>::iterator selection;
	QuickHull(list<shared_ptr<MyEllipse>> orig_list);
	list<MyEllipse> getConvexHull();
};

