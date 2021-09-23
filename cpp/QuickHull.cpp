#include <windows.h>
#include <Windowsx.h>
#include <d2d1.h>

#include <algorithm>
#include <list>
#include <memory>
using namespace std;

#pragma comment(lib, "d2d1")

#include "basewin.h"
#include "resource.h"
#include "Vector2D.h"

class QuickHull {

public:
	list<D2D1_ELLIPSE> original_list;
	list<D2D1_ELLIPSE>::iterator selection;

	QuickHull(list<D2D1_ELLIPSE> orig_list) {
		original_list = orig_list;
		selection = original_list.begin();
	}


	D2D1_ELLIPSE PointFarthestFromEdge(D2D1_ELLIPSE a, D2D1_ELLIPSE b, list<D2D1_ELLIPSE> p) {
		Vector2D *e = new Vector2D(b.point.x - a.point.x, b.point.y - a.point.y);
		Vector2D *eperp = new Vector2D(-(*e).y, (*e).x);

		int best_index = -1;
		float max_val = -FLT_MAX;
		float right_most_val = -FLT_MAX;

		for (size_t i = 0; i < p.size(); i++) {
			auto point = std::next(p.begin(), i);
			D2D1_ELLIPSE curr_point = *point;
			Vector2D *curr = new Vector2D(curr_point.point.x - a.point.x, curr_point.point.y - a.point.y);
			float d = (*curr).DotProduct(*eperp);
			float r = (*curr).DotProduct(*e);
			if (d > max_val || (d == max_val && r > right_most_val)) {
				best_index = i;
				max_val = d;
				right_most_val = r;
			}
		}

		if (best_index != -1) {
			auto point = std::next(p.begin(), best_index);
			D2D1_ELLIPSE final_point = *point;
			return final_point;
		}
		a.point.x = -1;
		a.point.y = -1;
		return a;
	}

	bool Contains(list<D2D1_ELLIPSE> points, D2D1_ELLIPSE to_be_found) {
		for (size_t i = 0; i < points.size(); i++) {
			auto point = std::next(points.begin(), i);
			D2D1_ELLIPSE win = *point;
			if ((win.point.x == to_be_found.point.x) && (win.point.y == to_be_found.point.y)) {
				return true;
			}
		}
		return false;
	}

	list<D2D1_ELLIPSE>  GetConvexHull() {
		list<D2D1_ELLIPSE> hull;
		list<D2D1_ELLIPSE> extreme_points;

		D2D1_ELLIPSE left_most_point = *selection;
		D2D1_ELLIPSE right_most_point = *selection;
		D2D1_ELLIPSE top_most_point = *selection;
		D2D1_ELLIPSE bottom_most_point = *selection;

		for (size_t i = 0; i < original_list.size(); i++) {
			D2D1_ELLIPSE current_point = *selection;
			if (current_point.point.x < left_most_point.point.x) {
				left_most_point = current_point;
			}
			if (current_point.point.x > right_most_point.point.x) {
				right_most_point = current_point;
			}
			if (current_point.point.y < top_most_point.point.y) {
				top_most_point = current_point;
			}
			if (current_point.point.y > bottom_most_point.point.y) {
				bottom_most_point = current_point;
			}
			advance(selection, 1);
		}

		hull.push_back(top_most_point);
		hull.push_back(right_most_point);
		hull.push_back(bottom_most_point);
		hull.push_back(left_most_point);

		extreme_points.push_back(top_most_point);
		extreme_points.push_back(right_most_point);
		extreme_points.push_back(bottom_most_point);
		extreme_points.push_back(left_most_point);

		for (size_t i = 1; i < hull.size() + 1; i++) {
			auto hull_a = std::next(hull.begin(), 0);
			auto hull_b = std::next(hull.begin(), 0);
			auto placement = std::next(hull.begin(), i);
			if (i != hull.size()) {
				hull_a = std::next(hull.begin(), i - 1);
				hull_b = std::next(hull.begin(), i);
			}
			else {
				hull_a = std::next(hull.begin(), hull.size() - 1);
				hull_b = std::next(hull.begin(), 0);
			}
			D2D1_ELLIPSE farthest_pt = PointFarthestFromEdge(*hull_a, *hull_b, original_list);
			if (!Contains(hull, farthest_pt)) {
				hull.insert(placement, farthest_pt);
				if (i < 1) {
					i = i - 2;
				}
				else {
					i--;
				}
			}
		}
		return hull;


	}
};
