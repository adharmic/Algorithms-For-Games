#include <windows.h>
#include <Windowsx.h>
#include <d2d1.h>

#include <algorithm>
#include <vector>
#include <set>
#include <memory>
using namespace std;

#pragma comment(lib, "d2d1")

#include "basewin.h"
#include "resource.h"
#include "Vector2D.h"

class QuickHull {

public:
	vector<D2D1_ELLIPSE> points;
	vector<D2D1_ELLIPSE> hull;
	D2D1_ELLIPSE first_point;

	QuickHull(vector<D2D1_ELLIPSE> orig_list) {
		points = orig_list;
	}


	D2D1_ELLIPSE PointFarthestFromEdge(D2D1_ELLIPSE a, D2D1_ELLIPSE b, vector<D2D1_ELLIPSE> p) {
		Vector2D *e = new Vector2D(b.point.x - a.point.x, b.point.y - a.point.y);
		Vector2D *eperp = new Vector2D(-(*e).y, (*e).x);

		int best_index = -1;
		float max_val = -FLT_MAX;
		float right_most_val = -FLT_MAX;

		for (size_t i = 0; i < p.size(); i++) {
			Vector2D *curr = new Vector2D(p[i].point.x - a.point.x, (p[i].point.y - a.point.y)*-1);
			(*curr).Normalize();
			(*e).Normalize();
			(*eperp).Normalize();
			float d = (*curr).DotProduct((*eperp));
			float r = (*curr).DotProduct((*e));
			if (d > max_val || (d == max_val && r > right_most_val)) {
				best_index = i;
				max_val = d;
				right_most_val = r;
			}
		}

		if (best_index != -1) {
			return p[best_index];
		}
		a.point.x = -1;
		a.point.y = -1;
		return a;
	}

	D2D1_ELLIPSE* Insert(D2D1_ELLIPSE points[], int index, int size, D2D1_ELLIPSE new_point) {
		for (size_t i = size; i > index; i--) {
			points[i] = points[i - 1];
		}
		points[index] = new_point;

		return points;
	}

	bool Contains(vector<D2D1_ELLIPSE> points, D2D1_ELLIPSE to_be_found) {
		for (size_t i = 0; i < points.size(); i++) {
			if (points[i].point.x == to_be_found.point.x && points[i].point.y == to_be_found.point.y) {
				return true;
			}
		}
		return false;
	}

	int FindSide(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE curr_point) {
		int value = (curr_point.point.y - p1.point.y) * (p2.point.x - p1.point.x) - (p2.point.y - p1.point.y) * (curr_point.point.x - p1.point.x);

		if (value > 0) {
			return 1;
		}
		if (value < 0) {
			return -1;
		}
		return 0;
	}

	

	int LineDistance(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE curr_point) {
		return abs((curr_point.point.y - p1.point.y) * (p2.point.x - p1.point.x) - (p2.point.y - p1.point.y) * (curr_point.point.x - p1.point.x));
	}

	void Quick(vector<D2D1_ELLIPSE> points, D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, int side) {
		int in_hull = -1;
		int max_dist = 0;

		for (int i = 0; i < points.size(); i++) {
			int temp = LineDistance(p1, p2, points[i]);
			if (FindSide(p1, p2, points[i]) == side && temp > max_dist) {
				in_hull = i;
				max_dist = temp;
			}
		}

		if (in_hull == -1) {
			if (!Contains(hull, p1))
				hull.push_back(p1);
			if (!Contains(hull, p2))
				hull.push_back(p2);
			return;
		}

		Quick(points, points[in_hull], p1, -1 * FindSide(points[in_hull], p1, p2));
		Quick(points, points[in_hull], p2, -1 * FindSide(points[in_hull], p2, p1));
	}

	vector<D2D1_ELLIPSE> GetConvexHull() {
		int min_x = 0;
		int max_x = 0;

		for (int i = 0; i < points.size(); i++) {
			if (points[i].point.x < points[min_x].point.x) {
				min_x = i;
			}
			if (points[i].point.x > points[max_x].point.x) {
				max_x = i;
			}
		}

		Quick(points, points[min_x], points[max_x], 1);
		Quick(points, points[min_x], points[max_x], -1);
		return hull;
	}

	//vector<D2D1_ELLIPSE>  GetConvexHull() {
	//	vector<D2D1_ELLIPSE> hull;
	//	D2D1_ELLIPSE empt[15];
	//	int num_points = 0;

	//	D2D1_ELLIPSE left_most_point = *selection;
	//	D2D1_ELLIPSE right_most_point = *selection;
	//	D2D1_ELLIPSE top_most_point = *selection;
	//	D2D1_ELLIPSE bottom_most_point = *selection;

	//	for (size_t i = 0; i < original_list.size(); i++) {
	//		D2D1_ELLIPSE current_point = *selection;
	//		if (current_point.point.x < left_most_point.point.x) {
	//			left_most_point = current_point;
	//		}
	//		if (current_point.point.x > right_most_point.point.x) {
	//			right_most_point = current_point;
	//		}
	//		if (current_point.point.y < top_most_point.point.y) {
	//			top_most_point = current_point;
	//		}
	//		if (current_point.point.y > bottom_most_point.point.y) {
	//			bottom_most_point = current_point;
	//		}
	//		advance(selection, 1);
	//	}

	//	/*hull.push_back(top_most_point);
	//	hull.push_back(right_most_point);
	//	hull.push_back(bottom_most_point);
	//	hull.push_back(left_most_point);*/

	//	empt[0] = top_most_point;
	//	empt[1] = right_most_point;
	//	empt[2] = bottom_most_point;
	//	empt[3] = left_most_point;
	//	num_points += 4;

	//	D2D1_ELLIPSE farthest_pt;

	//	for (size_t i = 1; i < num_points + 1; i++) {
	//		if (i != num_points) {
	//			farthest_pt = PointFarthestFromEdge(empt[i-1], empt[i], original_list);
	//		}
	//		else {
	//			farthest_pt = PointFarthestFromEdge(empt[num_points - 1], empt[0], original_list);
	//		}
	//		if ((farthest_pt.point.x != -1 || farthest_pt.point.y != -1 ) && !Contains(empt, num_points, farthest_pt)) {
	//			//hull.insert(placement, farthest_pt);
	//			num_points++;
	//			Insert(empt, i, num_points, farthest_pt);
	//			if (i < 1) {
	//				i = i - 2;
	//			}
	//			else {
	//				i--;
	//			}
	//		}
	//	}

	//	for (size_t i = 0; i < num_points; i++) {
	//		hull.push_back(empt[i]);
	//	}
	//	return hull;


	//}
};
